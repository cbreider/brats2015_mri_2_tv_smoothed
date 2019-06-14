/* Simplistic command-line interface to the TV flow code
 * Copyright: Thomas Schultz, 2010
 */

#include <stdio.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <teem/hest.h>
#include "tv.h"

void load_and_exit_on_error(Nrrd *nrrd, const char *filename) {
  if (nrrdLoad(nrrd, filename, NULL)) {
    char *err;
    fprintf(stderr, "ERROR: Could not open file \"%s\"\n"
	    "Reason: %s\n",
	    filename, err = biffGetDone(NRRD));
    free(err);
    exit(1);
  }
}

void save_and_exit_on_error(const char *filename, Nrrd *nrrd) {
  if (nrrdSave(filename, nrrd, NULL)) {
    char *err=biffGetDone(NRRD);
    fprintf(stderr, "ERROR: Could not write \"%s\"\n"
	    "Reason: %s\n", filename, err);
    free(err); exit(1);
  }
}


// Use global variables such that they can also be accesses in callback function
char *infilename, *outfilename, *sclfilename, *err;
unsigned int maxitr, scaleMinItr;
double tau, eps, gradeps;

bool tvflow_callback( unsigned itr, Nrrd* nout, Nrrd* nscale )
{
	using namespace std;

	cout << "Iteration " << itr << "\r";

#if 0 // DEBUG OUTPUTS OF INTERMEDIATE RESULTS
	stringstream name, scname, scount, sopts;
	sopts << "smi" << scaleMinItr << "_tau" << tau << "_eps" << eps;
	scount << setfill('0') << setw(4) << itr;
	name   << string("tvflow-")  << sopts.str() << "-" << scount.str();
	scname << string("tvscale-") << sopts.str() << "-" << scount.str();

  // write nrrds
  using std::string;
  string ofname = name.str() + string(".nrrd");
  save_and_exit_on_error(ofname.c_str(), nout);  
  bool has_valid_scale_image = (nscale!=NULL && nscale->axis[0].size>0 && nscale->axis[1].size>0 && nscale->axis[2].size>0);
  if( has_valid_scale_image )  {
	string sclfname = scname.str() + string(".nrrd");
    save_and_exit_on_error(sclfname.c_str(), nscale);
  }
#endif

  return false;
}

int main(int argc, const char **argv) {
  const char *ourname = *argv;
  hestOpt opt[] = {
    {"e", "eps", airTypeDouble, 1, 1, &eps, "0.001", "regularization eps"},
    {"i", "itr", airTypeUInt, 1, 1, &maxitr, "10", "maximum # iterations"},
    {"g", "gradeps", airTypeDouble, 1, 1, &gradeps, "0.001", "gradient eps"},
    {"s", "scl", airTypeString, 1, 1, &sclfilename, "", "scale output file"},
    {"smi", "sclMinItr", airTypeUInt, 1, 1, &scaleMinItr, "0",
     "iteration # to start measurement"},
    {"t", "tau", airTypeDouble, 1, 1, &tau, "1.0", "stepsize tau"},
    {NULL, "in", airTypeString, 1, 1, &infilename, NULL, "image input file"},
    {NULL, "out", airTypeString, 1, 1, &outfilename, NULL,"smooth output file"},
    {NULL, NULL, 0}
  };
  
  /* parse command line */
  if (hestParse(opt, argc-1, argv+1, &err, NULL)) {
    fprintf(stderr, "ERROR: %s\n", err);
    hestUsage(stderr, opt, ourname, NULL);
    hestGlossary(stderr, opt, NULL);
    free(err); exit(1);
  }

  if (infilename==NULL || outfilename==NULL ||
      *infilename==0 || *outfilename==0) {
    fprintf(stderr, "ERROR: Please specify two filenames.\n");
    hestUsage(stderr, opt, ourname, NULL);
    hestGlossary(stderr, opt, NULL);
    exit(1);
  }
  
  Nrrd *nin = nrrdNew(), *nout=nrrdNew(), *nscale=NULL;
  load_and_exit_on_error(nin, infilename);
  if (nin->type!=nrrdTypeFloat) {
    Nrrd *nfloat=nrrdNew();
    nrrdConvert(nfloat, nin, nrrdTypeFloat);
    nin=nrrdNuke(nin);
    nin=nfloat;
  }
  if (sclfilename!=NULL && *sclfilename!=0) {
    nscale=nrrdNew();
  }
  if (tvFlow(nout, nscale, nin, tau, eps, gradeps, maxitr, scaleMinItr, NULL, tvflow_callback)) {

    fprintf(stderr, "\nERROR: tvFlow failed.\n");
  }
  std::cout << std::endl;
  // write nrrds
  using std::string;
  string ofname = string(outfilename) + string(".nrrd");
  save_and_exit_on_error(ofname.c_str(), nout);
  if (nscale!=NULL) {
	string sclfname = string(sclfilename) + string(".nrrd");
    save_and_exit_on_error(sclfname.c_str(), nscale);
  }
}
