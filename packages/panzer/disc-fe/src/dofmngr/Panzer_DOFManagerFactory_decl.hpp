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

#ifndef __Panzer_DOFManagerFactory_decl_hpp__
#define __Panzer_DOFManagerFactory_decl_hpp__

#include "Panzer_config.hpp"

#include "Panzer_UniqueGlobalIndexerFactory.hpp"

namespace panzer {

template <typename LO,typename GO>
class DOFManagerFactory : public virtual UniqueGlobalIndexerFactory<LO,GO,LO,GO> {
public:
   DOFManagerFactory() : useDOFManagerFEI_(false), useTieBreak_(false), enableGhosting_(false) {}

   virtual ~DOFManagerFactory() {}


   /** Use the physics block to construct a unique global indexer object.
     * 
     * \param[in] mpiComm MPI communicator to use in the construction
     * \param[in] physicsBlocks A vector of physics block objects that contain
     *                          unknown field information.
     * \param[in] connMngr Connection manager that contains the mesh topology
     * \param[in] fieldOrder Specifies the local ordering of the degrees of
     *            freedom. This is relevant when degrees of freedom are shared
     *            on the same geometric entity. The default is an alphabetical
     *            ordering.
     *
     * \returns A UniqueGlobalIndexer object. If buildGlobalUnknowns is true,
     *          the object is fully constructed. If it is false, the caller must
     *          finalize it.
     */
   virtual Teuchos::RCP<panzer::UniqueGlobalIndexer<LO,GO> > 
   buildUniqueGlobalIndexer(const Teuchos::RCP<const Teuchos::OpaqueWrapper<MPI_Comm> > & mpiComm,
                            const std::vector<Teuchos::RCP<panzer::PhysicsBlock> > & physicsBlocks,
                            const Teuchos::RCP<ConnManager<LO,GO> > & connMngr,
                            const std::string & fieldOrder="") const;

   void setUseDOFManagerFEI(bool flag)
   { 
     #ifdef PANZER_HAVE_FEI
     useDOFManagerFEI_ = flag; 
     #else
     useDOFManagerFEI_ = flag; 

     Teuchos::FancyOStream out(Teuchos::rcpFromRef(std::cout));
     out.setShowProcRank(false);
     out.setOutputToRootOnly(0);
     out << "*********************************************************************" << std::endl;
     out << "*                                                                   *" << std::endl;
     out << "*  PANZER WARNING: Panzer was not configured with FEI enabled,      *" << std::endl;
     out << "*                  therefore the internal DOFManager will be used.  *" << std::endl;
     out << "*                                                                   *" << std::endl;
     out << "*********************************************************************" << std::endl;
     #endif
   }

   bool getUseDOFManagerFEI() const
   { 
     #ifdef PANZER_HAVE_FEI
     return useDOFManagerFEI_; 
     #else
     return false;
     #endif
   }

   void setUseTieBreak(bool flag) 
   { useTieBreak_ = flag; }

   bool getUseTieBreak() const
   { return useTieBreak_; }

   void setEnableGhosting(bool flag)
   { enableGhosting_ = flag; }

   bool getEnableGhosting() const
   { return enableGhosting_; }

   static void buildFieldOrder(const std::string & fieldOrderStr,std::vector<std::string> & fieldOrder);
protected:
   template <typename DOFManagerT>
   Teuchos::RCP<panzer::UniqueGlobalIndexer<LO,GO> > 
   buildUniqueGlobalIndexer(const Teuchos::RCP<const Teuchos::OpaqueWrapper<MPI_Comm> > & mpiComm,
                            const std::vector<Teuchos::RCP<panzer::PhysicsBlock> > & physicsBlocks,
                            const Teuchos::RCP<ConnManager<LO,GO> > & connMngr,
                            const std::string & fieldOrder) const;

   bool useDOFManagerFEI_;
   bool useTieBreak_;
   bool enableGhosting_;
};

}

#endif
