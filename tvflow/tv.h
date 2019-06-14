#ifndef TV_H_INCLUDED
#define TV_H_INCLUDED
#include <teem/nrrd.h>

/// Convenience wrapper with default gradient_eps
extern int tvFlow(Nrrd *nout, Nrrd *nscale, Nrrd *nin, double tau, double eps,
		  unsigned int maxItr, unsigned int scaleMinItr,
		  bool (*callback)( unsigned itr, Nrrd* nout )=NULL,
	      bool (*callback_with_scale)( unsigned itr, Nrrd* nout, Nrrd* nscale )=NULL );

/// Total variation flow computed via operator splitting
extern int tvFlow(Nrrd *nout, Nrrd *nscale, Nrrd *nin, double tau, double eps,  double gradient_eps,
		  unsigned int maxItr, unsigned int scaleMinItr,
		  bool (*callback)( unsigned itr, Nrrd* nout )=NULL,
	      bool (*callback_with_scale)( unsigned itr, Nrrd* nout, Nrrd* nscale )=NULL );
extern int sqrSten2d(Nrrd *nout, Nrrd *nin);
extern int sqrSten3d(Nrrd *nout, Nrrd *nin);
#endif
