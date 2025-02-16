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

#ifndef PANZER_GATHER_SOLUTION_EPETRA_SG_IMPL_HPP
#define PANZER_GATHER_SOLUTION_EPETRA_SG_IMPL_HPP

#include "Panzer_config.hpp"
#ifdef HAVE_STOKHOS

#include "Teuchos_Assert.hpp"
#include "Phalanx_DataLayout.hpp"

#include "Panzer_UniqueGlobalIndexer.hpp"
#include "Panzer_BasisIRLayout.hpp"
#include "Panzer_SGEpetraLinearObjContainer.hpp"

#include "Teuchos_FancyOStream.hpp"

#include "Epetra_Vector.h"
#include "Epetra_Map.h"

// **********************************************************************
// Specialization: SGResidual
// **********************************************************************

template<typename TRAITS,typename LO,typename GO>
panzer::GatherSolution_Epetra<panzer::Traits::SGResidual, TRAITS,LO,GO>::
GatherSolution_Epetra(
  const Teuchos::RCP<const panzer::UniqueGlobalIndexer<LO,GO> > & indexer,
  const Teuchos::ParameterList& p)
  : globalIndexer_(indexer)
  , useTimeDerivativeSolutionVector_(false)
  , globalDataKey_("Solution Gather Container")
{ 
  const std::vector<std::string>& names = 
    *(p.get< Teuchos::RCP< std::vector<std::string> > >("DOF Names"));

  indexerNames_ = p.get< Teuchos::RCP< std::vector<std::string> > >("Indexer Names");

  Teuchos::RCP<panzer::PureBasis> basis = 
    p.get< Teuchos::RCP<panzer::PureBasis> >("Basis");

  gatherFields_.resize(names.size());
  for (std::size_t fd = 0; fd < names.size(); ++fd) {
    gatherFields_[fd] = 
      PHX::MDField<ScalarT,Cell,NODE>(names[fd],basis->functional);
    this->addEvaluatedField(gatherFields_[fd]);
  }

  if (p.isType<bool>("Use Time Derivative Solution Vector"))
    useTimeDerivativeSolutionVector_ = p.get<bool>("Use Time Derivative Solution Vector");

  if (p.isType<std::string>("Global Data Key"))
     globalDataKey_ = p.get<std::string>("Global Data Key");

  this->setName("Gather Solution");
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO> 
void panzer::GatherSolution_Epetra<panzer::Traits::SGResidual, TRAITS,LO,GO>::
postRegistrationSetup(typename TRAITS::SetupData d, 
		      PHX::FieldManager<TRAITS>& fm)
{
  // globalIndexer_ = d.globalIndexer_;
  TEUCHOS_ASSERT(gatherFields_.size() == indexerNames_->size());

  fieldIds_.resize(gatherFields_.size());

  for (std::size_t fd = 0; fd < gatherFields_.size(); ++fd) {
    // get field ID from DOF manager
    //std::string fieldName = gatherFields_[fd].fieldTag().name();
    const std::string& fieldName = (*indexerNames_)[fd];
    fieldIds_[fd] = globalIndexer_->getFieldNum(fieldName);

    // setup the field data object
    this->utils.setFieldData(gatherFields_[fd],fm);
  }

  indexerNames_ = Teuchos::null;  // Don't need this anymore
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::GatherSolution_Epetra<panzer::Traits::SGResidual, TRAITS,LO,GO>::
preEvaluate(typename TRAITS::PreEvalData d)
{
   // extract linear object container
   sgEpetraContainer_ = Teuchos::rcp_dynamic_cast<SGEpetraLinearObjContainer>(d.gedc.getDataObject(globalDataKey_),true);
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::GatherSolution_Epetra<panzer::Traits::SGResidual, TRAITS,LO,GO>::
evaluateFields(typename TRAITS::EvalData workset)
{ 
   std::vector<GO> GIDs;
   std::vector<int> LIDs;
 
   // for convenience pull out some objects from workset
   std::string blockId = this->wda(workset).block_id;
   const std::vector<std::size_t> & localCellIds = this->wda(workset).cell_local_ids;

   Teuchos::RCP<Stokhos::OrthogPolyExpansion<int,double> > expansion = sgEpetraContainer_->getExpansion();

   Teuchos::RCP<Epetra_Vector> x_template; // this will be used to map from GIDs --> LIDs
   if (useTimeDerivativeSolutionVector_)
     x_template = (*sgEpetraContainer_->begin())->get_dxdt();
   else
     x_template = (*sgEpetraContainer_->begin())->get_x(); 
 
   // NOTE: A reordering of these loops will likely improve performance
   //       The "getGIDFieldOffsets may be expensive.  However the
   //       "getElementGIDs" can be cheaper. However the lookup for LIDs
   //       may be more expensive!
 
   // gather operation for each cell in workset
   for(std::size_t worksetCellIndex=0;worksetCellIndex<localCellIds.size();++worksetCellIndex) {
      std::size_t cellLocalId = localCellIds[worksetCellIndex];
 
      globalIndexer_->getElementGIDs(cellLocalId,GIDs,blockId); 
 
      // caculate the local IDs for this element
      LIDs.resize(GIDs.size());
      for(std::size_t i=0;i<GIDs.size();i++)
         LIDs[i] = x_template->Map().LID(GIDs[i]);
 
      // loop over the fields to be gathered
      for (std::size_t fieldIndex=0; fieldIndex<gatherFields_.size();fieldIndex++) {
         int fieldNum = fieldIds_[fieldIndex];
         const std::vector<int> & elmtOffset = globalIndexer_->getGIDFieldOffsets(blockId,fieldNum);
 
         // loop over basis functions and fill the fields
         for(std::size_t basis=0;basis<elmtOffset.size();basis++) {
            int offset = elmtOffset[basis];
            int lid = LIDs[offset];

            PHX::MDField<ScalarT,Cell,NODE> field = (gatherFields_[fieldIndex]);
            // ScalarT & field = (gatherFields_[fieldIndex])(worksetCellIndex,basis);
            field(worksetCellIndex,basis).reset(expansion); // jamb in expansion here
            field(worksetCellIndex,basis).copyForWrite(); 

            // loop over stochastic basis initialzing field gather values
            int stochIndex = 0;
            panzer::SGEpetraLinearObjContainer::iterator itr; 
            for(itr=sgEpetraContainer_->begin();itr!=sgEpetraContainer_->end();++itr,++stochIndex) {
               // extract solution and time derivative vectors
               Teuchos::RCP<Epetra_Vector> x;
               if (useTimeDerivativeSolutionVector_)
                 x = (*itr)->get_dxdt();
               else
                 x = (*itr)->get_x(); 

               field(worksetCellIndex,basis).fastAccessCoeff(stochIndex) = (*x)[lid];
            }
         }
      }
   }
}

// **********************************************************************
// Specialization: SGJacobian
// **********************************************************************

template<typename TRAITS,typename LO,typename GO>
panzer::GatherSolution_Epetra<panzer::Traits::SGJacobian, TRAITS,LO,GO>::
GatherSolution_Epetra(
  const Teuchos::RCP<const panzer::UniqueGlobalIndexer<LO,GO> > & indexer,
  const Teuchos::ParameterList& p)
  : globalIndexer_(indexer)
  , useTimeDerivativeSolutionVector_(false)
  , globalDataKey_("Solution Gather Container")
{ 
  const std::vector<std::string>& names = 
    *(p.get< Teuchos::RCP< std::vector<std::string> > >("DOF Names"));

  indexerNames_ = p.get< Teuchos::RCP< std::vector<std::string> > >("Indexer Names");

  Teuchos::RCP<PHX::DataLayout> dl = 
    p.get< Teuchos::RCP<panzer::PureBasis> >("Basis")->functional;

  gatherFields_.resize(names.size());
  for (std::size_t fd = 0; fd < names.size(); ++fd) {
    PHX::MDField<ScalarT,Cell,NODE> f(names[fd],dl);
    gatherFields_[fd] = f;
    this->addEvaluatedField(gatherFields_[fd]);
  }

  if (p.isType<bool>("Use Time Derivative Solution Vector"))
    useTimeDerivativeSolutionVector_ = p.get<bool>("Use Time Derivative Solution Vector");

  if (p.isType<std::string>("Global Data Key"))
     globalDataKey_ = p.get<std::string>("Global Data Key");


  this->setName("Gather Solution");
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO> 
void panzer::GatherSolution_Epetra<panzer::Traits::SGJacobian, TRAITS,LO,GO>::
postRegistrationSetup(typename TRAITS::SetupData d, 
		      PHX::FieldManager<TRAITS>& fm)
{
  // globalIndexer_ = d.globalIndexer_;
  TEUCHOS_ASSERT(gatherFields_.size() == indexerNames_->size());

  fieldIds_.resize(gatherFields_.size());

  for (std::size_t fd = 0; fd < gatherFields_.size(); ++fd) {
    // get field ID from DOF manager
    //std::string fieldName = gatherFields_[fd].fieldTag().name();
    const std::string& fieldName = (*indexerNames_)[fd];
    fieldIds_[fd] = globalIndexer_->getFieldNum(fieldName);

    // setup the field data object
    this->utils.setFieldData(gatherFields_[fd],fm);
  }

  indexerNames_ = Teuchos::null;  // Don't need this anymore
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::GatherSolution_Epetra<panzer::Traits::SGJacobian, TRAITS,LO,GO>::
preEvaluate(typename TRAITS::PreEvalData d)
{
   // extract linear object container
   sgEpetraContainer_ = Teuchos::rcp_dynamic_cast<SGEpetraLinearObjContainer>(d.gedc.getDataObject(globalDataKey_),true);
}

// **********************************************************************
template<typename TRAITS,typename LO,typename GO>
void panzer::GatherSolution_Epetra<panzer::Traits::SGJacobian, TRAITS,LO,GO>::
evaluateFields(typename TRAITS::EvalData workset)
{ 
   std::vector<GO> GIDs;
   std::vector<int> LIDs;

   // for convenience pull out some objects from workset
   std::string blockId = this->wda(workset).block_id;
   const std::vector<std::size_t> & localCellIds = this->wda(workset).cell_local_ids;

   Teuchos::RCP<Stokhos::OrthogPolyExpansion<int,double> > expansion = sgEpetraContainer_->getExpansion();

   Teuchos::RCP<Epetra_Vector> x_template;
   double seed_value = 0.0;
   if (useTimeDerivativeSolutionVector_) {
     x_template = (*sgEpetraContainer_->begin())->get_dxdt();
     seed_value = workset.alpha;
   }
   else {
     x_template = (*sgEpetraContainer_->begin())->get_x(); 
     seed_value = workset.beta;
   }

   // NOTE: A reordering of these loops will likely improve performance
   //       The "getGIDFieldOffsets may be expensive.  However the
   //       "getElementGIDs" can be cheaper. However the lookup for LIDs
   //       may be more expensive!

   // gather operation for each cell in workset
   for(std::size_t worksetCellIndex=0;worksetCellIndex<localCellIds.size();++worksetCellIndex) {
      std::size_t cellLocalId = localCellIds[worksetCellIndex];

      globalIndexer_->getElementGIDs(cellLocalId,GIDs,blockId); 

      // caculate the local IDs for this element
      LIDs.resize(GIDs.size());
      for(std::size_t i=0;i<GIDs.size();i++)
        LIDs[i] = x_template->Map().LID(GIDs[i]);

      // loop over the fields to be gathered
      for(std::size_t fieldIndex=0;
          fieldIndex<gatherFields_.size();fieldIndex++) {
         int fieldNum = fieldIds_[fieldIndex];
         const std::vector<int> & elmtOffset = globalIndexer_->getGIDFieldOffsets(blockId,fieldNum);

         // loop over basis functions and fill the fields
         for(std::size_t basis=0;basis<elmtOffset.size();basis++) {
            int offset = elmtOffset[basis];
            int lid = LIDs[offset];

            PHX::MDField<ScalarT,Cell,NODE> field = (gatherFields_[fieldIndex]);
            // ScalarT & field = (gatherFields_[fieldIndex])(worksetCellIndex,basis);

            field(worksetCellIndex,basis) = ScalarT(GIDs.size(), 0.0);

            // set the value and seed the FAD object
            field(worksetCellIndex,basis).fastAccessDx(offset) = seed_value;
            field(worksetCellIndex,basis).val().reset(expansion);
            field(worksetCellIndex,basis).val().copyForWrite();

            // loop over stochastic basis initialzing field gather values
            int stochIndex = 0;
            panzer::SGEpetraLinearObjContainer::iterator itr; 
            for(itr=sgEpetraContainer_->begin();itr!=sgEpetraContainer_->end();++itr,++stochIndex) {
               // extract solution and time derivative vectors
               Teuchos::RCP<Epetra_Vector> x;
               if (useTimeDerivativeSolutionVector_)
                 x = (*itr)->get_dxdt();
               else
                 x = (*itr)->get_x(); 

               field(worksetCellIndex,basis).val().fastAccessCoeff(stochIndex) = (*x)[lid];
            }
         }
      }
   }
}

// **********************************************************************
#endif // end HAVE_STOKHOS

#endif 
