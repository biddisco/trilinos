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

#ifndef ROL_CDFOBJECTIVE_H
#define ROL_CDFOBJECTIVE_H

#include "ROL_Objective.hpp"
#include "ROL_BatchManager.hpp"
#include "ROL_Vector.hpp"
#include "ROL_Distribution.hpp"
#include "Teuchos_RCP.hpp"
#include <math.h>

namespace ROL {

template <class Real>
class CDFObjective : public Objective<Real> {
private:
  std::vector<Teuchos::RCP<Distribution<Real> > > dist_;
  Teuchos::RCP<BatchManager<Real> > bman_;
  const std::vector<Real> lowerBound_;
  const std::vector<Real> upperBound_;
  const Real scale_;
  const Real sqrt2_;
  const Real sqrtpi_;

  std::vector<Real> pts_;
  std::vector<Real> wts_;
  size_t numPoints_;

  void initializeQuadrature(void) {
    numPoints_ = 20;
    pts_.clear(); pts_.resize(numPoints_,0.);
    wts_.clear(); wts_.resize(numPoints_,0.);
    wts_[0]  = 0.1527533871307258; pts_[0]  = -0.0765265211334973;
    wts_[1]  = 0.1527533871307258; pts_[1]  =  0.0765265211334973;
    wts_[2]  = 0.1491729864726037; pts_[2]  = -0.2277858511416451;
    wts_[3]  = 0.1491729864726037; pts_[3]  =  0.2277858511416451;
    wts_[4]  = 0.1420961093183820; pts_[4]  = -0.3737060887154195;
    wts_[5]  = 0.1420961093183820; pts_[5]  =  0.3737060887154195;
    wts_[6]  = 0.1316886384491766; pts_[6]  = -0.5108670019508271;
    wts_[7]  = 0.1316886384491766; pts_[7]  =  0.5108670019508271;
    wts_[8]  = 0.1181945319615184; pts_[8]  = -0.6360536807265150;
    wts_[9]  = 0.1181945319615184; pts_[9]  =  0.6360536807265150;
    wts_[10] = 0.1019301198172404; pts_[10] = -0.7463319064601508;
    wts_[11] = 0.1019301198172404; pts_[11] =  0.7463319064601508;
    wts_[12] = 0.0832767415767048; pts_[12] = -0.8391169718222188;
    wts_[13] = 0.0832767415767048; pts_[13] =  0.8391169718222188;
    wts_[14] = 0.0626720483341091; pts_[14] = -0.9122344282513259;
    wts_[15] = 0.0626720483341091; pts_[15] =  0.9122344282513259;
    wts_[16] = 0.0406014298003869; pts_[16] = -0.9639719272779138;
    wts_[17] = 0.0406014298003869; pts_[17] =  0.9639719272779138;
    wts_[18] = 0.0176140071391521; pts_[18] = -0.9931285991850949;
    wts_[19] = 0.0176140071391521; pts_[19] =  0.9931285991850949;
    for (size_t i = 0; i < numPoints_; i++) {
      wts_[i] *= 0.5;
      pts_[i] += 1.; pts_[i] *= 0.5;
    }
  }

  Real valueCDF(const size_t dim, const Real loc,
                const PrimalSROMVector<Real> &x) const {
    const size_t numSamples = x.getNumSamples();
    Real val = 0., hs = 0., xpt = 0., xwt = 0., sum = 0.;
    for (size_t k = 0; k < numSamples; k++) {
      xpt = (*x.getPoint(k))[dim]; xwt = x.getWeight(k);
      hs = 0.5 * (1. + erf((loc-xpt)/(sqrt2_*scale_)));
      val += xwt * hs;
    }
    bman_->sumAll(&val,&sum,1);
    return sum;
  }

  Real gradientCDF(std::vector<Real> &gradx, std::vector<Real> &gradp,
             const size_t dim, const Real loc,
             const PrimalSROMVector<Real> &x) const {
    const size_t numSamples = x.getNumSamples();
    gradx.resize(numSamples,0.); gradp.resize(numSamples,0.);
    Real val = 0., hs = 0., xpt = 0., xwt = 0., sum = 0.;
    for (size_t k = 0; k < numSamples; k++) {
      xpt = (*x.getPoint(k))[dim]; xwt = x.getWeight(k);
      hs = 0.5 * (1. + erf((loc-xpt)/(sqrt2_*scale_)));
      val += xwt * hs;
      gradx[k] = -(xwt/(sqrt2_*sqrtpi_*scale_))
                 * std::exp(-std::pow((loc-xpt)/(sqrt2_*scale_),2));
      gradp[k] = hs;
    }
    bman_->sumAll(&val,&sum,1);
    return sum;
  }

  Real hessVecCDF(std::vector<Real> &hvxx, std::vector<Real> &hvxp, std::vector<Real> &hvpx,
                  std::vector<Real> &gradx, std::vector<Real> &gradp,
                  Real &sumx, Real &sump,
            const size_t dim, const Real loc,
            const PrimalSROMVector<Real> &x, const PrimalSROMVector<Real> &v) const {
    const size_t numSamples = x.getNumSamples();
    hvxx.resize(numSamples,0.); hvxp.resize(numSamples,0.); hvpx.resize(numSamples,0.);
    gradx.resize(numSamples,0.); gradp.resize(numSamples,0.);
    sumx = 0.; sump = 0.;
    std::vector<Real> psum(3,0.0), out(3,0.0);
    Real val = 0., hs = 0., dval = 0., scale3 = std::pow(scale_,3);
    Real xpt = 0., xwt = 0., vpt = 0., vwt = 0.;
    for (size_t k = 0; k < numSamples; k++) {
      xpt = (*x.getPoint(k))[dim]; xwt = x.getWeight(k);
      vpt = (*v.getPoint(k))[dim]; vwt = v.getWeight(k);
      hs = 0.5 * (1. + erf((loc-xpt)/(sqrt2_*scale_)));
      psum[0] += xwt * hs;
      dval = std::exp(-std::pow((loc-xpt)/(sqrt2_*scale_),2));
      gradx[k] = -(xwt/(sqrt2_*sqrtpi_*scale_)) * dval;
      gradp[k] = hs;
      hvxx[k] = -(xwt/(sqrt2_*sqrtpi_*scale3)) * dval * (loc-xpt) * vpt;
      hvxp[k] = -dval/(sqrt2_*sqrtpi_*scale_)*vwt;
      hvpx[k] = -dval/(sqrt2_*sqrtpi_*scale_)*vpt;
      psum[1] += vpt*gradx[k];
      psum[2] += vwt*gradp[k];
    }
    bman_->sumAll(&psum[0],&out[0],3);
    val = out[0]; sumx = out[1]; sump = out[2];
    return val;
  }

public:
  CDFObjective(const std::vector<Teuchos::RCP<Distribution<Real> > > &dist,
               const std::vector<Real> &lo, const std::vector<Real> &up,
                     Teuchos::RCP<BatchManager<Real> > &bman,
               const Real scale = 1.e-2)
    : Objective<Real>(), dist_(dist), bman_(bman), lowerBound_(lo), upperBound_(up),
      scale_(scale), sqrt2_(std::sqrt(2.)), sqrtpi_(std::sqrt(M_PI)) {
    initializeQuadrature();
  }

  Real value( const Vector<Real> &x, Real &tol ) {
    const PrimalSROMVector<Real> &ex = Teuchos::dyn_cast<const PrimalSROMVector<Real> >(x);
    const size_t dimension  = ex.getDimension();
    Real val = 0., diff = 0., pt = 0., wt = 0., meas = 0., lb = 0.;
    for (size_t d = 0; d < dimension; d++) {
      lb   = lowerBound_[d];
      meas = (upperBound_[d] - lb);
      for (size_t k = 0; k < numPoints_; k++) {
        pt = meas*pts_[k] + lb;
        wt = wts_[k]/meas;
        diff = (valueCDF(d,pt,ex)-dist_[d]->evaluateCDF(pt));
        val += wt*std::pow(diff,2);
      }
    }
    return 0.5*val;
  }

  void gradient( Vector<Real> &g, const Vector<Real> &x, Real &tol ) {
    DualSROMVector<Real> &eg = Teuchos::dyn_cast<DualSROMVector<Real> >(g);
    const PrimalSROMVector<Real> &ex = Teuchos::dyn_cast<const PrimalSROMVector<Real> >(x);
    const size_t dimension  = ex.getDimension();
    const size_t numSamples = ex.getNumSamples();
    std::vector<Real> gradx(numSamples,0.), gradp(numSamples,0.);
    Real diff = 0., pt = 0., wt = 0., meas = 0., lb = 0., val = 0.;
    std::vector<Real> val_wt(numSamples,0.), tmp(dimension,0.);
    std::vector<std::vector<Real> > val_pt(numSamples,tmp);
    for (size_t d = 0; d < dimension; d++) {
      lb   = lowerBound_[d];
      meas = (upperBound_[d] - lb);
      for (size_t k = 0; k < numPoints_; k++) {
        pt = meas*pts_[k] + lb;
        wt = wts_[k]/meas;
        val = gradientCDF(gradx,gradp,d,pt,ex);
        diff = (val-dist_[d]->evaluateCDF(pt));
        for (size_t j = 0; j < numSamples; j++) {
          (val_pt[j])[d] += wt * diff * gradx[j];
          val_wt[j]      += wt * diff * gradp[j];
        }
      }
    }
    for (size_t k = 0; k < numSamples; k++) {
      eg.setPoint(k,val_pt[k]);
      eg.setWeight(k,val_wt[k]);
    }
  }

  void hessVec( Vector<Real> &hv, const Vector<Real> &v, const Vector<Real> &x, Real &tol ) {
    DualSROMVector<Real> &ehv = Teuchos::dyn_cast<DualSROMVector<Real> >(hv);
    const PrimalSROMVector<Real> &ev = Teuchos::dyn_cast<const PrimalSROMVector<Real> >(v);
    const PrimalSROMVector<Real> &ex = Teuchos::dyn_cast<const PrimalSROMVector<Real> >(x);
    const size_t dimension  = ex.getDimension();
    const size_t numSamples = ex.getNumSamples();
    std::vector<Real> hvxx(numSamples,0.), hvxp(numSamples,0.), hvpx(numSamples,0.);
    std::vector<Real> gradx(numSamples,0.), gradp(numSamples,0.);
    Real diff = 0., pt = 0., wt = 0., meas = 0., lb = 0., val = 0., sumx = 0., sump = 0.;
    std::vector<Real> val_wt(numSamples,0.), tmp(dimension,0.);
    std::vector<std::vector<Real> > val_pt(numSamples,tmp);
    for (size_t d = 0; d < dimension; d++) {
      lb   = lowerBound_[d];
      meas = (upperBound_[d] - lb);
      for (size_t k = 0; k < numPoints_; k++) {
        pt = meas*pts_[k] + lb;
        wt = wts_[k]/meas;
        val = hessVecCDF(hvxx,hvxp,hvpx,gradx,gradp,sumx,sump,d,pt,ex,ev);
        diff = (val-dist_[d]->evaluateCDF(pt));
        for (size_t j = 0; j < numSamples; j++) {
          (val_pt[j])[d] += wt * ( (sump + sumx) * gradx[j] + diff * (hvxx[j] + hvxp[j]) );
          val_wt[j]      += wt * ( (sump + sumx) * gradp[j] + diff * hvpx[j] );
        }
      }
    }
    for (size_t k = 0; k < numSamples; k++) {
      ehv.setPoint(k,val_pt[k]);
      ehv.setWeight(k,val_wt[k]);
    }
  }
}; // class LinearCombinationObjective

} // namespace ROL

#endif
