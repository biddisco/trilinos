// @HEADER
// ***********************************************************************
//
//           Panzer: A partial differential equation assembly
//       engine for strongly coupled complex multiphysics systems
//                 Copyright (2011) Sandia Corporation
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
// Questions? Contact Roger P. Pawlowski (rppawlo@sandia.gov) and
// Eric C. Cyr (eccyr@sandia.gov)
// ***********************************************************************
// @HEADER

#ifdef HAVE_STOKHOS

#ifndef PANZER_EVALUATOR_SCATTER_DIRICHLET_RESIDUAL_TPETRA_SG_DECL_HPP
#define PANZER_EVALUATOR_SCATTER_DIRICHLET_RESIDUAL_TPETRA_SG_DECL_HPP

#include "Panzer_SGTpetraLinearObjContainer.hpp"

#include "Panzer_Evaluator_WithBaseImpl.hpp"

namespace panzer {

// **************************************************************
// SGResidual 
// **************************************************************
template<typename TRAITS,typename LO,typename GO,typename NodeT>
class ScatterDirichletResidual_Tpetra<panzer::Traits::SGResidual,TRAITS,LO,GO,NodeT>
  : public panzer::EvaluatorWithBaseImpl<TRAITS>,
    public PHX::EvaluatorDerived<panzer::Traits::SGResidual, TRAITS>,
    public panzer::CloneableEvaluator  {
  
public:
  ScatterDirichletResidual_Tpetra(const Teuchos::RCP<const UniqueGlobalIndexer<LO,GO> > & indexer)
     : globalIndexer_(indexer) {}
  
  ScatterDirichletResidual_Tpetra(const Teuchos::RCP<const UniqueGlobalIndexer<LO,GO> > & indexer,
                                  const Teuchos::ParameterList& p);
  
  void postRegistrationSetup(typename TRAITS::SetupData d,
			     PHX::FieldManager<TRAITS>& vm);

  void preEvaluate(typename TRAITS::PreEvalData d);
  
  void evaluateFields(typename TRAITS::EvalData workset);
  
  virtual Teuchos::RCP<CloneableEvaluator> clone(const Teuchos::ParameterList & pl) const
  { return Teuchos::rcp(new ScatterDirichletResidual_Tpetra<panzer::Traits::SGResidual,TRAITS,LO,GO>(globalIndexer_,pl)); }

private:
  typedef typename panzer::Traits::SGResidual::ScalarT ScalarT;
  typedef SGTpetraLinearObjContainer<double,LO,GO,NodeT> LOC;

  // dummy field so that the evaluator will have something to do
  Teuchos::RCP<PHX::FieldTag> scatterHolder_;

  // fields that need to be scattered will be put in this vector
  std::vector< PHX::MDField<ScalarT,Cell,NODE> > scatterFields_;

  // maps the local (field,element,basis) triplet to a global ID
  // for scattering
  Teuchos::RCP<const panzer::UniqueGlobalIndexer<LO,GO> > globalIndexer_;
  std::vector<int> fieldIds_; // field IDs needing mapping

  // This maps the scattered field names to the DOF manager field
  // For instance a Navier-Stokes map might look like
  //    fieldMap_["RESIDUAL_Velocity"] --> "Velocity"
  //    fieldMap_["RESIDUAL_Pressure"] --> "Pressure"
  Teuchos::RCP<const std::map<std::string,std::string> > fieldMap_;

  std::size_t num_nodes;

  std::size_t side_subcell_dim_;
  std::size_t local_side_id_;

  // Teuchos::RCP<typename LOC::VectorType> dirichletCounter_;

  std::string globalDataKey_; // what global data does this fill?

  Teuchos::RCP<const LOC> sgTpetraContainer_;

  ScatterDirichletResidual_Tpetra() {}
};

// **************************************************************
// SGJacobian
// **************************************************************
template<typename TRAITS,typename LO,typename GO,typename NodeT>
class ScatterDirichletResidual_Tpetra<panzer::Traits::SGJacobian,TRAITS,LO,GO,NodeT>
  : public panzer::EvaluatorWithBaseImpl<TRAITS>,
    public PHX::EvaluatorDerived<panzer::Traits::SGJacobian, TRAITS>,
    public panzer::CloneableEvaluator  {
  
public:
  ScatterDirichletResidual_Tpetra(const Teuchos::RCP<const UniqueGlobalIndexer<LO,GO> > & indexer)
     : globalIndexer_(indexer) {}
  
  ScatterDirichletResidual_Tpetra(const Teuchos::RCP<const UniqueGlobalIndexer<LO,GO> > & indexer,
                                  const Teuchos::ParameterList& p);
  
  void postRegistrationSetup(typename TRAITS::SetupData d,
			     PHX::FieldManager<TRAITS>& vm);

  void preEvaluate(typename TRAITS::PreEvalData d);
  
  void evaluateFields(typename TRAITS::EvalData workset);

  virtual Teuchos::RCP<CloneableEvaluator> clone(const Teuchos::ParameterList & pl) const
  { return Teuchos::rcp(new ScatterDirichletResidual_Tpetra<panzer::Traits::SGJacobian,TRAITS,LO,GO>(globalIndexer_,pl)); }
  
private:

  typedef typename panzer::Traits::SGJacobian::ScalarT ScalarT;
  typedef SGTpetraLinearObjContainer<double,LO,GO,NodeT> LOC;

  // dummy field so that the evaluator will have something to do
  Teuchos::RCP<PHX::FieldTag> scatterHolder_;

  // fields that need to be scattered will be put in this vector
  std::vector< PHX::MDField<ScalarT,Cell,NODE> > scatterFields_;

  // maps the local (field,element,basis) triplet to a global ID
  // for scattering
  Teuchos::RCP<const panzer::UniqueGlobalIndexer<LO,GO> > globalIndexer_;
  std::vector<int> fieldIds_; // field IDs needing mapping

  // This maps the scattered field names to the DOF manager field
  // For instance a Navier-Stokes map might look like
  //    fieldMap_["RESIDUAL_Velocity"] --> "Velocity"
  //    fieldMap_["RESIDUAL_Pressure"] --> "Pressure"
  Teuchos::RCP<const std::map<std::string,std::string> > fieldMap_;

  std::size_t num_nodes;
  std::size_t num_eq;

  std::size_t side_subcell_dim_;
  std::size_t local_side_id_;

  // Teuchos::RCP<typename LOC::VectorType> dirichletCounter_;

  std::string globalDataKey_; // what global data does this fill?

  Teuchos::RCP<const LOC> sgTpetraContainer_;

  ScatterDirichletResidual_Tpetra();
};

}

// **************************************************************
#endif
#endif // end HAVE_STOKHOS
