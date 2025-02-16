// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
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
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef ROL_SROM_EQUALITY_CONSTRAINT_H
#define ROL_SROM_EQUALITY_CONSTRAINT_H

#include "ROL_EqualityConstraint.hpp"
#include "ROL_BatchManager.hpp"
#include "ROL_SROMVector.hpp"
#include "ROL_StdVector.hpp"
#include "ROL_Types.hpp"

namespace ROL {

template <class Real>
class SROMEqualityConstraint : public EqualityConstraint<Real> {
private:
  Teuchos::RCP<BatchManager<Real> > bman_;

public:
  SROMEqualityConstraint(Teuchos::RCP<BatchManager<Real> > &bman)
    : EqualityConstraint<Real>(), bman_(bman) {}

  void value(Vector<Real> &c, const Vector<Real> &x, Real &tol) {
    Teuchos::RCP<std::vector<Real> > ec =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<StdVector<Real> >(c)).getVector());
    const PrimalSROMVector<Real> &ex = Teuchos::dyn_cast<const PrimalSROMVector<Real> >(x);
    Real psum = 0.0, sum = 0.0;
    for (size_t i = 0; i < ex.getNumSamples(); i++) {
      psum += ex.getWeight(i);
    }
    bman_->sumAll(&psum,&sum,1);
    (*ec)[0] = sum - 1.0;
  }

  void applyJacobian(Vector<Real> &jv, const Vector<Real> &v, const Vector<Real> &x, Real &tol) {
    Teuchos::RCP<std::vector<Real> > ejv =
      Teuchos::rcp_const_cast<std::vector<Real> >((Teuchos::dyn_cast<StdVector<Real> >(jv)).getVector());
    const PrimalSROMVector<Real> &ev = Teuchos::dyn_cast<const PrimalSROMVector<Real> >(v);
    Real psum = 0.0, sum = 0.0;
    for (size_t i = 0; i < ev.getNumSamples(); i++) {
      psum += ev.getWeight(i);
    }
    bman_->sumAll(&psum,&sum,1);
    (*ejv)[0] = sum;
  }

  void applyAdjointJacobian(Vector<Real> &ajv, const Vector<Real> &v, const Vector<Real> &x, Real &tol) {
    DualSROMVector<Real> &eajv = Teuchos::dyn_cast<DualSROMVector<Real> >(ajv);
    Teuchos::RCP<const std::vector<Real> > ev =
      (Teuchos::dyn_cast<StdVector<Real> >(const_cast<Vector<Real> &>(v))).getVector();
    const std::vector<Real> pt(eajv.getDimension(),0.0);
    for (size_t i = 0; i < eajv.getNumSamples(); i++) {
      eajv.setPoint(i,pt);
      eajv.setWeight(i,(*ev)[0]);
    }
  }

  void applyAdjointHessian(Vector<Real> &ahuv, const Vector<Real> &u, const Vector<Real> &v,
                                   const Vector<Real> &x, Real &tol) {
    ahuv.zero();
  }

}; // class SROMEqualityConstraint

} // namespace ROL

#endif
