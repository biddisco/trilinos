// @HEADER
//
// ***********************************************************************
//
//   Zoltan2: A package of combinatorial algorithms for scientific computing
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Karen Devine      (kddevin@sandia.gov)
//                    Erik Boman        (egboman@sandia.gov)
//                    Siva Rajamanickam (srajama@sandia.gov)
//
// ***********************************************************************
//
// @HEADER

/*! \file zoltanCompare.cpp
 *  Compares zoltan execution through Zoltan2 with direct zoltan execution
*/

#include <Zoltan2_TestHelpers.hpp>
#include <Zoltan2_XpetraCrsMatrixAdapter.hpp>
#include <Zoltan2_XpetraMultiVectorAdapter.hpp>
#include <Zoltan2_PartitioningSolution.hpp>
#include <Zoltan2_PartitioningProblem.hpp>

#include <Tpetra_MultiVector.hpp>

using namespace std;
using Teuchos::RCP;
using Teuchos::rcp;
using Teuchos::Comm;

//
// A few of the tests done by Zoltan in nightly testing.
//

enum testFields {
  TESTNAMEOFFSET = 0,
  TESTMETHODOFFSET,
  TESTOBJWGTOFFSET,
  TESTNUMPROCS,
  TESTNUMARGS
};


#define NUMTESTS 24
static string testArgs[] = {
// Filename  LB_Method   ObjWeightDim   NumProcs
"simple",       "rcb",          "0",      "2",
"vwgt2",        "rcb",          "2",      "2",

"bug",          "rcb",          "1",      "3",
"drake",        "rcb",          "0",      "3",
"onedbug",      "rcb",          "0",      "3",
"simple",       "rcb",          "0",      "3",
"vwgt",         "rcb",          "1",      "3",
"vwgt2",        "rcb",          "2",      "3",

"simple",       "default",      "0",      "4",
"ewgt",         "hsfc",         "0",      "4",
"grid20x19",    "hsfc",         "0",      "4",
"grid20x19",    "hsfc",         "0",      "4",
"grid20x19",    "hsfc",         "0",      "4",
"nograph",      "rib",          "0",      "4",
"simple",       "rib",          "0",      "4",
"simple",       "rib",          "0",      "4",
"vwgt2",        "rib",          "2",      "4",

"brack2_3",     "rcb",          "3",      "5",

"hammond2",     "rcb",          "2",      "6",
"degenerateAA", "rcb",          "0",      "6",
"degenerate",   "rcb",          "0",      "6",
"degenerate",   "rcb",          "0",      "6",

"hammond",      "rcb",          "0",      "8",
"vwgt2",        "rcb",          "2",      "8"
};

typedef Tpetra::CrsMatrix<zscalar_t, zlno_t, zgno_t, znode_t> tMatrix_t;
typedef Tpetra::MultiVector<zscalar_t, zlno_t, zgno_t, znode_t> tMVector_t;
typedef Zoltan2::XpetraMultiVectorAdapter<tMVector_t> vectorAdapter_t;
typedef Zoltan2::XpetraCrsMatrixAdapter<tMatrix_t,tMVector_t> matrixAdapter_t;

////////////////////////////////////////////////////////////////////////////////
// Zoltan callbacks

static int znumobj(void *data, int *ierr) 
{
  *ierr = ZOLTAN_OK;
  tMVector_t *vec = (tMVector_t *) data;
  return vec->getLocalLength();
}

static void zobjlist(void *data, int ngid, int nlid, 
                     ZOLTAN_ID_PTR gids, ZOLTAN_ID_PTR lids,
                     int nwgts, float *wgts, int *ierr)
{
  *ierr = ZOLTAN_OK;
  tMVector_t *vec = (tMVector_t *) data;
  int n = vec->getLocalLength();
  for (int i = 0; i < n; i++) {
    gids[i] = vec->getMap()->getGlobalElement(i);
    lids[i] = i;
  }
  for (int w = 0; w < nwgts; w++) {
    ArrayRCP<const zscalar_t> wvec = vec->getData(w);
    for (int i = 0; i < n; i++)
      wgts[i*nwgts+w] = wvec[i];
  }
}

static int znumgeom(void *data, int *ierr) 
{
  *ierr = ZOLTAN_OK;
  tMVector_t *cvec = (tMVector_t *) data;
  return cvec->getNumVectors();
}

static void zgeom(void *data, int ngid, int nlid, int nobj, 
                  ZOLTAN_ID_PTR gids, ZOLTAN_ID_PTR lids,
                  int ndim, double *coords, int *ierr)
{
  *ierr = ZOLTAN_OK;
  tMVector_t *vec = (tMVector_t *) data;
  for (int d = 0; d < ndim; d++) {
    ArrayRCP<const zscalar_t> cvec = vec->getData(d);
    for (int i = 0; i < nobj; i++) {
      coords[lids[i]*ndim+d] = cvec[lids[i]];
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// Function to compute both Zoltan2 and Zoltan partitions and print metrics

int run(
  const RCP<const Comm<int> > &comm,
  int numGlobalParts,
  int testCnt,
  std::string *thisTest
)
{
#ifdef HAVE_ZOLTAN2_MPI
  // Zoltan needs an MPI comm
  const Teuchos::MpiComm<int> *tmpicomm =
               dynamic_cast<const Teuchos::MpiComm<int> *>(comm.getRawPtr());
  MPI_Comm mpiComm = *(tmpicomm->getRawMpiComm());
#endif

  int me = comm->getRank();
  int np = comm->getSize();
  double tolerance = 1.05;

  //////////////////////////////////////////////
  // Read test data from Zoltan's test directory
  //////////////////////////////////////////////

  UserInputForTests *uinput;
  try{
    uinput = new UserInputForTests(zoltanTestDirectory,
                                   thisTest[TESTNAMEOFFSET],
                                   comm, true);
  }
  catch(std::exception &e){
    if (me == 0)
      cout << "Test " << testCnt << ":  FAIL: UserInputForTests "
           << e.what() << endl;
    return 1;
  }

  RCP<tMatrix_t> matrix;
  try{
    matrix = uinput->getUITpetraCrsMatrix();
  }
  catch(std::exception &e){
    if (me == 0)
      cout << "Test " << testCnt << ":  FAIL: get matrix "
           << e.what() << endl;
    return 1;
  }

  RCP<const tMatrix_t> matrixConst = rcp_const_cast<const tMatrix_t>(matrix);

  RCP<tMVector_t> coords;
  try{
   coords = uinput->getUICoordinates();
  }
  catch(std::exception &e){
    if (me == 0)
      cout << "Test " << testCnt << ":  FAIL: get coordinates "
           << e.what() << endl;
    return 1;
  }

  RCP<tMVector_t> weights;
  try{
   weights = uinput->getUIWeights();
  }
  catch(std::exception &e){
    if (me == 0)
      cout << "Test " << testCnt << ":  FAIL: get weights "
           << e.what() << endl;
    return 1;
  }
  int nWeights = atoi(thisTest[TESTOBJWGTOFFSET].c_str());

  if (me == 0) {
    cout << "Test " << testCnt << " filename            = "
         << thisTest[TESTNAMEOFFSET] << endl;
    cout << "Test " << testCnt << " num processors      = "
         << np << endl;
    cout << "Test " << testCnt << " zoltan method       = "
         << thisTest[TESTMETHODOFFSET] << endl;
    cout << "Test " << testCnt << " num_global_parts    = "
         << numGlobalParts << endl;
    cout << "Test " << testCnt << " imbalance_tolerance = "
         << tolerance << endl;
    cout << "Test " << testCnt << " num weights per ID  = "
         << nWeights << endl;
  }

  /////////////////////////////////////////
  // PARTITION USING ZOLTAN DIRECTLY
  /////////////////////////////////////////

  if (me == 0) cout << "Calling Zoltan directly" << endl;

# ifdef HAVE_ZOLTAN2_MPI
    Zoltan zz(mpiComm);
# else
    Zoltan zz;
# endif

  char tmp[56];
  zz.Set_Param("LB_METHOD", thisTest[TESTMETHODOFFSET]);
  
  sprintf(tmp, "%d", numGlobalParts);
  zz.Set_Param("NUM_GLOBAL_PARTS", tmp);
  sprintf(tmp, "%d", nWeights);
  zz.Set_Param("OBJ_WEIGHT_DIM", tmp);
  sprintf(tmp, "%f", tolerance);
  zz.Set_Param("IMBALANCE_TOL", tmp);
  zz.Set_Param("RETURN_LISTS", "PART");
  zz.Set_Param("FINAL_OUTPUT", "1");

  zz.Set_Num_Obj_Fn(znumobj, (void *) coords.getRawPtr());
  if (nWeights)
    zz.Set_Obj_List_Fn(zobjlist, (void *) weights.getRawPtr());
  else
    zz.Set_Obj_List_Fn(zobjlist, (void *) coords.getRawPtr());
  zz.Set_Num_Geom_Fn(znumgeom, (void *) coords.getRawPtr());
  zz.Set_Geom_Multi_Fn(zgeom, (void *) coords.getRawPtr());

  int changes, ngid, nlid;
  int numd, nump;
  ZOLTAN_ID_PTR dgid = NULL, dlid = NULL, pgid = NULL, plid = NULL;
  int *dproc = NULL, *dpart = NULL, *pproc = NULL, *ppart = NULL;

  int ierr = zz.LB_Partition(changes, ngid, nlid,
                             numd, dgid, dlid, dproc, dpart,
                             nump, pgid, plid, pproc, ppart);
  if (ierr != ZOLTAN_OK && ierr != ZOLTAN_WARN) {
    if (me == 0)
      cout << "Test " << testCnt << ":  FAIL: direct Zoltan call" << endl;
    zz.LB_Free_Part(&pgid, &plid, &pproc, &ppart);
    return 1;
  }

  /////////////////////////////////////////
  // PARTITION USING ZOLTAN THROUGH ZOLTAN2
  /////////////////////////////////////////

  if (me == 0) cout << "Calling Zoltan through Zoltan2" << endl;

  matrixAdapter_t *ia;
  try{
    ia = new matrixAdapter_t(matrixConst, nWeights);
  }
  catch(std::exception &e){
    if (me == 0)
      cout << "Test " << testCnt << ":  FAIL: matrix adapter "
           << e.what() << endl;
    return 1;
  }
  for (int idx=0; idx < nWeights; idx++)
    ia->setRowWeights(weights->getData(idx).getRawPtr(), 1, idx);

  vectorAdapter_t *ca = NULL;
  try{
    ca = new vectorAdapter_t(coords);
  }
  catch(std::exception &e){
    if (me == 0)
      cout << "Test " << testCnt << ":  FAIL: vector adapter "
           << e.what() << endl;
    return 1;
  }
  ia->setCoordinateInput(ca);
  
  Teuchos::ParameterList params;
  params.set("timer_output_stream" , "std::cout");
  params.set("compute_metrics", "true");
  // params.set("debug_level" , "verbose_detailed_status");

  params.set("algorithm", "zoltan");
  params.set("imbalance_tolerance", tolerance );
  params.set("num_global_parts", numGlobalParts);

  if (thisTest[TESTMETHODOFFSET] != "default") {
    // "default" tests case of no Zoltan parameter sublist
    Teuchos::ParameterList &zparams = params.sublist("zoltan_parameters",false);
    zparams.set("LB_METHOD",thisTest[TESTMETHODOFFSET]);
  }

  Zoltan2::PartitioningProblem<matrixAdapter_t> *problem;
# ifdef HAVE_ZOLTAN2_MPI
    try{
      problem = new Zoltan2::PartitioningProblem<matrixAdapter_t>(ia, &params,
                                                                  mpiComm);
    }
# else
    try{
      problem = new Zoltan2::PartitioningProblem<matrixAdapter_t>(ia, &params);
    }
# endif
  catch(std::exception &e){
    cout << "Test " << testCnt << " FAIL: problem " << e.what() << endl;
    return 1;
  }

  try {
    problem->solve();
  }
  catch(std::exception &e){
    cout << "Test " << testCnt << " FAIL: solve " << e.what() << endl;
    return 1;
  }

  if (me == 0){
    problem->printMetrics(cout);
  }
  problem->printTimers();

  /////////////////////////////////////////
  // COMPARE RESULTS
  /////////////////////////////////////////
  size_t nObj = coords->getLocalLength();
  const int *z2parts = problem->getSolution().getPartListView();
  int diffcnt = 0, gdiffcnt = 0;
  for (size_t i = 0; i < nObj; i++) {
    if (z2parts[plid[i]] != ppart[i]) {
      diffcnt++;
      cout << me << " DIFF for " << i << " (" 
           << coords->getMap()->getGlobalElement(i) << "):  "
           << "Z2 = " << z2parts[i] << "; Z1 = " << ppart[plid[i]] << endl;
    }
  }

  /////////////////////////////////////////
  // CLEAN UP
  /////////////////////////////////////////
  zz.LB_Free_Part(&pgid, &plid, &pproc, &ppart);
  delete ia;
  delete ca;
  delete problem;
  delete uinput;

  Teuchos::reduceAll(*comm, Teuchos::REDUCE_SUM, 1, &diffcnt, &gdiffcnt);
  if (gdiffcnt > 0) {
    if (me == 0) 
      cout << "Test " << testCnt << " "
           << thisTest[TESTNAMEOFFSET] << " "
           << thisTest[TESTMETHODOFFSET] << " "
           << thisTest[TESTOBJWGTOFFSET] << " "
           << " FAIL: comparison " << endl;
    return 1;
  }

  return 0;
}
  
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  Teuchos::GlobalMPISession session(&argc, &argv);
  RCP<const Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();

  int me = comm->getRank();
  int np = comm->getSize();

  int fail=0;

  Array<int> ranks(np);
  for (int i = 0; i < np; i++) ranks[i] = i;

  for (int i=0; i < NUMTESTS; i++) {
    std::string *thisTest = &(testArgs[i*TESTNUMARGS]);
    int nTestProcs = atoi(thisTest[TESTNUMPROCS].c_str());
    if (nTestProcs > np) {
      if (me == 0) {
        cout << "Skipping test " << i << " on "
             << thisTest[TESTNAMEOFFSET]
             << "; required number of procs " << nTestProcs 
             << " is greater than available procs " << np << endl;
      }
      continue;
    }

    // Make a communicator of appropriate size for the test
    RCP<const Comm<int> > testcomm;
    if (nTestProcs == np)
      testcomm = comm;
    else
      testcomm = comm->createSubcommunicator(ranks.view(0,nTestProcs));

    // Run the test if in the communicator
    if (me < nTestProcs) {
      fail += run(testcomm, nTestProcs, i, thisTest);
    }
  }
  
  if (me == 0 && !fail)
    cout << "PASS" << endl;
  
  return 0;
}

