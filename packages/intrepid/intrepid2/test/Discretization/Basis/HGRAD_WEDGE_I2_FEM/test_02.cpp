// @HEADER
// ************************************************************************
//
//                           Intrepid Package
//                 Copyright (2007) Sandia Corporation
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
// Questions? Contact Pavel Bochev  (pbboche@sandia.gov)
//                    Denis Ridzal  (dridzal@sandia.gov), or
//                    Kara Peterson (kjpeter@sandia.gov)
//
// ************************************************************************
// @HEADER

/** \file test_02.cpp
\brief  Patch test for the Intrepid2::Basis_HGRAD_WEDGE_I2_FEM class.
\author Created by P. Bochev, D. Ridzal, K. Peterson, M. Perego.
*/

#include "Intrepid2_FieldContainer.hpp"
#include "Intrepid2_HGRAD_WEDGE_I2_FEM.hpp"
#include "Intrepid2_DefaultCubatureFactory.hpp"
#include "Intrepid2_RealSpaceTools.hpp"
#include "Intrepid2_ArrayTools.hpp"
#include "Intrepid2_FunctionSpaceTools.hpp"
#include "Intrepid2_CellTools.hpp"
#include "Teuchos_oblackholestream.hpp"
#include "Teuchos_RCP.hpp"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_SerialDenseMatrix.hpp"
#include "Teuchos_SerialDenseVector.hpp"
#include "Teuchos_LAPACK.hpp"

using namespace std;
using namespace Intrepid2;

void rhsFunc(FieldContainer<double> &, const FieldContainer<double> &, int, int, int);
void neumann(FieldContainer<double>       & ,
             const FieldContainer<double> & ,
             const FieldContainer<double> & ,
             const shards::CellTopology   & ,
             int, int, int, int);
void u_exact(FieldContainer<double> &, const FieldContainer<double> &, int, int, int);

/// right-hand side function
void rhsFunc(FieldContainer<double> & result,
             const FieldContainer<double> & points,
             int xd,
             int yd,
             int zd) {

  int x = 0, y = 1, z = 2;

  // second x-derivatives of u
  if (xd > 1) {
    for (int cell=0; cell<result.dimension(0); cell++) {
      for (int pt=0; pt<result.dimension(1); pt++) {
        result(cell,pt) = - xd*(xd-1)*std::pow(points(cell,pt,x), xd-2) *
                            std::pow(points(cell,pt,y), yd) * std::pow(points(cell,pt,z), zd);
      }
    }
  }

  // second y-derivatives of u
  if (yd > 1) {
    for (int cell=0; cell<result.dimension(0); cell++) {
      for (int pt=0; pt<result.dimension(1); pt++) {
        result(cell,pt) -=  yd*(yd-1)*std::pow(points(cell,pt,y), yd-2) *
                            std::pow(points(cell,pt,x), xd) * std::pow(points(cell,pt,z), zd);
      }
    }
  }

  // second z-derivatives of u
  if (zd > 1) {
    for (int cell=0; cell<result.dimension(0); cell++) {
      for (int pt=0; pt<result.dimension(1); pt++) {
        result(cell,pt) -=  zd*(zd-1)*std::pow(points(cell,pt,z), zd-2) *
                            std::pow(points(cell,pt,x), xd) * std::pow(points(cell,pt,y), yd);
      }
    }
  }

  // add u
  for (int cell=0; cell<result.dimension(0); cell++) {
    for (int pt=0; pt<result.dimension(1); pt++) {
      result(cell,pt) +=  std::pow(points(cell,pt,x), xd) * std::pow(points(cell,pt,y), yd) * std::pow(points(cell,pt,z), zd);
    }
  }

}


/// neumann boundary conditions
void neumann(FieldContainer<double>       & result,
             const FieldContainer<double> & points,
             const FieldContainer<double> & jacs,
             const shards::CellTopology   & parentCell,
             int sideOrdinal, int xd, int yd, int zd) {

  int x = 0, y = 1, z = 2;

  int numCells  = result.dimension(0);
  int numPoints = result.dimension(1);

  FieldContainer<double> grad_u(numCells, numPoints, 3);
  FieldContainer<double> side_normals(numCells, numPoints, 3);
  FieldContainer<double> normal_lengths(numCells, numPoints);

  // first x-derivatives of u
  if (xd > 0) {
    for (int cell=0; cell<numCells; cell++) {
      for (int pt=0; pt<numPoints; pt++) {
        grad_u(cell,pt,x) = xd*std::pow(points(cell,pt,x), xd-1) *
                            std::pow(points(cell,pt,y), yd) * std::pow(points(cell,pt,z), zd);
      }
    }
  }

  // first y-derivatives of u
  if (yd > 0) {
    for (int cell=0; cell<numCells; cell++) {
      for (int pt=0; pt<numPoints; pt++) {
        grad_u(cell,pt,y) = yd*std::pow(points(cell,pt,y), yd-1) *
                            std::pow(points(cell,pt,x), xd) * std::pow(points(cell,pt,z), zd);
      }
    }
  }

  // first z-derivatives of u
  if (zd > 0) {
    for (int cell=0; cell<numCells; cell++) {
      for (int pt=0; pt<numPoints; pt++) {
        grad_u(cell,pt,z) = zd*std::pow(points(cell,pt,z), zd-1) *
                            std::pow(points(cell,pt,x), xd) * std::pow(points(cell,pt,y), yd);
      }
    }
  }
  
  CellTools<double>::getPhysicalSideNormals(side_normals, jacs, sideOrdinal, parentCell);

  // scale normals
  RealSpaceTools<double>::vectorNorm(normal_lengths, side_normals, NORM_TWO);
  FunctionSpaceTools::scalarMultiplyDataData<double>(side_normals, normal_lengths, side_normals, true); 

  FunctionSpaceTools::dotMultiplyDataData<double>(result, grad_u, side_normals);

}

/// exact solution
void u_exact(FieldContainer<double> & result, const FieldContainer<double> & points, int xd, int yd, int zd) {
  int x = 0, y = 1, z = 2;
  for (int cell=0; cell<result.dimension(0); cell++) {
    for (int pt=0; pt<result.dimension(1); pt++) {
      result(cell,pt) = std::pow(points(pt,x), xd)*std::pow(points(pt,y), yd)*std::pow(points(pt,z), zd);
    }
  }
}




int main(int argc, char *argv[]) {

  Teuchos::GlobalMPISession mpiSession(&argc, &argv);
Kokkos::initialize();
  // This little trick lets us print to std::cout only if
  // a (dummy) command-line argument is provided.
  int iprint     = argc - 1;
  Teuchos::RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  if (iprint > 0)
    outStream = Teuchos::rcp(&std::cout, false);
  else
    outStream = Teuchos::rcp(&bhs, false);

  // Save the format state of the original std::cout.
  Teuchos::oblackholestream oldFormatState;
  oldFormatState.copyfmt(std::cout);

  *outStream \
    << "===============================================================================\n" \
    << "|                                                                             |\n" \
    << "|                    Unit Test (Basis_HGRAD_WEDGE_I2_FEM)                     |\n" \
    << "|                                                                             |\n" \
    << "|     1) Patch test involving mass and stiffness matrices,                    |\n" \
    << "|        for the Neumann problem on a wedge patch                             |\n" \
    << "|        Omega with boundary Gamma.                                           |\n" \
    << "|                                                                             |\n" \
    << "|        - div (grad u) + u = f  in Omega,  (grad u) . n = g  on Gamma        |\n" \
    << "|                                                                             |\n" \
    << "|  Questions? Contact  Pavel Bochev  (pbboche@sandia.gov),                    |\n" \
    << "|                      Denis Ridzal  (dridzal@sandia.gov),                    |\n" \
    << "|                      Kara Peterson (kjpeter@sandia.gov).                    |\n" \
    << "|                      Mauro Perego  (mperego@sandia.gov).                    |\n" \
    << "|                                                                             |\n" \
    << "|  Intrepid's website: http://trilinos.sandia.gov/packages/intrepid           |\n" \
    << "|  Trilinos website:   http://trilinos.sandia.gov                             |\n" \
    << "|                                                                             |\n" \
    << "===============================================================================\n"\
    << "| TEST 1: Patch test                                                          |\n"\
    << "===============================================================================\n";

  
  int errorFlag = 0;

  outStream -> precision(16);


  try {

    int max_order = 2;                                                                  // max total order of polynomial solution
    DefaultCubatureFactory<double>  cubFactory;                                         // create factory
    shards::CellTopology cell(shards::getCellTopologyData< shards::Wedge<> >());       // create parent cell topology
    shards::CellTopology sideQ(shards::getCellTopologyData< shards::Quadrilateral<> >());     // create relevant subcell (side) topology
    shards::CellTopology sideT(shards::getCellTopologyData< shards::Triangle<> >());
    int cellDim = cell.getDimension();
    int sideQDim = sideQ.getDimension();
    int sideTDim = sideT.getDimension();

    // Define array containing points at which the solution is evaluated, on the reference Wedge.
    int numIntervals = 10;
    int numInterpPoints = ((numIntervals + 1)*(numIntervals + 2)*(numIntervals + 3))/6;
    FieldContainer<double> interp_points_ref(numInterpPoints, 3);
    int counter = 0;
    for (int k=0; k<=numIntervals; k++) {
      for (int j=0; j<=numIntervals; j++) {
        for (int i=0; i<=numIntervals; i++) {
          if (i+j+k <= numIntervals) {
            interp_points_ref(counter,0) = i*(1.0/numIntervals);
            interp_points_ref(counter,1) = j*(1.0/numIntervals);
            interp_points_ref(counter,2) = k*(1.0/numIntervals);
            counter++;
          }
        }
      }
    }

    /* Definition of parent cell. */
    FieldContainer<double> cell_nodes(1, 6, cellDim);
    // funky Wedge (affine mapping)
    cell_nodes(0, 0, 0) = -3.0;
    cell_nodes(0, 0, 1) = -5.0;
    cell_nodes(0, 0, 2) = -5.0;
    cell_nodes(0, 1, 0) = -1.0;
    cell_nodes(0, 1, 1) = -4.0;
    cell_nodes(0, 1, 2) = -4.0;
    cell_nodes(0, 2, 0) = 0.0;
    cell_nodes(0, 2, 1) = 0.0;
    cell_nodes(0, 2, 2) = -5.0;
    cell_nodes(0, 3, 0) = 5.0;
    cell_nodes(0, 3, 1) = -1.0;
    cell_nodes(0, 3, 2) = 1.0;
    cell_nodes(0, 4, 0) = 7.0;
    cell_nodes(0, 4, 1) = 0.0;
    cell_nodes(0, 4, 2) = 2.0;
    cell_nodes(0, 5, 0) = 8.0;
    cell_nodes(0, 5, 1) = 4.0;
    cell_nodes(0, 5, 2) = 1.0;

    // perturbed reference Wedge
    /*cell_nodes(0, 0, 0) = 0.1;
    cell_nodes(0, 0, 1) = 0.1;
    cell_nodes(0, 0, 2) = -0.8;
    cell_nodes(0, 1, 0) = 1.2;
    cell_nodes(0, 1, 1) = 0.1;
    cell_nodes(0, 1, 2) = -0.95;
    cell_nodes(0, 2, 0) = 0.0;
    cell_nodes(0, 2, 1) = 0.9;
    cell_nodes(0, 2, 2) = -0.0;
    cell_nodes(0, 3, 0) = 0.1;
    cell_nodes(0, 3, 1) = 0.0;
    cell_nodes(0, 3, 2) = 0.9;
    cell_nodes(0, 4, 0) = 0.9;
    cell_nodes(0, 4, 1) = 0.1;
    cell_nodes(0, 4, 2) = 1.1;
    cell_nodes(0, 5, 0) = -0.1;
    cell_nodes(0, 5, 1) = 1.0;
    cell_nodes(0, 5, 2) = 1.0;*/

    // reference Wedge
    /*cell_nodes(0, 0, 0) = 0.0;
    cell_nodes(0, 0, 1) = 0.0;
    cell_nodes(0, 0, 2) = -1.0;
    cell_nodes(0, 1, 0) = 1.0;
    cell_nodes(0, 1, 1) = 0.0;
    cell_nodes(0, 1, 2) = -1.0;
    cell_nodes(0, 2, 0) = 0.0;
    cell_nodes(0, 2, 1) = 1.0;
    cell_nodes(0, 2, 2) = -1.0;
    cell_nodes(0, 3, 0) = 0.0;
    cell_nodes(0, 3, 1) = 0.0;
    cell_nodes(0, 3, 2) = 1.0;
    cell_nodes(0, 4, 0) = 1.0;
    cell_nodes(0, 4, 1) = 0.0;
    cell_nodes(0, 4, 2) = 1.0;
    cell_nodes(0, 5, 0) = 0.0;
    cell_nodes(0, 5, 1) = 1.0;
    cell_nodes(0, 5, 2) = 1.0;*/


    FieldContainer<double> interp_points(1, numInterpPoints, cellDim);
    CellTools<double>::mapToPhysicalFrame(interp_points, interp_points_ref, cell_nodes, cell);
    interp_points.resize(numInterpPoints, cellDim);

    for (int x_order=0; x_order <= max_order; x_order++) {
      for (int y_order=0; y_order <= max_order-x_order; y_order++) {
        for (int z_order=0; z_order <= max_order-x_order-y_order; z_order++) {

          // evaluate exact solution
          FieldContainer<double> exact_solution(1, numInterpPoints);
          u_exact(exact_solution, interp_points, x_order, y_order, z_order);

          int basis_order = 2;

          // set test tolerance;
          double zero = basis_order*basis_order*basis_order*100*INTREPID_TOL;

          //create basis
          Teuchos::RCP<Basis<double,FieldContainer<double> > > basis =
            Teuchos::rcp(new Basis_HGRAD_WEDGE_I2_FEM<double,FieldContainer<double> >() );
          int numFields = basis->getCardinality();

          // create cubatures
          Teuchos::RCP<Cubature<double> > cellCub = cubFactory.create(cell, 2*basis_order);
          Teuchos::RCP<Cubature<double> > sideQCub = cubFactory.create(sideQ, 2*basis_order);
          Teuchos::RCP<Cubature<double> > sideTCub = cubFactory.create(sideT, 2*basis_order);
          int numCubPointsCell = cellCub->getNumPoints();
          int numCubPointsSideQ = sideQCub->getNumPoints();
          int numCubPointsSideT = sideTCub->getNumPoints();

          /* Computational arrays. */
          /* Section 1: Related to parent cell integration. */
          FieldContainer<double> cub_points_cell(numCubPointsCell, cellDim);
          FieldContainer<double> cub_points_cell_physical(1, numCubPointsCell, cellDim);
          FieldContainer<double> cub_weights_cell(numCubPointsCell);
          FieldContainer<double> jacobian_cell(1, numCubPointsCell, cellDim, cellDim);
          FieldContainer<double> jacobian_inv_cell(1, numCubPointsCell, cellDim, cellDim);
          FieldContainer<double> jacobian_det_cell(1, numCubPointsCell);
          FieldContainer<double> weighted_measure_cell(1, numCubPointsCell);

          FieldContainer<double> value_of_basis_at_cub_points_cell(numFields, numCubPointsCell);
          FieldContainer<double> transformed_value_of_basis_at_cub_points_cell(1, numFields, numCubPointsCell);
          FieldContainer<double> weighted_transformed_value_of_basis_at_cub_points_cell(1, numFields, numCubPointsCell);
          FieldContainer<double> grad_of_basis_at_cub_points_cell(numFields, numCubPointsCell, cellDim);
          FieldContainer<double> transformed_grad_of_basis_at_cub_points_cell(1, numFields, numCubPointsCell, cellDim);
          FieldContainer<double> weighted_transformed_grad_of_basis_at_cub_points_cell(1, numFields, numCubPointsCell, cellDim);
          FieldContainer<double> fe_matrix(1, numFields, numFields);

          FieldContainer<double> rhs_at_cub_points_cell_physical(1, numCubPointsCell);
          FieldContainer<double> rhs_and_soln_vector(1, numFields);

          /* Section 2: Related to subcell (side) integration. */
          unsigned numSides = 5;
          unsigned numSidesQ = 3;
          FieldContainer<double> cub_points_sideQ(numCubPointsSideQ, sideQDim);
          FieldContainer<double> cub_points_sideT(numCubPointsSideT, sideTDim);
          FieldContainer<double> cub_weights_sideQ(numCubPointsSideQ);
          FieldContainer<double> cub_weights_sideT(numCubPointsSideT);
          FieldContainer<double> cub_points_sideQ_refcell(numCubPointsSideQ, cellDim);
          FieldContainer<double> cub_points_sideT_refcell(numCubPointsSideT, cellDim);
          FieldContainer<double> cub_points_sideQ_physical(1, numCubPointsSideQ, cellDim);
          FieldContainer<double> cub_points_sideT_physical(1, numCubPointsSideT, cellDim);
          FieldContainer<double> jacobian_sideQ_refcell(1, numCubPointsSideQ, cellDim, cellDim);
          FieldContainer<double> jacobian_sideT_refcell(1, numCubPointsSideT, cellDim, cellDim);
          FieldContainer<double> jacobian_det_sideQ_refcell(1, numCubPointsSideQ);
          FieldContainer<double> jacobian_det_sideT_refcell(1, numCubPointsSideT);
          FieldContainer<double> weighted_measure_sideQ_refcell(1, numCubPointsSideQ);
          FieldContainer<double> weighted_measure_sideT_refcell(1, numCubPointsSideT);

          FieldContainer<double> value_of_basis_at_cub_points_sideQ_refcell(numFields, numCubPointsSideQ);
          FieldContainer<double> value_of_basis_at_cub_points_sideT_refcell(numFields, numCubPointsSideT);
          FieldContainer<double> transformed_value_of_basis_at_cub_points_sideQ_refcell(1, numFields, numCubPointsSideQ);
          FieldContainer<double> transformed_value_of_basis_at_cub_points_sideT_refcell(1, numFields, numCubPointsSideT);
          FieldContainer<double> weighted_transformed_value_of_basis_at_cub_points_sideQ_refcell(1, numFields, numCubPointsSideQ);
          FieldContainer<double> weighted_transformed_value_of_basis_at_cub_points_sideT_refcell(1, numFields, numCubPointsSideT);
          FieldContainer<double> neumann_data_at_cub_points_sideQ_physical(1, numCubPointsSideQ);
          FieldContainer<double> neumann_data_at_cub_points_sideT_physical(1, numCubPointsSideT);
          FieldContainer<double> neumann_fields_per_side(1, numFields);

          /* Section 3: Related to global interpolant. */
          FieldContainer<double> value_of_basis_at_interp_points_ref(numFields, numInterpPoints);
          FieldContainer<double> transformed_value_of_basis_at_interp_points_ref(1, numFields, numInterpPoints);
          FieldContainer<double> interpolant(1, numInterpPoints);

          FieldContainer<int> ipiv(numFields);



          /******************* START COMPUTATION ***********************/

          // get cubature points and weights
          cellCub->getCubature(cub_points_cell, cub_weights_cell);

          // compute geometric cell information
          CellTools<double>::setJacobian(jacobian_cell, cub_points_cell, cell_nodes, cell);
          CellTools<double>::setJacobianInv(jacobian_inv_cell, jacobian_cell);
          CellTools<double>::setJacobianDet(jacobian_det_cell, jacobian_cell);

          // compute weighted measure
          FunctionSpaceTools::computeCellMeasure<double>(weighted_measure_cell, jacobian_det_cell, cub_weights_cell);

          ///////////////////////////
          // Computing mass matrices:
          // tabulate values of basis functions at (reference) cubature points
          basis->getValues(value_of_basis_at_cub_points_cell, cub_points_cell, OPERATOR_VALUE);

          // transform values of basis functions 
          FunctionSpaceTools::HGRADtransformVALUE<double>(transformed_value_of_basis_at_cub_points_cell,
                                                          value_of_basis_at_cub_points_cell);

          // multiply with weighted measure
          FunctionSpaceTools::multiplyMeasure<double>(weighted_transformed_value_of_basis_at_cub_points_cell,
                                                      weighted_measure_cell,
                                                      transformed_value_of_basis_at_cub_points_cell);

          // compute mass matrices
          FunctionSpaceTools::integrate<double>(fe_matrix,
                                                transformed_value_of_basis_at_cub_points_cell,
                                                weighted_transformed_value_of_basis_at_cub_points_cell,
                                                COMP_BLAS);
          ///////////////////////////

          ////////////////////////////////
          // Computing stiffness matrices:
          // tabulate gradients of basis functions at (reference) cubature points
          basis->getValues(grad_of_basis_at_cub_points_cell, cub_points_cell, OPERATOR_GRAD);

          // transform gradients of basis functions 
          FunctionSpaceTools::HGRADtransformGRAD<double>(transformed_grad_of_basis_at_cub_points_cell,
                                                         jacobian_inv_cell,
                                                         grad_of_basis_at_cub_points_cell);

          // multiply with weighted measure
          FunctionSpaceTools::multiplyMeasure<double>(weighted_transformed_grad_of_basis_at_cub_points_cell,
                                                      weighted_measure_cell,
                                                      transformed_grad_of_basis_at_cub_points_cell);

          // compute stiffness matrices and sum into fe_matrix
          FunctionSpaceTools::integrate<double>(fe_matrix,
                                                transformed_grad_of_basis_at_cub_points_cell,
                                                weighted_transformed_grad_of_basis_at_cub_points_cell,
                                                COMP_BLAS,
                                                true);
          ////////////////////////////////

          ///////////////////////////////
          // Computing RHS contributions:
          // map cell (reference) cubature points to physical space
          CellTools<double>::mapToPhysicalFrame(cub_points_cell_physical, cub_points_cell, cell_nodes, cell);

          // evaluate rhs function
          rhsFunc(rhs_at_cub_points_cell_physical, cub_points_cell_physical, x_order, y_order, z_order);

          // compute rhs
          FunctionSpaceTools::integrate<double>(rhs_and_soln_vector,
                                                rhs_at_cub_points_cell_physical,
                                                weighted_transformed_value_of_basis_at_cub_points_cell,
                                                COMP_BLAS);

          // compute neumann b.c. contributions and adjust rhs
          sideQCub->getCubature(cub_points_sideQ, cub_weights_sideQ);
          sideTCub->getCubature(cub_points_sideT, cub_weights_sideT);

          for (unsigned i=0; i<numSidesQ; i++) {
            // compute geometric cell information
            CellTools<double>::mapToReferenceSubcell(cub_points_sideQ_refcell, cub_points_sideQ, sideQDim, (int)i, cell);
            CellTools<double>::setJacobian(jacobian_sideQ_refcell, cub_points_sideQ_refcell, cell_nodes, cell);
            CellTools<double>::setJacobianDet(jacobian_det_sideQ_refcell, jacobian_sideQ_refcell);

            // compute weighted face measure
            FunctionSpaceTools::computeFaceMeasure<double>(weighted_measure_sideQ_refcell,
                                                           jacobian_sideQ_refcell,
                                                           cub_weights_sideQ,
                                                           i,
                                                           cell);

            // tabulate values of basis functions at side cubature points, in the reference parent cell domain
            basis->getValues(value_of_basis_at_cub_points_sideQ_refcell, cub_points_sideQ_refcell, OPERATOR_VALUE);
            // transform
            FunctionSpaceTools::HGRADtransformVALUE<double>(transformed_value_of_basis_at_cub_points_sideQ_refcell,
                                                            value_of_basis_at_cub_points_sideQ_refcell);

            // multiply with weighted measure
            FunctionSpaceTools::multiplyMeasure<double>(weighted_transformed_value_of_basis_at_cub_points_sideQ_refcell,
                                                        weighted_measure_sideQ_refcell,
                                                        transformed_value_of_basis_at_cub_points_sideQ_refcell);

            // compute Neumann data
            // map side cubature points in reference parent cell domain to physical space
            CellTools<double>::mapToPhysicalFrame(cub_points_sideQ_physical, cub_points_sideQ_refcell, cell_nodes, cell);
            // now compute data
            neumann(neumann_data_at_cub_points_sideQ_physical, cub_points_sideQ_physical, jacobian_sideQ_refcell,
                    cell, (int)i, x_order, y_order, z_order);

            FunctionSpaceTools::integrate<double>(neumann_fields_per_side,
                                                  neumann_data_at_cub_points_sideQ_physical,
                                                  weighted_transformed_value_of_basis_at_cub_points_sideQ_refcell,
                                                  COMP_BLAS);

            // adjust RHS
            RealSpaceTools<double>::add(rhs_and_soln_vector, neumann_fields_per_side);;
          }

          for (unsigned i=numSidesQ; i<numSides; i++) {
			// compute geometric cell information
			CellTools<double>::mapToReferenceSubcell(cub_points_sideT_refcell, cub_points_sideT, sideTDim, (int)i, cell);
			CellTools<double>::setJacobian(jacobian_sideT_refcell, cub_points_sideT_refcell, cell_nodes, cell);
			CellTools<double>::setJacobianDet(jacobian_det_sideT_refcell, jacobian_sideT_refcell);

			// compute weighted face measure
			FunctionSpaceTools::computeFaceMeasure<double>(weighted_measure_sideT_refcell,
														   jacobian_sideT_refcell,
														   cub_weights_sideT,
														   i,
														   cell);

			// tabulate values of basis functions at side cubature points, in the reference parent cell domain
			basis->getValues(value_of_basis_at_cub_points_sideT_refcell, cub_points_sideT_refcell, OPERATOR_VALUE);
			// transform
			FunctionSpaceTools::HGRADtransformVALUE<double>(transformed_value_of_basis_at_cub_points_sideT_refcell,
			                       						    value_of_basis_at_cub_points_sideT_refcell);

			// multiply with weighted measure
			FunctionSpaceTools::multiplyMeasure<double>(weighted_transformed_value_of_basis_at_cub_points_sideT_refcell,
													    weighted_measure_sideT_refcell,
														transformed_value_of_basis_at_cub_points_sideT_refcell);

			// compute Neumann data
			// map side cubature points in reference parent cell domain to physical space
			CellTools<double>::mapToPhysicalFrame(cub_points_sideT_physical, cub_points_sideT_refcell, cell_nodes, cell);
			// now compute data
			neumann(neumann_data_at_cub_points_sideT_physical, cub_points_sideT_physical, jacobian_sideT_refcell,
				    cell, (int)i, x_order, y_order, z_order);

			FunctionSpaceTools::integrate<double>(neumann_fields_per_side,
			 									  neumann_data_at_cub_points_sideT_physical,
												  weighted_transformed_value_of_basis_at_cub_points_sideT_refcell,
												  COMP_BLAS);

		    // adjust RHS
			RealSpaceTools<double>::add(rhs_and_soln_vector, neumann_fields_per_side);;
		  }
          ///////////////////////////////

          /////////////////////////////
          // Solution of linear system:
          int info = 0;
          Teuchos::LAPACK<int, double> solver;
          solver.GESV(numFields, 1, &fe_matrix[0], numFields, &ipiv(0), &rhs_and_soln_vector[0], numFields, &info);
          /////////////////////////////

          ////////////////////////
          // Building interpolant:
          // evaluate basis at interpolation points
          basis->getValues(value_of_basis_at_interp_points_ref, interp_points_ref, OPERATOR_VALUE);
          // transform values of basis functions 
          FunctionSpaceTools::HGRADtransformVALUE<double>(transformed_value_of_basis_at_interp_points_ref,
                                                          value_of_basis_at_interp_points_ref);
          FunctionSpaceTools::evaluate<double>(interpolant, rhs_and_soln_vector, transformed_value_of_basis_at_interp_points_ref);
          ////////////////////////

          /******************* END COMPUTATION ***********************/
      
          RealSpaceTools<double>::subtract(interpolant, exact_solution);

          *outStream << "\nRelative norm-2 error between exact solution polynomial of order ("
                     << x_order << ", " << y_order << ", " << z_order
                     << ") and finite element interpolant of order " << basis_order << ": "
                     << RealSpaceTools<double>::vectorNorm(&interpolant[0], interpolant.dimension(1), NORM_TWO) /
                        RealSpaceTools<double>::vectorNorm(&exact_solution[0], exact_solution.dimension(1), NORM_TWO) << "\n";

          if (RealSpaceTools<double>::vectorNorm(&interpolant[0], interpolant.dimension(1), NORM_TWO) /
              RealSpaceTools<double>::vectorNorm(&exact_solution[0], exact_solution.dimension(1), NORM_TWO) > zero) {
            *outStream << "\n\nPatch test failed for solution polynomial order ("
                       << x_order << ", " << y_order << ", " << z_order << ") and basis order " << basis_order << "\n\n";
            errorFlag++;
          }
        } // end for z_order
      } // end for y_order
    } // end for x_order

  }
  // Catch unexpected errors
  catch (std::logic_error err) {
    *outStream << err.what() << "\n\n";
    errorFlag = -1000;
  };

  if (errorFlag != 0)
    std::cout << "End Result: TEST FAILED\n";
  else
    std::cout << "End Result: TEST PASSED\n";

  // reset format state of std::cout
  std::cout.copyfmt(oldFormatState);
Kokkos::finalize();
  return errorFlag;
}
