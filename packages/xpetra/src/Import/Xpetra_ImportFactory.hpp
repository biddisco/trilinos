// @HEADER
//
// ***********************************************************************
//
//             Xpetra: A linear algebra interface package
//                  Copyright 2012 Sandia Corporation
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
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef XPETRA_IMPORTFACTORY_HPP
#define XPETRA_IMPORTFACTORY_HPP

#include "Xpetra_ConfigDefs.hpp"

#include "Xpetra_Import.hpp"

#ifdef HAVE_XPETRA_TPETRA
#include "Xpetra_TpetraImport.hpp"
#endif
#ifdef HAVE_XPETRA_EPETRA
#include "Xpetra_EpetraImport.hpp"
#endif

#include "Xpetra_Exceptions.hpp"

namespace Xpetra {

  template <class LocalOrdinal/* = Import<>::local_ordinal_type*/,
            class GlobalOrdinal/* = typename Import<LocalOrdinal>::global_ordinal_type*/,
            class Node = typename Import<LocalOrdinal, GlobalOrdinal>::node_type>
  class ImportFactory {
  private:
    //! Private constructor. This is a static class.
    ImportFactory() {}

  public:

    //! Constructor specifying the number of non-zeros for all rows.
    static RCP<Import<LocalOrdinal, GlobalOrdinal, Node> > Build(const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &source, const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &target) {
      XPETRA_MONITOR("ImportFactory::Build");

      TEUCHOS_TEST_FOR_EXCEPTION(source->lib() != target->lib(), Xpetra::Exceptions::RuntimeError, "");

#ifdef HAVE_XPETRA_TPETRA
      if (source->lib() == UseTpetra)
        return rcp( new TpetraImport<LocalOrdinal, GlobalOrdinal, Node>(source, target));
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(source->lib());
      XPETRA_FACTORY_END;
    }

  };

  // Specialization on Serial Node (mainly used for Epetra)
#ifdef HAVE_XPETRA_SERIAL

  // Specialization on LO=GO=int with serial node.
  // Used for Epetra and Tpetra
  // For any other node definition the general default implementation is used which allows Tpetra only
  template <>
  class ImportFactory<int, int, Kokkos::Compat::KokkosSerialWrapperNode> {
    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::Compat::KokkosSerialWrapperNode Node;

  private:
    //! Private constructor. This is a static class.
    ImportFactory() {}

  public:

    static RCP<Import<LocalOrdinal, GlobalOrdinal, Node> > Build(const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &source, const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &target) {
      XPETRA_MONITOR("ImportFactory::Build");
      TEUCHOS_TEST_FOR_EXCEPTION(source->lib() != target->lib(), Xpetra::Exceptions::RuntimeError, "");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (source->lib() == UseTpetra)
        return rcp( new TpetraImport<LocalOrdinal, GlobalOrdinal, Node>(source, target));
#else
      XPETRA_TPETRA_ETI_EXCEPTION("ImportFactory<int,int>", "TpetraImport<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (source->lib() == UseEpetra)
        return rcp( new EpetraImportT<int,Node>(source, target));
#endif
#endif

      XPETRA_FACTORY_END;
    }

  };

  // Specialization on LO=int, GO=long long with serial node.
  // Used for Epetra and Tpetra
  // For any other node definition the general default implementation is used which allows Tpetra only
#ifdef HAVE_XPETRA_INT_LONG_LONG
  template <>
  class ImportFactory<int, long long, Kokkos::Compat::KokkosSerialWrapperNode> {

    typedef int LocalOrdinal;
    typedef long long GlobalOrdinal;
    typedef Kokkos::Compat::KokkosSerialWrapperNode Node;

  private:
    //! Private constructor. This is a static class.
    ImportFactory() {}

  public:

    static RCP<Import<LocalOrdinal, GlobalOrdinal, Node> > Build(const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &source, const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &target) {
      XPETRA_MONITOR("ImportFactory::Build");
      TEUCHOS_TEST_FOR_EXCEPTION(source->lib() != target->lib(), Xpetra::Exceptions::RuntimeError, "");

#ifdef HAVE_XPETRA_TPETRA
      if (source->lib() == UseTpetra)
        return rcp( new TpetraImport<LocalOrdinal, GlobalOrdinal, Node>(source, target));
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES
      if (source->lib() == UseEpetra)
        return rcp( new EpetraImportT<long long,Node>(source, target));
#endif
#endif

      XPETRA_FACTORY_END;
    }

  };
#endif // HAVE_XPETRA_INT_LONG_LONG
#endif // HAVE_XPETRA_SERIAL
}

#define XPETRA_IMPORTFACTORY_SHORT
#endif
