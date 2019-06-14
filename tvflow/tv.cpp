#include "tv.h"
#include <teem/ell.h>

#pragma warning(disable : 4244 4267)
// warning C4244: '*=' : conversion from 'double' to 'float', possible loss of data	
// warning C4267: 'initializing' : conversion from 'size_t' to 'int', possible loss of data


int
tvFlow(Nrrd *nout, Nrrd *nscale, Nrrd *nin, double tau, double eps,
       unsigned int maxItr, unsigned int scaleMinItr,
	   bool (*callback)( unsigned itr, Nrrd* nout ),
	   bool (*callback_with_scale)( unsigned itr, Nrrd* nout, Nrrd* nscale ) )
{
	return tvFlow( nout, nscale, nin, tau, eps, 0.001 /* default gradient_eps */,
		   maxItr, scaleMinItr, callback, callback_with_scale );
}

/* Does TV flow on nin, which can be 2-4 dimensional. If it is 4D, we assume
 * that the 0th axis defines vector components and uses coupled diffusivities
 * for them.
 * Limitation: Assumes isotropic pixels/voxels; does not try to read any
 * spacings from the input Nrrd
 * Currently only works for float input
 */
/* TODO: Change API so that we can handle 2D color images */
int
tvFlow(Nrrd *nout, Nrrd *nscale, Nrrd *nin, double tau, double eps, double gradient_eps,
       unsigned int maxItr, unsigned int scaleMinItr,
	   bool (*callback)( unsigned itr, Nrrd* nout ),
	   bool (*callback_with_scale)( unsigned itr, Nrrd* nout, Nrrd* nscale ) )
{
  if (nin==NULL || nin->dim<2 || nin->type!=nrrdTypeFloat)
    return 1;
  double epssqr=eps*eps;
  /* determine spatial dimensionality */
  unsigned int D = (nin->dim==2)?2:3;
  double Dtau = D*tau;
  /* find shape of input Nrrd */
  size_t nk = (nin->dim==4)?nin->axis[0].size:1;
  size_t nx = (nin->dim==4)?nin->axis[1].size:nin->axis[0].size;
  size_t ny = (nin->dim==4)?nin->axis[2].size:nin->axis[1].size;
  size_t nz;
  if (nin->dim==3) nz=nin->axis[2].size;
  else if (nin->dim==4) nz=nin->axis[3].size;
  else nz=1;
  double *gx = (double*)malloc(sizeof(double)*(nx-1)); /* diffusivities between pixels */
  double *ax = (double*)malloc(sizeof(double)*nx); /* diagonals of the sparse system matrix */
  double *bx = (double*)malloc(sizeof(double)*nx);
  double *cx = (double*)malloc(sizeof(double)*nx);
  double *dx = (double*)malloc(sizeof(double)*nx*nk);
  /* same variables for y direction */
  double *gy = (double*)malloc(sizeof(double)*(ny-1));
  double *ay = (double*)malloc(sizeof(double)*ny);
  double *by = (double*)malloc(sizeof(double)*ny);
  double *cy = (double*)malloc(sizeof(double)*ny);
  double *dy =(double*) malloc(sizeof(double)*ny*nk);
  /* same variables for z direction */
  double *gz = (double*)malloc(sizeof(double)*(nz-1));
  double *az = (double*)malloc(sizeof(double)*nz);
  double *bz = (double*)malloc(sizeof(double)*nz);
  double *cz = (double*)malloc(sizeof(double)*nz);
  double *dz = (double*)malloc(sizeof(double)*nz*nk);
  /* Allocate buffers */
  Nrrd *nbuf = nrrdNew();
  nrrdCopy(nbuf, nin);
  nrrdCopy(nout, nin);
  /* If scale measurement is required, allocate more buffers */
  Nrrd *ntime = NULL;
  float *dscale= NULL, *dtime=NULL;
  if (nscale!=NULL) {
    ntime=nrrdNew(); /* the following can probably be made more efficient */
    nrrdCopy(ntime, nin); nrrdSetZero(ntime); dtime=(float*)ntime->data;
    nrrdCopy(nscale, nin); nrrdSetZero(nscale); dscale=(float*)nscale->data;
  }
  float *indata = (float*) nbuf->data;
  float *outdata = (float *) nout->data;
  for (unsigned int itr=0; itr<maxItr; itr++) {
    /* evaluate operator in x direction */
    for (unsigned int z=0; z<nz; z++) {
      for (unsigned int y=0; y<ny; y++) {
	/* squared gradient magnitude in x direction */
	float *I=indata+nk*nx*(y+ny*z);
	for (unsigned int x=0; x<nx-1; x++) {
	  gx[x] = 0;
	  for (unsigned int k=0; k<nk; k++) {
	    float g=I[nk]-I[0];
	    gx[x]+=g*g;
	    I++;
	  }
	}
	/* add squared gradient magnitude in y direction */
	if (y!=0 && y!=ny-1) {
	  int ipjp=nx*nk+nk, ijp=nx*nk,
	    ipjm=-(int)nx*nk+nk, ijm=-(int)nx*nk;
	  I=indata+nk*nx*(y+ny*z);
	  for (unsigned int x=0; x<nx-1; x++) {
	    for (unsigned int k=0; k<nk; k++) {
	      float g=0.25*(I[ipjp]-I[ipjm]+I[ijp]-I[ijm]);
	      gx[x]+=g*g;
	      I++;
	    }
	  }
	}
	/* add squared gradient magnitude in z direction */
	if (z!=0 && z!=nz-1) {
	  int ipjp=nx*ny*nk+nk, ijp=nx*ny*nk,
	    ipjm=-(int)nx*ny*nk+nk, ijm=-(int)nx*ny*nk;
	  I=indata+nk*nx*(y+ny*z);
	  for (unsigned int x=0; x<nx-1; x++) {
	    for (unsigned int k=0; k<nk; k++) {
	      float g=0.25*(I[ipjp]-I[ipjm]+I[ijp]-I[ijm]);
	      gx[x]+=g*g;
	      I++;
	    }
	  }
	}
	/* compute diffusivity from summed gradient magnitude */
	for (unsigned int x=0; x<nx-1; x++) {
	  gx[x]=1.0/sqrt(gx[x]+epssqr); /* total variation */
	}
	/* create tridiagonal system matrix from diffusivities */
	ax[0]=0; cx[nx-1]=0;
	for (unsigned int x=0; x<nx-1; x++) {
	  ax[x+1]=cx[x]=-Dtau*gx[x];
	  bx[x]=1-ax[x]-cx[x];
	}
	bx[nx-1]=1-ax[nx-1];
	/* solve using tridiagonal matrix algorithm; forward step */
	cx[0]=cx[0]/bx[0];
	I=indata+nk*nx*(y+ny*z);
	double *dp=dx;
	for (unsigned int k=0; k<nk; k++) {
	  *(dp++)=*(I++)/bx[0];
	}
	for (unsigned int x=1; x<nx; x++) {
	  double id = 1.0 / (bx[x]-cx[x-1]*ax[x]);
	  cx[x]*=id;
	  for (unsigned int k=0; k<nk; k++) {
	    dp[0]=(I[0]-dp[-(int)nk]*ax[x])*id;
	    dp++; I++;
	  }
	}
	float *O=outdata+nk*nx*(y+1+ny*z)-1; /* last element of the line */
	dp=dx+nx*nk-1;
	/* backward step */
	for (unsigned int k=0; k<nk; k++) {
	  *(O--)=*(dp--);
	}
	for (int x=nx-2; x>=0; x--) {
	  for (unsigned int k=0; k<nk; k++) {
	    O[0]=dp[0]-cx[x]*O[nk];
	    O--; dp--;
	  }
	}
      }
    }
    /* evaluate operator in y direction */
    for (unsigned int z=0; z<nz; z++) {
      for (unsigned int x=0; x<nx; x++) {
	/* squared gradient magnitude in y direction */
	float *I=indata+nk*(x+nx*ny*z);
	int ip=nk*nx;
	for (unsigned int y=0; y<ny-1; y++) {
	  gy[y] = 0;
	  for (unsigned int k=0; k<nk; k++) {
	    float g=I[ip]-I[0];
	    gy[y]+=g*g;
	    I++;
	  }
	  I+=ip-nk;
	}
	/* add squared gradient magnitude in x direction */
	if (x!=0 && x!=nx-1) {
	  int ipjp=nx*nk+nk, ijp=nk, ipjm=nx*nk-(int)nk, ijm=-(int)nk;
	  I=indata+nk*(x+nx*ny*z);
	  for (unsigned int y=0; y<ny-1; y++) {
	    for (unsigned int k=0; k<nk; k++) {
	      float g=0.25*(I[ipjp]-I[ipjm]+I[ijp]-I[ijm]);
	      gy[y]+=g*g;
	      I++;
	    }
	    I+=ip-nk;
	  }
	}
	/* add squared gradient magnitude in z direction */
	if (z!=0 && z!=nz-1) {
	  int ipjp=nx*ny*nk+nx*nk, ijp=nx*ny*nk,
	    ipjm=-(int)nx*ny*nk+nx*nk, ijm=-(int)nx*ny*nk;
	  I=indata+nk*(x+nx*ny*z);
	  for (unsigned int y=0; y<ny-1; y++) {
	    for (unsigned int k=0; k<nk; k++) {
	      float g=0.25*(I[ipjp]-I[ipjm]+I[ijp]-I[ijm]);
	      gy[y]+=g*g;
	      I++;
	    }
	    I+=ip-nk;
	  }
	}
	/* compute diffusivity from summed gradient magnitude */
	for (unsigned int y=0; y<ny-1; y++) {
	  gy[y]=1.0/sqrt(gy[y]+epssqr); /* total variation */
	}
	/* create tridiagonal system matrix from diffusivities */
	ay[0]=0; cy[ny-1]=0;
	for (unsigned int y=0; y<ny-1; y++) {
	  ay[y+1]=cy[y]=-Dtau*gy[y];
	  by[y]=1-ay[y]-cy[y];
	}
	by[ny-1]=1-ay[ny-1];
	/* solve using tridiagonal matrix algorithm; forward step */
	cy[0]=cy[0]/by[0];
	I=indata+nk*(x+nx*ny*z);
	double *dp=dy;
	for (unsigned int k=0; k<nk; k++) {
	  *(dp++)=*(I++)/by[0];
	}
	I+=nk*(nx-1);
	for (unsigned int y=1; y<ny; y++) {
	  double id = 1.0 / (by[y]-cy[y-1]*ay[y]);
	  cy[y]*=id;
	  for (unsigned int k=0; k<nk; k++) {
	    dp[0]=(I[0]-dp[-(int)nk]*ay[y])*id;
	    dp++; I++;
	  }
	  I+=nk*(nx-1);
	}
	dp=dy+(ny-1)*nk-1;
	/* backward step */
	for (int y=ny-2; y>=0; y--) {
	  for (unsigned int k=0; k<nk; k++) {
	    dp[0]+=-cy[y]*dp[nk];
	    dp--;
	  }
	}
	/* add contents of dy to output */
	dp=dy;
	float *O=outdata+nk*(x+nx*ny*z);
	for (unsigned int y=0; y<ny; y++) {
	  for (unsigned int k=0; k<nk; k++) {
	    *(O++)+=*(dp++);
	  }
	  O+=nk*(nx-1);
	}
      }
    }
    if (nz>1) {
      /* evaluate operator in z direction */
      for (unsigned int y=0; y<ny; y++) {
	for (unsigned int x=0; x<nx; x++) {
	  /* squared gradient magnitude in z direction */
	  float *I=indata+nk*(x+nx*y);
	  int ip=nk*nx*ny;
	  for (unsigned int z=0; z<nz-1; z++) {
	    gz[z] = 0;
	    for (unsigned int k=0; k<nk; k++) {
	      float g=I[ip]-I[0];
	      gz[z]+=g*g;
	      I++;
	    }
	    I+=ip-nk;
	  }
	  /* add squared gradient magnitude in x direction */
	  if (x!=0 && x!=nx-1) {
	    int ipjp=ip+nk, ijp=nk, ipjm=ip-(int)nk, ijm=-(int)nk;
	    I=indata+nk*(x+nx*y);
	    for (unsigned int z=0; z<nz-1; z++) {
	      for (unsigned int k=0; k<nk; k++) {
		float g=0.25*(I[ipjp]-I[ipjm]+I[ijp]-I[ijm]);
		gz[z]+=g*g;
		I++;
	      }
	    }
	    I+=ip-nk;
	  }
	  /* add squared gradient magnitude in y direction */
	  if (y!=0 && y!=ny-1) {
	    int ipjp=ip+nx*nk, ijp=nx*nk,
	      ipjm=ip-(int)nx*nk, ijm=-(int)nx*nk;
	    I=indata+nk*(x+nx*y);
	    for (unsigned int z=0; z<nz-1; z++) {
	      for (unsigned int k=0; k<nk; k++) {
		float g=0.25*(I[ipjp]-I[ipjm]+I[ijp]-I[ijm]);
		gz[z]+=g*g;
		I++;
	      }
	    }
	    I+=ip-nk;
	  }
	  /* compute diffusivity from summed gradient magnitude */
	  for (unsigned int z=0; z<nz-1; z++) {
	    gz[z]=1.0/sqrt(gz[z]+epssqr); /* total variation */
	  }
	  /* create tridiagonal system matrix from diffusivities */
	  az[0]=0; cz[nz-1]=0;
	  for (unsigned int z=0; z<nz-1; z++) {
	    az[z+1]=cz[z]=-Dtau*gz[z];
	    bz[z]=1-az[z]-cz[z];
	  }
	  bz[nz-1]=1-az[nz-1];
	  /* solve using tridiagonal matrix algorithm; forward step */
	  cz[0]=cz[0]/bz[0];
	  I=indata+nk*(x+y*nx);
	  double *dp=dz;
	  for (unsigned int k=0; k<nk; k++) {
	    *(dp++)=*(I++)/bz[0];
	  }
	  I+=ip-nk;
	  for (unsigned int z=1; z<nz; z++) {
	    double id = 1.0 / (bz[z]-cz[z-1]*az[z]);
	    cz[z]*=id;
	    for (unsigned int k=0; k<nk; k++) {
	      dp[0]=(I[0]-dp[-(int)nk]*az[z])*id;
	      dp++; I++;
	    }
	    I+=ip-nk;
	  }
	  dp=dz+(nz-1)*nk-1;
	  /* backward step */
	  for (int z=nz-2; z>=0; z--) {
	    for (unsigned int k=0; k<nk; k++) {
	      dp[0]+=-cz[z]*dp[nk];
	      dp--;
	    }
	  }
	  /* add contents of dz to output */
	  dp=dz;
	  float *O=outdata+nk*(x+y*nx);
	  for (unsigned int z=0; z<nz; z++) {
	    for (unsigned int k=0; k<nk; k++) {
	      *(O++)+=*(dp++);
	    }
	    O+=ip-nk;
	  }
	}
      }
    }
    /* re-scale */
    float *O=outdata;
    float *I=indata;
    unsigned int items=nk*nx*ny*nz;
    double factor=1.0/D;
    if (nscale!=NULL) {
      dscale=(float*)nscale->data;
      dtime=(float*)ntime->data;
    }
    for (unsigned int i=0; i<items; i++) {
      O[0]*=factor;
      if (nscale!=NULL && itr>=scaleMinItr) {
	double absdiff=fabs(I[0]-O[0]);
	
	if (absdiff>gradient_eps) {
	  dscale[0]+=absdiff;
	  dtime[0]+=tau;
	}
	dscale++; dtime++; I++;
      }
      O++;
    }
    /* switch the roles of indata and outdata */
    float *helper = indata;
    indata = outdata;
    outdata = helper;

#if 1
	bool cancel = false;
	if( callback )
	{
		// Prepare intermediate output data for callback
		nout->data = indata;

		cancel = callback( itr, nout );
	}
	else
	if( callback_with_scale )
	{
		// Prepare intermediate output data for callback
		nout->data = indata;

		// Temporary scale image to apply normalization to
		Nrrd* tmp_scale = nrrdNew();

		  if (nscale!=NULL) { /* compute the inverse scale measure */
			  nrrdCopy( tmp_scale, nscale );
			float* tmp_dscale=(float*)tmp_scale->data;
			float* tmp_dtime=(float*)ntime->data;
			unsigned int items=nk*nx*ny*nz;
			double factor=1.0;
			for (unsigned int i=0; i<items; i++) {
			  if (tmp_dtime[0]>0) {
			tmp_dscale[0]*=factor/tmp_dtime[0];
			  }
			  tmp_dscale++; tmp_dtime++;
			}
		  }

		cancel = callback_with_scale( itr, nout, tmp_scale );

		tmp_scale = nrrdNuke( tmp_scale );
	}
	if( cancel )
		break;
#endif

  } /* end iterations */
  if (nscale!=NULL) { /* compute the inverse scale measure */
    dscale=(float*)nscale->data;
    dtime=(float*)ntime->data;
    unsigned int items=nk*nx*ny*nz;
    double factor=1.0; // Brox uses 1.0/(2.0*D), Strong uses 1.0
    for (unsigned int i=0; i<items; i++) {
      if (dtime[0]>0) {
	dscale[0]*=factor/dtime[0];
      }
      dscale++; dtime++;
    }
  }
  free(gx); free(ax); free(bx); free(cx); free(dx);
  free(gy); free(ay); free(by); free(cy); free(dy);
  free(gz); free(az); free(bz); free(cz); free(dz);
  /* make sure nout has the correct data pointer */
  nout->data = indata;
  nbuf->data = outdata;
  nbuf=nrrdNuke(nbuf);
  ntime=nrrdNuke(ntime);
  return 0;
}

/* finds a structure tensor field for a 2D grayscale image, using
 * central differences
 * makes the same assumptions as above - square pixels, unit distance */
int
sqrSten2d(Nrrd *nout, Nrrd *nin)
{
  if (nin==NULL || nout==NULL || nin->dim!=2 || nin->type!=nrrdTypeFloat)
    return 1;
  size_t nx = nin->axis[0].size;
  size_t ny = nin->axis[1].size;
  /* Allocate space for output */
  nrrdAlloc_va(nout, nrrdTypeFloat, 3, (size_t) 3, nx, ny);
  float *I = (float*) nin->data;
  float *S = (float*) nout->data;
  for (unsigned int y=0; y<ny; y++) {
    for (unsigned int x=0; x<nx; x++) {
      double g[2]={0,0};
      /* use central differences to compute gradient */
      if (x!=0 && x!=nx-1) {
	g[0]=0.5*(I[1]-I[-1]);
      }
      if (y!=0 && y!=ny-1) {
	g[1]=0.5*(I[nx]-I[-(int)nx]);
      }
      I++;
      double gm=sqrt(g[0]*g[0]+g[1]*g[1]);
      if (gm>0) {
	gm=1.0/gm;
	S[0]=gm*g[0]*g[0];
	S[1]=gm*g[0]*g[1];
	S[2]=gm*g[1]*g[1];
      } else {
	ELL_3V_SET(S,0,0,0);
      }
      S+=3;
    }
  }
  return 0;
}

/* finds a structure tensor field for a 3D grayscale image, using
 * central differences
 * makes the same assumptions as above - square pixels, unit distance */
int
sqrSten3d(Nrrd *nout, Nrrd *nin)
{
  if (nin==NULL || nout==NULL || nin->dim!=3 || nin->type!=nrrdTypeFloat)
    return 1;
  size_t nx = nin->axis[0].size;
  size_t ny = nin->axis[1].size;
  size_t nz = nin->axis[2].size;
  /* Allocate space for output */
  nrrdAlloc_va(nout, nrrdTypeFloat, 4, (size_t) 6, nx, ny, nz);
  float *I = (float*) nin->data;
  float *S = (float*) nout->data;
  for (unsigned int z=0; z<nz; z++) {
    for (unsigned int y=0; y<ny; y++) {
      for (unsigned int x=0; x<nx; x++) {
	double g[3]={0,0,0};
	/* use central differences to compute gradient */
	if (x!=0 && x!=nx-1) {
	  g[0]=0.5*(I[1]-I[-1]);
	}
	if (y!=0 && y!=ny-1) {
	  g[1]=0.5*(I[nx]-I[-(int)nx]);
	}
	if (z!=0 && z!=nz-1) {
	  g[2]=0.5*(I[nx*ny]-I[-(int)nx*ny]);
	}
	I++;
	double gm=sqrt(g[0]*g[0]+g[1]*g[1]+g[2]*g[2]);
	if (gm>0) {
	  gm=1.0/gm;
	  S[0]=gm*g[0]*g[0];
	  S[1]=gm*g[0]*g[1];
	  S[2]=gm*g[0]*g[2];
	  S[3]=gm*g[1]*g[1];
	  S[4]=gm*g[1]*g[2];
	  S[5]=gm*g[2]*g[2];
	} else {
	  ELL_3V_SET(S,0,0,0);
	  ELL_3V_SET(S+3,0,0,0);
	}
	S+=6;
      }
    }
  }
  return 0;
}
