/*
//@HEADER
// ***********************************************************************
//
//       Ifpack2: Tempated Object-Oriented Algebraic Preconditioner Package
//                 Copyright (2009) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
//@HEADER
*/

// ***********************************************************************
//
//      Ifpack2: Tempated Object-Oriented Algebraic Preconditioner Package
//                 Copyright (2004) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************


/*! \file Ifpack2_UnitTestRelaxation.cpp

\brief Ifpack2 Unit test for the Relaxation template.
*/


#include <Teuchos_ConfigDefs.hpp>
#include <Ifpack2_ConfigDefs.hpp>
#include <Teuchos_UnitTestHarness.hpp>
#include <Ifpack2_Version.hpp>
#include <iostream>

#include <Ifpack2_UnitTestHelpers.hpp>
#include <Ifpack2_BlockRelaxation.hpp>
#include <Ifpack2_SparseContainer.hpp>
#include <Ifpack2_TriDiContainer.hpp>
#include <Ifpack2_BandedContainer.hpp>
#include <Ifpack2_DenseContainer.hpp>
#include <Ifpack2_OverlappingPartitioner.hpp>
#include <Ifpack2_LinearPartitioner.hpp>
#include <Ifpack2_LinePartitioner.hpp>
#include <Ifpack2_ILUT.hpp>

#include <Tpetra_RowMatrix.hpp>
#include <Tpetra_Experimental_BlockMultiVector.hpp>
#include <Tpetra_Experimental_BlockCrsMatrix.hpp>

namespace {
using Tpetra::global_size_t;
typedef tif_utest::Node Node;

//this macro declares the unit-test-class:
TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, Test0, Scalar, LocalOrdinal, GlobalOrdinal)
{
//we are now in a class method declared by the above macro, and
//that method has these input arguments:
//Teuchos::FancyOStream& out, bool& success

  std::string version = Ifpack2::Version();
  out << "Ifpack2::Version(): " << version << std::endl;

  global_size_t num_rows_per_proc = 5;

  typedef Tpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> CRS;
  typedef Tpetra::RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> ROW;
  typedef Ifpack2::ILUT< Tpetra::RowMatrix<Scalar,LocalOrdinal,LocalOrdinal,Node>    > ILUTlo;

  const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > rowmap =
    tif_utest::create_tpetra_map<LocalOrdinal,GlobalOrdinal,Node>(num_rows_per_proc);
  Teuchos::RCP<const CRS> crsmatrix = tif_utest::create_test_matrix<Scalar,LocalOrdinal,GlobalOrdinal,Node>(rowmap);
  Ifpack2::BlockRelaxation<ROW, Ifpack2::SparseContainer<ROW, ILUTlo> > prec(crsmatrix);

  Teuchos::ParameterList params;
  params.set("relaxation: type", "Jacobi");
  params.set("partitioner: type","linear");
  params.set("partitioner: local parts",(int)num_rows_per_proc);

  TEST_NOTHROW(prec.setParameters(params));

  //trivial tests to insist that the preconditioner's domain/range maps are
  //identically those of the matrix:
  const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node>* mtx_dom_map_ptr = &*crsmatrix->getDomainMap();
  const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node>* mtx_rng_map_ptr = &*crsmatrix->getRangeMap();

  const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node>* prec_dom_map_ptr = &*prec.getDomainMap();
  const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node>* prec_rng_map_ptr = &*prec.getRangeMap();

  TEST_EQUALITY( prec_dom_map_ptr, mtx_dom_map_ptr);
  TEST_EQUALITY( prec_rng_map_ptr, mtx_rng_map_ptr);

  prec.initialize();
  prec.compute();

  Tpetra::MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Node> x(rowmap,2), y(rowmap,2);
  x.putScalar(1);

  prec.applyMat(x, y);

  Teuchos::ArrayRCP<const Scalar> yview = y.get1dView();

  //Since crsmatrix is a diagonal matrix with 2 on the diagonal,
  //y should be full of 2's now.

  Teuchos::ArrayRCP<Scalar> twos(num_rows_per_proc*2, 2);

  TEST_COMPARE_FLOATING_ARRAYS(yview, twos(), Teuchos::ScalarTraits<Scalar>::eps());

  prec.apply(x, y);

  //y should be full of 0.5's now.

  Teuchos::ArrayRCP<Scalar> halfs(num_rows_per_proc*2, 0.5);

  TEST_COMPARE_FLOATING_ARRAYS(yview, halfs(), Teuchos::ScalarTraits<Scalar>::eps());
}

// Test apply() with x == y.
// When x == y, apply() need to create internally an auxiliary vector, Xcopy.
TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, Test1, Scalar, LocalOrdinal, GlobalOrdinal)
{
  std::string version = Ifpack2::Version();
  out << "Ifpack2::Version(): " << version << std::endl;

  global_size_t num_rows_per_proc = 5;

  typedef Tpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> CRS;
  typedef Tpetra::RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> ROW;
  typedef Ifpack2::ILUT<ROW> ILUTlo;

  const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > rowmap = tif_utest::create_tpetra_map<LocalOrdinal,GlobalOrdinal,Node>(num_rows_per_proc);
  Teuchos::RCP<const CRS> crsmatrix = tif_utest::create_test_matrix<Scalar,LocalOrdinal,GlobalOrdinal,Node>(rowmap);
  Ifpack2::BlockRelaxation<ROW, Ifpack2::SparseContainer<ROW, ILUTlo> > prec(crsmatrix);

  Teuchos::ParameterList params;
  params.set("relaxation: type", "Jacobi");
  params.set("partitioner: type","linear");
  params.set("partitioner: local parts",(int)num_rows_per_proc);
  prec.setParameters(params);

  prec.initialize();
  prec.compute();

  Tpetra::MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Node> x(rowmap,2);
  x.putScalar(1);

  prec.apply(x, x);

  //y should be full of 0.5's now.
  {
    Teuchos::ArrayRCP<const Scalar> xview = x.get1dView();
    Teuchos::ArrayRCP<Scalar> halfs(num_rows_per_proc*2, 0.5);
    TEST_COMPARE_FLOATING_ARRAYS(xview, halfs(), Teuchos::ScalarTraits<Scalar>::eps());
  }
}

// Test apply() with x != y  but x and y pointing to the same memory location.
// It is possible to create two different Tpetra vectors pointing to the same memory location by using MultiVector::subView().
// I can't imagine anyone trying to do this but... in this case, apply() need also to create internally an auxiliary vector, Xcopy.
TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, Test2, Scalar, LO, GO)
{
  typedef Tpetra::Map<LO,GO,Node> map_type;
  typedef Tpetra::CrsMatrix<Scalar,LO,GO,Node> CRS;
  typedef Tpetra::RowMatrix<Scalar,LO,GO,Node> ROW;
  typedef Tpetra::MultiVector<Scalar,LO,GO,Node> MV;
  typedef Ifpack2::ILUT<ROW> ILUTlo;
  using Teuchos::RCP;
  using std::endl;

  out << "Ifpack2::BlockRelaxation Test2" << endl;

  const Scalar one = Teuchos::ScalarTraits<Scalar>::one ();
  const Scalar two = one + one;

  global_size_t num_rows_per_proc = 5;
  RCP<const map_type> rowmap = tif_utest::create_tpetra_map<LO,GO,Node> (num_rows_per_proc);
  RCP<const CRS> crsmatrix = tif_utest::create_test_matrix<Scalar,LO,GO,Node> (rowmap);
  Ifpack2::BlockRelaxation<ROW, Ifpack2::SparseContainer<ROW, ILUTlo> > prec (crsmatrix);

  Teuchos::ParameterList params;
  params.set("relaxation: type", "Jacobi");
  params.set("partitioner: type","linear");
  params.set("partitioner: local parts",(int)num_rows_per_proc);
  prec.setParameters(params);

  prec.initialize();
  prec.compute();

  MV y (rowmap,2);
  y.putScalar (one);
  RCP<MV> xrcp = y.subViewNonConst (Teuchos::Range1D (0, 1));
  MV x = *xrcp;

  TEST_INEQUALITY(&x, &y);                                               // vector x and y are different
  x.template sync<Kokkos::HostSpace> ();
  y.template sync<Kokkos::HostSpace> ();
  auto x_lcl_host = x.template getLocalView<Kokkos::HostSpace> ();
  auto y_lcl_host = x.template getLocalView<Kokkos::HostSpace> ();
  TEST_EQUALITY( x_lcl_host.ptr_on_device (), y_lcl_host.ptr_on_device () ); // vector x and y are pointing to the same memory location (such test only works if num of local elements != 0)

  prec.apply(x, y);

  //y should be full of 0.5's now.
  {
    MV y_diff (y.getMap (), y.getNumVectors ());
    y_diff.putScalar (one / two);

    typedef typename MV::device_type device_type;
    typedef typename MV::mag_type mag_type;
    typedef Kokkos::View<mag_type*, device_type> norms_type;
    norms_type y_norms ("y_norms", y.getNumVectors ());

    y_diff.update (one, y, -one);
    y_diff.normInf (y_norms);

    auto y_norms_host = Kokkos::create_mirror_view (y_norms);
    Kokkos::deep_copy (y_norms_host, y_norms);

    for (size_t j = 0; j < y.getNumVectors (); ++j) {
      TEST_EQUALITY( y_norms_host(0), Kokkos::Details::ArithTraits<mag_type>::zero () );
    }
  }
}

// Note: Test3 from Ifpack2Relaxation test has been removed.
// Test apply() with "null" x and y. In parallel, it is possible that some nodes do not have any local elements.
// This never worked in Ifpack and won't work here either.

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, TriDi, Scalar, LocalOrdinal, GlobalOrdinal)
{
  std::string version = Ifpack2::Version();
  out << "Ifpack2::Version(): " << version << std::endl;

  global_size_t num_rows_per_proc = 5;

  typedef Tpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> CRS;
  typedef Tpetra::RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> ROW;
  typedef Tpetra::Vector<Scalar,LocalOrdinal,GlobalOrdinal,Node> Vector;
  typedef Ifpack2::TriDiContainer<ROW,Scalar> TriDi;

  const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > rowmap = tif_utest::create_tpetra_map<LocalOrdinal,GlobalOrdinal,Node>(num_rows_per_proc);
  Teuchos::RCP<const CRS > crsmatrix = tif_utest::create_test_matrix_variable_blocking<Scalar,LocalOrdinal,GlobalOrdinal,Node>(rowmap);

  /*** Fill RHS / LHS ***/
  Vector rhs(rowmap), lhs(rowmap), exact_soln(rowmap);
  rhs.putScalar(2.0);
  lhs.putScalar(0.0);
  exact_soln.putScalar(2.0);

  /* Setup Block Relaxation */
  int PartMap[5]={0,0,1,1,1};
  Teuchos::ArrayRCP<int> PMrcp(&PartMap[0],0,5,false);

  Teuchos::ParameterList ilist;
  ilist.set("partitioner: type","user");
  ilist.set("partitioner: map",PMrcp);
  ilist.set("partitioner: local parts",2);
  ilist.set("relaxation: sweeps",1);
  ilist.set("relaxation: type","Gauss-Seidel");

  Ifpack2::BlockRelaxation<ROW,TriDi> TDRelax(crsmatrix);

  TDRelax.setParameters(ilist);
  TDRelax.initialize();
  TDRelax.compute();
  TDRelax.apply(rhs,lhs);

  TEST_COMPARE_FLOATING_ARRAYS(lhs.get1dView(), exact_soln.get1dView(), 2*Teuchos::ScalarTraits<Scalar>::eps());
}

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, BandedContainer, Scalar, LocalOrdinal, GlobalOrdinal)
{
  std::string version = Ifpack2::Version();
  out << "Ifpack2::Version(): " << version << std::endl;

  global_size_t num_rows_per_proc = 5;

  typedef Tpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> CRS;
  typedef Tpetra::RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> ROW;
  typedef Tpetra::Vector<Scalar,LocalOrdinal,GlobalOrdinal,Node> Vector;
  typedef Ifpack2::BandedContainer<ROW, Scalar> BandedContainer;

  const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > rowmap = tif_utest::create_tpetra_map<LocalOrdinal,GlobalOrdinal,Node>(num_rows_per_proc);
  Teuchos::RCP<const CRS > crsmatrix = tif_utest::create_test_matrix_variable_banded<Scalar,LocalOrdinal,GlobalOrdinal,Node>(rowmap);

  /*** Fill RHS / LHS ***/
  Vector rhs(rowmap), lhs(rowmap), exact_soln(rowmap);
  rhs.putScalar(2.0);
  lhs.putScalar(0.0);
  exact_soln.putScalar(2.0);

  /* Setup Block Relaxation */
  int PartMap[5]={0,0,0,0,0};
  Teuchos::ArrayRCP<int> PMrcp(&PartMap[0],0,5,false);

  Teuchos::ParameterList ilist;
  ilist.set("partitioner: type","user");
  ilist.set("partitioner: map",PMrcp);
  ilist.set("partitioner: local parts",1);
  ilist.set("relaxation: sweeps",1);
  ilist.set("relaxation: type","Gauss-Seidel");

  Ifpack2::BlockRelaxation<ROW, BandedContainer> TDRelax(crsmatrix);

  TDRelax.setParameters(ilist);
  TDRelax.initialize();
  TDRelax.compute();
  TDRelax.apply(rhs,lhs);

  TEST_COMPARE_FLOATING_ARRAYS(lhs.get1dView(), exact_soln.get1dView(), 2*Teuchos::ScalarTraits<Scalar>::eps());
}


TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, BlockedBandedContainer, Scalar, LocalOrdinal, GlobalOrdinal)
{
  std::string version = Ifpack2::Version();
  out << "Ifpack2::Version(): " << version << std::endl;

  global_size_t num_rows_per_proc = 5;

  typedef Tpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> CRS;
  typedef Tpetra::RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> ROW;
  typedef Tpetra::Vector<Scalar,LocalOrdinal,GlobalOrdinal,Node> Vector;
  typedef Ifpack2::BandedContainer<ROW, Scalar> BandedContainer;

  const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > rowmap = tif_utest::create_tpetra_map<LocalOrdinal,GlobalOrdinal,Node>(num_rows_per_proc);
  Teuchos::RCP<const CRS > crsmatrix = tif_utest::create_test_matrix_banded_variable_blocking<Scalar,LocalOrdinal,GlobalOrdinal,Node>(rowmap);

  /*** Fill RHS / LHS ***/
  Vector rhs(rowmap), lhs(rowmap), exact_soln(rowmap);
  rhs.putScalar(2.0);
  lhs.putScalar(0.0);
  exact_soln.putScalar(2.0);

  /* Setup Block Relaxation */
  int PartMap[5]={0,0,1,1,1};
  Teuchos::ArrayRCP<int> PMrcp(&PartMap[0],0,5,false);

  Teuchos::ParameterList ilist;
  ilist.set("partitioner: type","user");
  ilist.set("partitioner: map",PMrcp);
  ilist.set("partitioner: local parts",2);
  ilist.set("relaxation: sweeps",1);
  ilist.set("relaxation: type","Gauss-Seidel");

  Ifpack2::BlockRelaxation<ROW, BandedContainer> TDRelax(crsmatrix);

  TDRelax.setParameters(ilist);
  TDRelax.initialize();
  TDRelax.compute();
  TDRelax.apply(rhs,lhs);

  TEST_COMPARE_FLOATING_ARRAYS(lhs.get1dView(), exact_soln.get1dView(), 2*Teuchos::ScalarTraits<Scalar>::eps());
}

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, LinePartition, Scalar, LocalOrdinal, GlobalOrdinal)
{
  std::string version = Ifpack2::Version();
  out << "Ifpack2::Version(): " << version << std::endl;

  global_size_t num_rows_per_proc = 5;

  //typedef Tpetra::RowGraph<LocalOrdinal,GlobalOrdinal,Node> RG;
  typedef Tpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> CRS;
  typedef Tpetra::RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> ROW;
  typedef Tpetra::Vector<Scalar,LocalOrdinal,GlobalOrdinal,Node> Vector;
  typedef Tpetra::MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Node> MultiVector;
  typedef Ifpack2::TriDiContainer<ROW,Scalar> TriDi;

  const Teuchos::RCP<const Tpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > rowmap = tif_utest::create_tpetra_map<LocalOrdinal,GlobalOrdinal,Node>(num_rows_per_proc);
  Teuchos::RCP<const CRS > crsmatrix = tif_utest::create_test_matrix_variable_blocking<Scalar,LocalOrdinal,GlobalOrdinal,Node>(rowmap);

  /*** Fill RHS / LHS ***/
  Vector rhs(rowmap), lhs(rowmap), exact_soln(rowmap);
  rhs.putScalar(2.0);
  lhs.putScalar(0.0);
  exact_soln.putScalar(2.0);

  /* Generate some fake coordinates */
  Teuchos::RCP<MultiVector> coord = rcp(new MultiVector(rowmap,2));
  Teuchos::ArrayRCP<Scalar> x0rcp = coord->getDataNonConst(0);
  Teuchos::ArrayRCP<Scalar> x1rcp = coord->getDataNonConst(1);

  Teuchos::ArrayView<Scalar> x0 = x0rcp();
  Teuchos::ArrayView<Scalar> x1 = x1rcp();
  x0[0]=0; x0[1]=1;   x0[2]=10;  x0[3]=11;  x0[4]=12;
  x1[0]=0; x1[1]=1;   x1[2]=2;   x1[3]=3;   x1[4]=4;


  /* Setup Block Relaxation */
  Teuchos::ParameterList ilist;
  ilist.set("partitioner: type","line");
  ilist.set("partitioner: coordinates",coord);
  ilist.set("partitioner: line detection threshold",0.5);
  ilist.set("relaxation: sweeps",1);
  ilist.set("relaxation: type","Gauss-Seidel");

  Ifpack2::BlockRelaxation<ROW,TriDi> TDRelax(crsmatrix);

  TDRelax.setParameters(ilist);
  TDRelax.initialize();
  TDRelax.compute();
  TDRelax.apply(rhs,lhs);

  // For debugging:
  // Teuchos::RCP<Ifpack2::Partitioner<RG> > MyPart = TDRelax.getPartitioner();
  // Teuchos::ArrayView<const LocalOrdinal> part = MyPart->nonOverlappingPartition();

  TEST_COMPARE_FLOATING_ARRAYS(lhs.get1dView(), exact_soln.get1dView(), 2*Teuchos::ScalarTraits<Scalar>::eps());
}


// Macro used inside the unit test below.  It tests for global error,
// and if so, prints each process' error message and quits the test
// early.
//
// 'out' only prints on Process 0.  It's really not OK for other
// processes to print to stdout, but it usually works and we need to
// do it for debugging.
#define IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( WHAT_STRING ) do { \
  reduceAll<int, int> (*comm, REDUCE_MIN, lclSuccess, outArg (gblSuccess)); \
  TEST_EQUALITY_CONST( gblSuccess, 1 ); \
  if (gblSuccess != 1) { \
    out << WHAT_STRING << " FAILED on one or more processes!" << endl; \
    for (int p = 0; p < numProcs; ++p) { \
      if (myRank == p && lclSuccess != 1) { \
        std::cout << errStrm.str () << std::flush; \
      } \
      comm->barrier (); \
      comm->barrier (); \
      comm->barrier (); \
    } \
    std::cerr << "TEST FAILED; RETURNING EARLY" << endl; \
    return; \
  } \
} while (false)


TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, TestDiagonalBlockCrsMatrix, Scalar, LO, GO)
{
  // Create a block diagonal matrix
  // There are 5 blocks, and each one is [2 0 3; 3 2 0; 0 3 2]
  // Solve the linear system A x = ones(15,1)
  // Solution is 0.2*ones(15,1)
  using Teuchos::outArg;
  using Teuchos::RCP;
  using Teuchos::rcp;
  using Teuchos::REDUCE_MIN;
  using Teuchos::reduceAll;
  using std::endl;
  typedef Tpetra::Experimental::BlockCrsMatrix<Scalar,LO,GO,Node> block_crs_matrix_type;
  typedef Tpetra::Experimental::BlockMultiVector<Scalar,LO,GO,Node> BMV;
  typedef Tpetra::RowMatrix<Scalar,LO,GO,Node> row_matrix_type;
  typedef Tpetra::CrsGraph<LO,GO,Node> crs_graph_type;
  typedef Tpetra::MultiVector<Scalar,LO,GO,Node> MV;
  typedef Ifpack2::BlockRelaxation<row_matrix_type, Ifpack2::DenseContainer<row_matrix_type,double> > prec_type;
  int lclSuccess = 1;
  int gblSuccess = 1;
  std::ostringstream errStrm; // for error collection

  out << "Ifpack2::BlockRelaxation diagonal block matrix test" << endl;

  RCP<const Teuchos::Comm<int> > comm =
    Tpetra::DefaultPlatform::getDefaultPlatform ().getComm ();
  const int myRank = comm->getRank ();
  const int numProcs = comm->getSize ();

  const int num_rows_per_proc = 5;
  const int blockSize = 3;
  RCP<crs_graph_type> crsgraph =
    tif_utest::create_diagonal_graph<LO,GO,Node> (num_rows_per_proc);

  RCP<block_crs_matrix_type> bcrsmatrix;
  bcrsmatrix = Teuchos::rcp_const_cast<block_crs_matrix_type> (tif_utest::create_block_diagonal_matrix<Scalar,LO,GO,Node> (crsgraph, blockSize));
  bcrsmatrix->computeDiagonalGraph ();

  RCP<prec_type> prec;
  try {
    prec = rcp (new prec_type (bcrsmatrix));
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": Preconditioner constructor threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "Preconditioner constructor" );

  Teuchos::ParameterList params;
  params.set ("relaxation: type", "Jacobi");
  params.set ("partitioner: local parts", (LO)bcrsmatrix->getNodeNumRows());
  try {
    prec->setParameters (params);
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": prec->setParameters() threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "prec->setParameters()" );

  try {
    prec->initialize ();
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": prec->initialize() threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "prec->initialize()" );

  try {
    prec->compute ();
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": prec->compute() threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "prec->compute()" );

  BMV xBlock (*crsgraph->getRowMap (), blockSize, 1);
  BMV yBlock (*crsgraph->getRowMap (), blockSize, 1);
  MV x = xBlock.getMultiVectorView ();
  MV y = yBlock.getMultiVectorView ();
  x.putScalar (Teuchos::ScalarTraits<Scalar>::one ());

  TEST_EQUALITY(x.getMap()->getNodeNumElements(), blockSize*num_rows_per_proc);
  TEST_EQUALITY(y.getMap()->getNodeNumElements(), blockSize*num_rows_per_proc);

  try {
    prec->apply (x, y);
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": prec->apply(x, y) threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "prec->apply(x, y)" );

  const Scalar exactSol = 0.2;

  for (int k = 0; k < num_rows_per_proc; ++k) {
    typename BMV::little_vec_type ylcl = yBlock.getLocalBlock(k,0);
    Scalar* yb = ylcl.getRawPtr();
    for (int j = 0; j < blockSize; ++j) {
      TEST_FLOATING_EQUALITY(yb[j],exactSol,1e-14);
    }
  }
}

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, TestLowerTriangularBlockCrsMatrix, Scalar, LO, GO)
{
  // Create a block lower triangular matrix
  // Matrix is [2*I_5 0 0; 3*I_5 2*I_ 0; I_5 3*I_5 2*I_5]
  // Solve the linear system A x = ones(15,1)
  // Solution is [0.5*ones(5,1); -0.25*ones(5,1); 0.625*ones(5,1)]
  using Teuchos::outArg;
  using Teuchos::RCP;
  using Teuchos::REDUCE_MIN;
  using Teuchos::reduceAll;
  using std::endl;
  typedef Tpetra::Experimental::BlockCrsMatrix<Scalar,LO,GO,Node> block_crs_matrix_type;
  typedef Tpetra::CrsGraph<LO,GO,Node> crs_graph_type;
  typedef Tpetra::RowMatrix<Scalar,LO,GO,Node> row_matrix_type;
  typedef Tpetra::Experimental::BlockMultiVector<Scalar,LO,GO,Node> BMV;
  typedef Tpetra::MultiVector<Scalar,LO,GO,Node> MV;
  typedef Ifpack2::BlockRelaxation<row_matrix_type, Ifpack2::DenseContainer<row_matrix_type,double> > prec_type;
  int lclSuccess = 1;
  int gblSuccess = 1;
  std::ostringstream errStrm; // for error collection

  out << "Ifpack2::BlockRelaxation lower triangular BlockCrsMatrix test" << endl;

  RCP<const Teuchos::Comm<int> > comm =
    Tpetra::DefaultPlatform::getDefaultPlatform().getComm();
  const int myRank = comm->getRank ();
  const int numProcs = comm->getSize ();

  const size_t num_rows_per_proc = 3;
  RCP<crs_graph_type> crsgraph;
  try {
    crsgraph = tif_utest::create_dense_local_graph<LO, GO, Node> (num_rows_per_proc);
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": create_dense_local_graph threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "create_dense_local_graph" );

  const int blockSize = 5;
  RCP<block_crs_matrix_type> bcrsmatrix;
  try {
    bcrsmatrix = Teuchos::rcp_const_cast<block_crs_matrix_type> (tif_utest::create_triangular_matrix<Scalar, LO, GO, Node, true> (crsgraph, blockSize));
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": create_triangular_matrix threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "create_triangular_matrix" );

  try {
    bcrsmatrix->computeDiagonalGraph ();
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": computeDiagonalGraph() threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "computeDiagonalGraph()" );

//  Teuchos::RCP<Teuchos::FancyOStream> wrappedStream = Teuchos::getFancyOStream (Teuchos::rcpFromRef (std::cout));
//  bcrsmatrix->describe (*wrappedStream, Teuchos::VERB_EXTREME);

  RCP<prec_type> prec;
  try {
    prec = rcp (new prec_type (bcrsmatrix));
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": Preconditioner constructor threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "Preconditioner constructor" );

  Teuchos::ParameterList params;
  params.set ("relaxation: type", "Gauss-Seidel");
  params.set ("partitioner: local parts", (LO)bcrsmatrix->getNodeNumRows());
  try {
    prec->setParameters (params);
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": prec->setParameters() threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "prec->setParameters()" );

  try {
    prec->initialize ();
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": prec->initialize() threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "prec->initialize()" );

  try {
    prec->compute ();
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": prec->compute() threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "prec->compute()" );

  BMV xBlock (* (crsgraph->getRowMap ()), blockSize, 1);
  BMV yBlock (* (crsgraph->getRowMap ()), blockSize, 1);
  MV x = xBlock.getMultiVectorView ();
  MV y = yBlock.getMultiVectorView ();
  x.putScalar (Teuchos::ScalarTraits<Scalar>::one ());

  TEST_EQUALITY( x.getMap()->getNodeNumElements (), blockSize * num_rows_per_proc );
  TEST_EQUALITY( y.getMap ()->getNodeNumElements (), blockSize * num_rows_per_proc );

  try {
    prec->apply (x, y);
  } catch (std::exception& e) {
    lclSuccess = 0;
    errStrm << "Process " << myRank << ": prec->apply(x, y) threw exception: "
            << e.what () << endl;
  }
  IFPACK2BLOCKRELAXATION_REPORT_GLOBAL_ERR( "prec->apply(x, y)" );

  Teuchos::Array<Scalar> exactSol(num_rows_per_proc);
  exactSol[0] = 0.5;
  exactSol[1] = -0.25;
  exactSol[2] = 0.625;

  for (size_t k = 0; k < num_rows_per_proc; ++k) {
    LO lcl_row = k;
    typename BMV::little_vec_type ylcl = yBlock.getLocalBlock(lcl_row,0);
    Scalar* yb = ylcl.getRawPtr();
    for (int j = 0; j < blockSize; ++j) {
      TEST_FLOATING_EQUALITY(yb[j],exactSol[k],1e-14);
    }
  }
}

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(Ifpack2BlockRelaxation, TestUpperTriangularBlockCrsMatrix, Scalar, LocalOrdinal, GlobalOrdinal)
{
  // Create a block lower triangular matrix
  // Matrix is [2*I_5 3*I_5 I_5; 0 2*I_5 3*I_5; 0 0 2*I_5]
  // Solve the linear system A x = ones(15,1)
  // Solution is [0.625*ones(5,1); -0.25*ones(5,1); 0.5*ones(5,1)]
  typedef Tpetra::Experimental::BlockCrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> block_crs_matrix_type;
  typedef Tpetra::RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> row_matrix_type;
  typedef Tpetra::Experimental::BlockMultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Node> BMV;
  typedef Tpetra::MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Node> MV;

  out << "Ifpack2::Version(): " << Ifpack2::Version () << std::endl;

  const int num_rows_per_proc = 3;
  const int blockSize = 5;

  Teuchos::RCP<const Teuchos::Comm<int> > comm = Tpetra::DefaultPlatform::getDefaultPlatform().getComm();
  Teuchos::RCP<Tpetra::CrsGraph<LocalOrdinal,GlobalOrdinal,Node> > crsgraph =
    tif_utest::create_dense_local_graph<LocalOrdinal,GlobalOrdinal,Node>(num_rows_per_proc);
  Teuchos::RCP<block_crs_matrix_type> bcrsmatrix =
    Teuchos::rcp_const_cast<block_crs_matrix_type> (tif_utest::create_triangular_matrix<Scalar,LocalOrdinal,GlobalOrdinal,Node,false> (crsgraph, blockSize));
  bcrsmatrix->computeDiagonalGraph();

//  Teuchos::RCP<Teuchos::FancyOStream> wrappedStream = Teuchos::getFancyOStream (Teuchos::rcpFromRef (std::cout));
//  bcrsmatrix->describe (*wrappedStream, Teuchos::VERB_EXTREME);
//  bcrsmatrix->getGraph()->describe (*wrappedStream, Teuchos::VERB_EXTREME);

  Ifpack2::BlockRelaxation<row_matrix_type, Ifpack2::DenseContainer<row_matrix_type,double> > prec (bcrsmatrix);

  Teuchos::ParameterList params;
  params.set("relaxation: type", "Symmetric Gauss-Seidel");
  params.set ("partitioner: local parts", (LocalOrdinal)bcrsmatrix->getNodeNumRows());
  prec.setParameters(params);

  prec.initialize();
  TEST_NOTHROW(prec.compute());

  BMV xBlock (*crsgraph->getRowMap (), blockSize, 1);
  BMV yBlock (*crsgraph->getRowMap (), blockSize, 1);
  MV x = xBlock.getMultiVectorView ();
  MV y = yBlock.getMultiVectorView ();
  x.putScalar (Teuchos::ScalarTraits<Scalar>::one ());

  TEST_EQUALITY(x.getMap()->getNodeNumElements(), blockSize*num_rows_per_proc);
  TEST_EQUALITY(y.getMap()->getNodeNumElements(), blockSize*num_rows_per_proc);
  TEST_NOTHROW(prec.apply(x, y));

  Teuchos::Array<Scalar> exactSol(num_rows_per_proc);
  exactSol[0] = 0.625;
  exactSol[1] = -0.25;
  exactSol[2] = 0.5;

  for (int k = 0; k < num_rows_per_proc; ++k) {
    typename BMV::little_vec_type ylcl = yBlock.getLocalBlock(k,0);
    auto yb = ylcl.getRawPtr();
    for (int j = 0; j < blockSize; ++j) {
      TEST_FLOATING_EQUALITY(yb[j],exactSol[k],1e-14);
    }
  }
}



#define UNIT_TEST_GROUP_SCALAR_ORDINAL(Scalar,LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, Test0,                             Scalar, LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, Test1,                             Scalar, LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, Test2,                             Scalar, LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, TriDi,                             Scalar, LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, LinePartition,                     Scalar, LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, BandedContainer,                   Scalar, LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, BlockedBandedContainer,            Scalar, LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, TestDiagonalBlockCrsMatrix,        Scalar, LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, TestLowerTriangularBlockCrsMatrix, Scalar, LocalOrdinal,GlobalOrdinal) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Ifpack2BlockRelaxation, TestUpperTriangularBlockCrsMatrix, Scalar, LocalOrdinal,GlobalOrdinal)

// FIXME (mfh 21 Oct 2015) This test exercises two different GO
// (GlobalOrdinal) types: whatever GO is, and GO = LO (LocalOrdinal).
// As such, given that LO = int is the only currently enabled LO type,
// this test won't build if GO = int is disabled (Bug 6358).  We
// disable building the test in that case.

#ifdef HAVE_TPETRA_INST_INT_INT

// mfh 21 Oct 2015: This class was only getting tested for Scalar =
// double, LocalOrdinal = int, GlobalOrdinal = int, and the default
// Node type.  As part of the fix for Bug 6358, I'm removing the
// assumption that GlobalOrdinal = int exists.

typedef Tpetra::MultiVector<>::scalar_type default_scalar_type;
typedef Tpetra::MultiVector<>::local_ordinal_type default_local_ordinal_type;
typedef Tpetra::MultiVector<>::global_ordinal_type default_global_ordinal_type;

UNIT_TEST_GROUP_SCALAR_ORDINAL(default_scalar_type, default_local_ordinal_type, default_global_ordinal_type)

#endif // HAVE_TPETRA_INST_INT_INT

} // namespace (anonymous)


