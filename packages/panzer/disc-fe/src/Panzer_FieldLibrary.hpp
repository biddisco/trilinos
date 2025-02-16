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

#ifndef __Panzer_FieldLibrary_hpp__
#define __Panzer_FieldLibrary_hpp__

#include "Panzer_config.hpp"

#include "Teuchos_RCP.hpp"

#include "Panzer_BasisIRLayout.hpp"
#include "Panzer_PointRule.hpp"
#include "Panzer_PureBasis.hpp"

#include <vector>

namespace panzer {

class FieldLibraryBase {
public:
   virtual ~FieldLibraryBase() = 0;

   //! Get the basis associated with a particular field.
   virtual Teuchos::RCP<const panzer::PureBasis> lookupBasis(const std::string & fieldName) const = 0;

   //! Get vector of unique bases contained in this field library
   virtual void uniqueBases(std::vector<Teuchos::RCP<const panzer::PureBasis> > & bases) const = 0;

   //! Get vector of unique bases contained in this field library
   virtual void basisPairs(std::vector<std::pair<std::string,Teuchos::RCP<const panzer::PureBasis> > > & bases) const = 0;

   /** Print information about the basis functions and fields contained in
     * the field library.
     */
   virtual void print(std::ostream & os) const = 0;
};

inline FieldLibraryBase::~FieldLibraryBase() {}

/** There is one of these objects per equation set.
  */
class FieldLayoutLibrary : public FieldLibraryBase {
public:
   /** Add a field associated with a basis to the library.
     */
   void addFieldAndLayout(const std::string & fieldName,
                         const Teuchos::RCP<panzer::BasisIRLayout> & basis);   

   //! Get vector of unique bases contained in this field library
   void uniqueBases(std::vector<Teuchos::RCP<const panzer::PureBasis> > & bases) const;

   //! Get the basis associated with a particular field.
   virtual Teuchos::RCP<const panzer::PureBasis> lookupBasis(const std::string & fieldName) const;

   //! Get the basis associated with a particular field.
   Teuchos::RCP<panzer::BasisIRLayout> lookupLayout(const std::string & fieldName) const;

   /** Print information about the basis functions and fields contained in
     * the field library.
     */
   virtual void print(std::ostream & os) const;

   //! Get vector of unique bases contained in this field library
   virtual void basisPairs(std::vector<std::pair<std::string,Teuchos::RCP<const panzer::PureBasis> > > & bases) const;

private:

   //! Basic mapped storage.
   std::map<std::string,Teuchos::RCP<panzer::BasisIRLayout> > fieldToLayout_;
   std::map<std::string,Teuchos::RCP<const panzer::PureBasis> > basisNameToPointer_; // to satisfy uniuqeBases interface

};

/** Build a container that holds, and provides
  * easy lookups for each fields basis. This provides
  * further functionality by providing a class that
  * oversees the marriage of the basis and integration
  * rule objects. There is one of these objects per
  * physics block.
  */
class FieldLibrary : public FieldLibraryBase {
public:

   //! Get the basis associated with a particular field.
   virtual Teuchos::RCP<const panzer::PureBasis> lookupBasis(const std::string & fieldName) const;

   //! Get vector of unique bases contained in this field library
   void uniqueBases(std::vector<Teuchos::RCP<const panzer::PureBasis> > & bases) const;

   /** Add a field associated witha basis to the library.
     */
   void addFieldAndBasis(const std::string & fieldName,
                 const Teuchos::RCP<panzer::PureBasis> & basis);   

   /** Given an integration rule build a FieldLayoutLibrary which
     * oversees the marriage of the integration rule and the basis
     * into a BasisIRLayout.
     */
   Teuchos::RCP<const FieldLayoutLibrary> buildFieldLayoutLibrary(panzer::PointRule & ir) const;

   /** Print information about the basis functions and fields contained in
     * the field library.
     */
   virtual void print(std::ostream & os) const;

   //! Get vector of unique bases contained in this field library
   virtual void basisPairs(std::vector<std::pair<std::string,Teuchos::RCP<const panzer::PureBasis> > > & bases) const;

private:

   //! Basic mapped storage.
   std::map<std::string,Teuchos::RCP<panzer::PureBasis> > fieldToBasis_;
   std::map<std::string,Teuchos::RCP<const panzer::PureBasis> > basisNameToPointer_; // to satisfy uniuqeBases interface
};

inline std::ostream & operator<<(std::ostream & os,const FieldLibraryBase & flb)
{
   flb.print(os);
   return os;
}

}

#endif
