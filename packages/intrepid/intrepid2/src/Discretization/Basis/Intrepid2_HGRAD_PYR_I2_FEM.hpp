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

/** \file   Intrepid_HGRAD_PYR_I2_FEM.hpp
    \brief  Header file for the Intrepid2::HGRAD_PYR_I2_FEM class.
    \author Created by P. Bochev, D. Ridzal and K. Peterson.
*/

#ifndef INTREPID2_HGRAD_PYR_I2_FEM_HPP
#define INTREPID2_HGRAD_PYR_I2_FEM_HPP
#include "Intrepid2_Basis.hpp"

namespace Intrepid2 {
  
/** \class  Intrepid2::Basis_HGRAD_PYR_I2_FEM
    \brief  Implementation of an H(grad)-compatible FEM basis of degree 2 on a Pyramid cell

            Implements Lagrangian basis of degree 2 on the reference Pyramid cell. The basis has
            cardinality 13 and spans an INCOMPLETE bi-quadratic polynomial space. Basis functions are dual 
            to a unisolvent set of degrees-of-freedom (DoF) defined and enumerated as follows:

  \verbatim
  =================================================================================================
  |         |           degree-of-freedom-tag table                    |                           |
  |   DoF   |----------------------------------------------------------|      DoF definition       |
  | ordinal |  subc dim    | subc ordinal | subc DoF ord |subc num DoF |                           |
  |=========|==============|==============|==============|=============|===========================|
  |    0    |       0      |       0      |       0      |      1      |   L_0(u) = u(-1,-1, 0)    |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |    1    |       0      |       1      |       0      |      1      |   L_1(u) = u( 1,-1, 0)    |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |    2    |       0      |       2      |       0      |      1      |   L_2(u) = u( 1, 1, 0)    |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |    3    |       0      |       3      |       0      |      1      |   L_3(u) = u(-1, 1, 0)    |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |    4    |       0      |       4      |       0      |      1      |   L_4(u) = u( 0, 0, 1)    |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |    5    |       1      |       0      |       0      |      1      |   L_5(u) = u( 0,-1, 0)    |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |    6    |       1      |       1      |       0      |      1      |   L_6(u) = u( 1, 0, 0)    |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |    7    |       1      |       2      |       0      |      1      |   L_7(u) = u( 0, 1, 0)    |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |    8    |       1      |       3      |       0      |      1      |   L_8(u) = u(-1, 0, 0)    |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |    9    |       1      |       4      |       0      |      1      | L_9(u) = u(-1/2,-1/2,1/2) |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |   10    |       1      |       5      |       0      |      1      | L_10(u)= u( 1/2,-1/2,1/2) |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |   11    |       1      |       6      |       0      |      1      | L_11(u)= u( 1/2, 1/2,1/2) |
  |---------|--------------|--------------|--------------|-------------|---------------------------|
  |   12    |       1      |       7      |       0      |      1      | L_12(u)= u(-1/2, 1/2,1/2) |
  |=========|==============|==============|==============|=============|===========================|
  |   MAX   |  maxScDim=1  |  maxScOrd=7  |  maxDfOrd=0  |      -      |                           |
  |=========|==============|==============|==============|=============|===========================|
  \endverbatim
  
      \remark   Ordering of DoFs follows the node order in Pyramid<13> topology. 

      \remark  WARNING: Does not satisfy a patch test for arbitrarily deformed pyramids, but only
                        for pyramids with bottom quadrilateral nodes forming a plane.
 */
template<class Scalar, class ArrayScalar> 
class Basis_HGRAD_PYR_I2_FEM : public Basis<Scalar, ArrayScalar> {
private:

  /** \brief  Initializes <var>tagToOrdinal_</var> and <var>ordinalToTag_</var> lookup arrays.
   */
  void initializeTags();
  
public:

  /** \brief  Constructor.
   */
  Basis_HGRAD_PYR_I2_FEM();
  
    
  /** \brief  FEM basis evaluation on a <strong>reference Pyramid</strong> cell. 
    
              Returns values of <var>operatorType</var> acting on FEM basis functions for a set of
              points in the <strong>reference Pyramid</strong> cell. For rank and dimensions of 
              I/O array arguments see Section \ref basis_md_array_sec .
  
      \param  outputValues      [out] - rank-2 or 3 array with the computed basis values
      \param  inputPoints       [in]  - rank-2 array with dimensions (P,D) containing reference points  
      \param  operatorType      [in]  - operator applied to basis functions        
   */
  void getValues(ArrayScalar &          outputValues,
                 const ArrayScalar &    inputPoints,
                 const EOperator        operatorType) const;
  
  
  /**  \brief  FVD basis evaluation: invocation of this method throws an exception.
   */
  void getValues(ArrayScalar &          outputValues,
                 const ArrayScalar &    inputPoints,
                 const ArrayScalar &    cellVertices,
                 const EOperator        operatorType = OPERATOR_VALUE) const;
};
}// namespace Intrepid2

#include "Intrepid2_HGRAD_PYR_I2_FEMDef.hpp"

#endif
