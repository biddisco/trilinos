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


#include <Teuchos_TimeMonitor.hpp>
#include <Panzer_config.hpp>

#include "Panzer_STK_PamgenReaderFactory.hpp"
#include "Panzer_STK_Interface.hpp"

#ifdef HAVE_IOSS 

#include <Ionit_Initializer.h>
#include <Ioss_ElementBlock.h>
#include <Ioss_Region.h>
#include <stk_io/MeshReadWriteUtils.hpp>
#include <stk_io/IossBridge.hpp>

#include "Teuchos_StandardParameterEntryValidators.hpp"

namespace panzer_stk_classic {

STK_PamgenReaderFactory::STK_PamgenReaderFactory()
   : fileName_(""), restartIndex_(0), useLowerCase_(false)
{ }

STK_PamgenReaderFactory::STK_PamgenReaderFactory(const std::string & fileName,int restartIndex)
   : fileName_(fileName), restartIndex_(restartIndex), useLowerCase_(false)
{ }

Teuchos::RCP<STK_Interface> STK_PamgenReaderFactory::buildMesh(stk_classic::ParallelMachine parallelMach) const
{
   PANZER_FUNC_TIME_MONITOR("panzer::STK_PamgenReaderFactory::buildMesh()");

   using Teuchos::RCP;
   using Teuchos::rcp;
   typedef stk_classic::mesh::Field<double,stk_classic::mesh::Cartesian> VectorFieldType;
   
   RCP<STK_Interface> mesh = buildUncommitedMesh(parallelMach);

   // in here you would add your fields...but it is better to use
   // the two step construction

   // this calls commit on meta data
   mesh->initialize(parallelMach,false);

   completeMeshConstruction(*mesh,parallelMach); 

   return mesh;
}

/** This builds all the meta data of the mesh. Does not call metaData->commit.
  * Allows user to add solution fields and other pieces. The mesh can be "completed"
  * by calling <code>completeMeshConstruction</code>.
  */
Teuchos::RCP<STK_Interface> STK_PamgenReaderFactory::buildUncommitedMesh(stk_classic::ParallelMachine parallelMach) const 
{ 
   PANZER_FUNC_TIME_MONITOR("panzer::STK_PamgenReaderFactory::buildUncomittedMesh()");

   using Teuchos::RCP;
   using Teuchos::rcp;

   RCP<STK_Interface> mesh = rcp(new STK_Interface());

   // immediately setup lower case usage
   mesh->setUseLowerCaseForIO(useLowerCase_);

   RCP<stk_classic::mesh::fem::FEMMetaData> femMetaData = mesh->getMetaData();
   stk_classic::mesh::MetaData & metaData = stk_classic::mesh::fem::FEMMetaData::get_meta_data(*femMetaData);

   // read in meta data
   Ioss::Init::Initializer io;
   stk_classic::io::MeshData * meshData = new stk_classic::io::MeshData;
   stk_classic::io::create_input_mesh("pamgen", fileName_, parallelMach,
                                    *femMetaData, *meshData,useLowerCase_);

   // add in "FAMILY_TREE" entity for doing refinement
   std::size_t dimension = femMetaData->spatial_dimension();
   std::vector<std::string> entity_rank_names = stk_classic::mesh::fem::entity_rank_names(dimension);
   entity_rank_names.push_back("FAMILY_TREE");
   femMetaData->set_entity_rank_names(entity_rank_names);

   // read in other transient fields, these will be useful later when
   // trying to read other fields for use in solve
   stk_classic::io::define_input_fields(*meshData,*femMetaData);

   // store mesh data pointer for later use in initializing 
   // bulk data
   metaData.declare_attribute_with_delete(meshData);

   mesh->initializeFromMetaData();

   // build element blocks
   registerElementBlocks(*mesh,*meshData);
   registerSidesets(*mesh,*meshData);
   registerNodesets(*mesh,*meshData);

   mesh->addPeriodicBCs(periodicBCVec_);

   return mesh; 
}

void STK_PamgenReaderFactory::completeMeshConstruction(STK_Interface & mesh,stk_classic::ParallelMachine parallelMach) const
{
   PANZER_FUNC_TIME_MONITOR("panzer::STK_PamgenReaderFactory::completeMeshConstruction()");

   using Teuchos::RCP;
   using Teuchos::rcp;

   if(not mesh.isInitialized())
      mesh.initialize(parallelMach);

   // grab mesh data pointer to build the bulk data
   stk_classic::mesh::MetaData & metaData = stk_classic::mesh::fem::FEMMetaData::get_meta_data(*mesh.getMetaData());
   stk_classic::io::MeshData * meshData = 
         const_cast<stk_classic::io::MeshData *>(metaData.get_attribute<stk_classic::io::MeshData>());
         // if const_cast is wrong ... why does it feel so right?
         // I believe this is safe since we are basically hiding this object under the covers
         // until the mesh construction can be completed...below I cleanup the object myself.
   TEUCHOS_ASSERT(metaData.remove_attribute(meshData)); 
      // remove the MeshData attribute

   RCP<stk_classic::mesh::BulkData> bulkData = mesh.getBulkData();

   // build mesh bulk data
   mesh.beginModification();
   stk_classic::io::populate_bulk_data(*bulkData, *meshData);
   mesh.endModification();

   // put in a negative index and (like python) the restart will be from the back
   // (-1 is the last time step)
   int restartIndex = restartIndex_;
   if(restartIndex<0) {
     std::pair<int,double> lastTimeStep = meshData->m_input_region->get_max_time();
     restartIndex = 1+restartIndex+lastTimeStep.first;
   }

   // populate mesh fields with specific index
   stk_classic::io::process_input_request(*meshData,*bulkData,restartIndex);

   mesh.buildSubcells();
   mesh.buildLocalElementIDs();

   if(restartIndex>0) // process_input_request is a no-op if restartIndex<=0 ... thus there would be no inital time
      mesh.setInitialStateTime(meshData->m_input_region->get_state_time(restartIndex));
   else
      mesh.setInitialStateTime(0.0); // no initial time to speak, might as well use 0.0

   // clean up mesh data object
   delete meshData;

   // calls Stk_MeshFactory::rebalance
   this->rebalance(mesh);
}

//! From ParameterListAcceptor
void STK_PamgenReaderFactory::setParameterList(const Teuchos::RCP<Teuchos::ParameterList> & paramList)
{
   TEUCHOS_TEST_FOR_EXCEPTION_PURE_MSG(!paramList->isParameter("File Name")
&& !paramList->isParameter("Text Input"),
        Teuchos::Exceptions::InvalidParameterName,
        "Error, either parameter {name=\"File Name\","
        "or the parameter {name=\"Text Input\","
        "type=\"string\""
        "\nis required in parameter (sub)list \""<< paramList->name() <<"\"."
        "\n\nThe parsed parameter parameter list is: \n" << paramList->currentParametersString()
   );
      
   paramList->validateParametersAndSetDefaults(*getValidParameters(),0); 

   setMyParamList(paramList);

   fileName_ = paramList->get<std::string>("File Name");

   restartIndex_ = paramList->get<int>("Restart Index");

   useLowerCase_ = paramList->get<bool>("Use Lower Case");

   // read in periodic boundary conditions
   parsePeriodicBCList(Teuchos::rcpFromRef(paramList->sublist("Periodic BCs")),periodicBCVec_);
}

//! From ParameterListAcceptor
Teuchos::RCP<const Teuchos::ParameterList> STK_PamgenReaderFactory::getValidParameters() const
{
   static Teuchos::RCP<Teuchos::ParameterList> validParams;

   if(validParams==Teuchos::null) {
      validParams = Teuchos::rcp(new Teuchos::ParameterList);
      validParams->set<std::string>("File Name","<file name not set>","Name of pamgen file to be read", 
                                    Teuchos::rcp(new Teuchos::FileNameValidator));
      
      validParams->set<int>("Restart Index",-1,"Index of solution to read in", 
			    Teuchos::rcp(new Teuchos::AnyNumberParameterEntryValidator(Teuchos::AnyNumberParameterEntryValidator::PREFER_INT,Teuchos::AnyNumberParameterEntryValidator::AcceptedTypes(true))));

      validParams->set<bool>("Use Lower Case",false,"Convert fields to lower case for Exodus I/O.");

      Teuchos::ParameterList & bcs = validParams->sublist("Periodic BCs");
      bcs.set<int>("Count",0); // no default periodic boundary conditions
   }

   return validParams.getConst();
}

void STK_PamgenReaderFactory::registerElementBlocks(STK_Interface & mesh,stk_classic::io::MeshData & meshData) const 
{
   using Teuchos::RCP;

   RCP<stk_classic::mesh::fem::FEMMetaData> femMetaData = mesh.getMetaData();

   // here we use the Ioss interface because they don't add
   // "bonus" element blocks and its easier to determine
   // "real" element blocks versus STK-only blocks
   const Ioss::ElementBlockContainer & elem_blocks = meshData.m_input_region->get_element_blocks();
   for(Ioss::ElementBlockContainer::const_iterator itr=elem_blocks.begin();itr!=elem_blocks.end();++itr) {
      Ioss::GroupingEntity * entity = *itr;
      const std::string & name = entity->name(); 

      const stk_classic::mesh::Part * part = femMetaData->get_part(name);
      const CellTopologyData * ct = femMetaData->get_cell_topology(*part).getCellTopologyData();

      TEUCHOS_ASSERT(ct!=0);
      mesh.addElementBlock(part->name(),ct);
   }
}

template <typename SetType>
void buildSetNames(const SetType & setData,std::vector<std::string> & names)
{
   // pull out all names for this set
   for(typename SetType::const_iterator itr=setData.begin();itr!=setData.end();++itr) {
      Ioss::GroupingEntity * entity = *itr;
      names.push_back(entity->name());
   }
}

void STK_PamgenReaderFactory::registerSidesets(STK_Interface & mesh,stk_classic::io::MeshData & meshData) const
{
   using Teuchos::RCP;

   RCP<stk_classic::mesh::fem::FEMMetaData> metaData = mesh.getMetaData();
   const stk_classic::mesh::PartVector & parts = metaData->get_parts();

   stk_classic::mesh::PartVector::const_iterator partItr;
   for(partItr=parts.begin();partItr!=parts.end();++partItr) {
      const stk_classic::mesh::Part * part = *partItr;
      const stk_classic::mesh::PartVector & subsets = part->subsets();
      // const CellTopologyData * ct = stk_classic::mesh::fem::get_cell_topology(*part).getCellTopologyData();
      const CellTopologyData * ct = metaData->get_cell_topology(*part).getCellTopologyData();

      // if a side part ==> this is a sideset: now storage is recursive
      // on part contains all sub parts with consistent topology
      if(part->primary_entity_rank()==mesh.getSideRank() && ct==0 && subsets.size()>0) {
         TEUCHOS_TEST_FOR_EXCEPTION(subsets.size()!=1,std::runtime_error,
                            "STK_PamgenReaderFactory::registerSidesets error - part \"" << part->name() << 
                            "\" has more than one subset"); 

         // grab cell topology and name of subset part
         const stk_classic::mesh::Part * ss_part = subsets[0];
         // const CellTopologyData * ss_ct = stk_classic::mesh::fem::get_cell_topology(*ss_part).getCellTopologyData();
         const CellTopologyData * ss_ct = metaData->get_cell_topology(*ss_part).getCellTopologyData();
 
         // only add subset parts that have no topology
         if(ss_ct!=0) 
            mesh.addSideset(part->name(),ss_ct);
      }
   }
}

void STK_PamgenReaderFactory::registerNodesets(STK_Interface & mesh,stk_classic::io::MeshData & meshData) const
{
   using Teuchos::RCP;

   RCP<stk_classic::mesh::fem::FEMMetaData> metaData = mesh.getMetaData();
   const stk_classic::mesh::PartVector & parts = metaData->get_parts();

   stk_classic::mesh::PartVector::const_iterator partItr;
   for(partItr=parts.begin();partItr!=parts.end();++partItr) {
      const stk_classic::mesh::Part * part = *partItr;
      const CellTopologyData * ct = metaData->get_cell_topology(*part).getCellTopologyData();

      // if a side part ==> this is a sideset: now storage is recursive
      // on part contains all sub parts with consistent topology
      if(part->primary_entity_rank()==mesh.getNodeRank() && ct==0) {

         // only add subset parts that have no topology
         if(part->name()!=STK_Interface::nodesString)
            mesh.addNodeset(part->name());
      }
   }
}

}

#endif
