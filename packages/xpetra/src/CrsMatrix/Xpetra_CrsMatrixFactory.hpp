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
#ifndef XPETRA_CRSMATRIXFACTORY_HPP
#define XPETRA_CRSMATRIXFACTORY_HPP

#include "Xpetra_ConfigDefs.hpp"

#include "Xpetra_CrsMatrix.hpp"

#ifdef HAVE_XPETRA_TPETRA
#include "Xpetra_TpetraCrsMatrix.hpp"
#endif

#ifdef HAVE_XPETRA_EPETRA
#include "Xpetra_EpetraCrsMatrix.hpp"
#endif

#include "Xpetra_Exceptions.hpp"

namespace Xpetra {

  template <class Scalar /*= CrsMatrix<>::scalar_type*/,
            class LocalOrdinal /*=
              typename CrsMatrix<Scalar>::local_ordinal_type*/,
            class GlobalOrdinal /*=
              typename CrsMatrix<Scalar, LocalOrdinal>::global_ordinal_type*/,
            class Node /*=
              typename CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal>::node_type*/>
  class CrsMatrixFactory {
  private:
    //! Private constructor. This is a static class.
    CrsMatrixFactory() {}

  public:
    //! Constructor specifying fixed number of entries for each row.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> >
    Build (const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &rowMap,
           size_t maxNumEntriesPerRow,
           Xpetra::ProfileType pftype = Xpetra::DynamicProfile,
           const Teuchos::RCP<Teuchos::ParameterList>& plist = Teuchos::null)
    {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, maxNumEntriesPerRow, pftype, plist) );
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(rowMap->lib());
      XPETRA_FACTORY_END;
    }

    //! Constructor specifying (possibly different) number of entries in each row.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> >
    Build (const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >& rowMap,
           const ArrayRCP<const size_t>& NumEntriesPerRowToAlloc,
           ProfileType pftype = Xpetra::DynamicProfile,
           const Teuchos::RCP<Teuchos::ParameterList>& plist = Teuchos::null)
    {
#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, NumEntriesPerRowToAlloc, pftype, plist) );
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(rowMap->lib());
      XPETRA_FACTORY_END;
    }

    //! Constructor specifying column Map and fixed number of entries for each row.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> >
    Build (const Teuchos::RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> >& rowMap,
           const Teuchos::RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> >& colMap,
           size_t maxNumEntriesPerRow,
           ProfileType pftype = DynamicProfile,
           const Teuchos::RCP<Teuchos::ParameterList>& plist = Teuchos::null)
    {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, colMap, maxNumEntriesPerRow, pftype, plist) );
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(rowMap->lib());
      XPETRA_FACTORY_END;
    }

    //! Constructor specifying column Map and number of entries in each row.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &colMap, const ArrayRCP< const size_t > &NumEntriesPerRowToAlloc, ProfileType pftype=DynamicProfile, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, colMap, NumEntriesPerRowToAlloc, pftype, plist) );
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(rowMap->lib());
      XPETRA_FACTORY_END;
    }

    //! Constructor specifying a previously constructed graph.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const CrsGraph< LocalOrdinal, GlobalOrdinal, Node > > &graph, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (graph->getRowMap()->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(graph, plist) );
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(graph->getRowMap()->lib());
      XPETRA_FACTORY_END;
    }


    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const CrsMatrix< Scalar, LocalOrdinal, GlobalOrdinal, Node > > &sourceMatrix, const Import<LocalOrdinal,GlobalOrdinal,Node> &importer, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & domainMap = Teuchos::null, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & rangeMap = Teuchos::null,const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (sourceMatrix->getRowMap()->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(sourceMatrix,importer,domainMap,rangeMap,params));
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(sourceMatrix->getRowMap()->lib());
      XPETRA_FACTORY_END;
    }

    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const CrsMatrix< Scalar, LocalOrdinal, GlobalOrdinal, Node > > &sourceMatrix, const Export<LocalOrdinal,GlobalOrdinal,Node> &exporter, const RCP<Map<LocalOrdinal,GlobalOrdinal,Scalar> > & domainMap = Teuchos::null, const RCP<Map<LocalOrdinal,GlobalOrdinal,Scalar> > & rangeMap = Teuchos::null,const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (sourceMatrix->getRowMap()->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(sourceMatrix,exporter,domainMap,rangeMap,params));
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(sourceMatrix->getRowMap()->lib());
      XPETRA_FACTORY_END;
    }

#ifdef HAVE_XPETRA_KOKKOS_REFACTOR
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build (
        const Teuchos::RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> >& rowMap,
        const Teuchos::RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> >& colMap,
        const typename Xpetra::CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>::local_matrix_type& lclMatrix,
        const Teuchos::RCP<Teuchos::ParameterList>& params = null)  {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, colMap, lclMatrix, params));
#endif

      XPETRA_FACTORY_ERROR_IF_EPETRA(rowMap->lib());
      XPETRA_FACTORY_END;
    }
#endif

  };

  // Specializations on SerialNode (mainly for Epetra)
#ifdef HAVE_XPETRA_SERIAL

  // Specializtion for SC=double, LO=int, GO=int and Node=Serial
  // Used both for Epetra and Tpetra
  template <>
  class CrsMatrixFactory<double, int, int, Kokkos::Compat::KokkosSerialWrapperNode> {
    typedef double Scalar;
    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::Compat::KokkosSerialWrapperNode Node;

  private:
    //! Private constructor. This is a static class.
    CrsMatrixFactory() {}

  public:

    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &rowMap, size_t maxNumEntriesPerRow, Xpetra::ProfileType pftype = Xpetra::DynamicProfile, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, maxNumEntriesPerRow, pftype, plist) );
#else
      XPETRA_TPETRA_ETI_EXCEPTION("CrsMatrixFactory<int,int>", "TpetraCrsMatrix<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<int,Node>(rowMap, maxNumEntriesPerRow, pftype, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const ArrayRCP< const size_t > &NumEntriesPerRowToAlloc, ProfileType pftype = Xpetra::DynamicProfile,  const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, NumEntriesPerRowToAlloc, pftype, plist) );
#else
      XPETRA_TPETRA_ETI_EXCEPTION("CrsMatrixFactory<int,int>", "TpetraCrsMatrix<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<int,Node>(rowMap, NumEntriesPerRowToAlloc, pftype, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    //! Constructor specifying column Map and fixed number of entries for each row.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &colMap, size_t maxNumEntriesPerRow, ProfileType pftype=DynamicProfile, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, colMap, maxNumEntriesPerRow, pftype, plist) );
#else
      XPETRA_TPETRA_ETI_EXCEPTION("CrsMatrixFactory<int,int>", "TpetraCrsMatrix<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<int,Node>(rowMap, colMap, maxNumEntriesPerRow, pftype, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    //! Constructor specifying column Map and number of entries in each row.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &colMap, const ArrayRCP< const size_t > &NumEntriesPerRowToAlloc, ProfileType pftype=DynamicProfile, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, colMap, NumEntriesPerRowToAlloc, pftype, plist) );
#else
      XPETRA_TPETRA_ETI_EXCEPTION("CrsMatrixFactory<int,int>", "TpetraCrsMatrix<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<int,Node>(rowMap, colMap, NumEntriesPerRowToAlloc, pftype, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    //! Constructor specifying a previously constructed graph.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const CrsGraph< LocalOrdinal, GlobalOrdinal, Node > > &graph, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (graph->getRowMap()->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(graph, plist) );
#else
      XPETRA_TPETRA_ETI_EXCEPTION("CrsMatrixFactory<int,int>", "TpetraCrsMatrix<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (graph->getRowMap()->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<int,Node>(graph, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }


    //! Constructor using FusedImport
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const CrsMatrix< Scalar, LocalOrdinal, GlobalOrdinal, Node > > &sourceMatrix, const Import<LocalOrdinal,GlobalOrdinal,Node> &importer, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & domainMap = Teuchos::null, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & rangeMap = Teuchos::null,const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (sourceMatrix->getRowMap()->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(sourceMatrix,importer,domainMap,rangeMap,params) );
#else
      XPETRA_TPETRA_ETI_EXCEPTION("CrsMatrixFactory<int,int>", "TpetraCrsMatrix<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (sourceMatrix->getRowMap()->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<int,Node>(sourceMatrix,importer,domainMap,rangeMap,params) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    //! Constructor using FusedExport
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const CrsMatrix< Scalar, LocalOrdinal, GlobalOrdinal, Node > > &sourceMatrix, const Export<LocalOrdinal,GlobalOrdinal,Node> &exporter, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & domainMap = Teuchos::null, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & rangeMap = Teuchos::null,const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (sourceMatrix->getRowMap()->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(sourceMatrix,exporter,domainMap,rangeMap,params) );
#else
      XPETRA_TPETRA_ETI_EXCEPTION("CrsMatrixFactory<int,int>", "TpetraCrsMatrix<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (sourceMatrix->getRowMap()->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<int,Node>(sourceMatrix,exporter,domainMap,rangeMap,params) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

#ifdef HAVE_XPETRA_KOKKOS_REFACTOR
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build (
        const Teuchos::RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> >& rowMap,
        const Teuchos::RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> >& colMap,
        const typename Xpetra::CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>::local_matrix_type& lclMatrix,
        const Teuchos::RCP<Teuchos::ParameterList>& params = null)  {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
#ifdef HAVE_XPETRA_TPETRA_INST_INT_INT
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, colMap, lclMatrix, params));
#else
      XPETRA_TPETRA_ETI_EXCEPTION("CrsMatrixFactory<int,int>", "TpetraCrsMatrix<int,int>", "int");
#endif
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<int,Node>(rowMap, colMap, lclMatrix, params) );
#endif
#endif
      XPETRA_FACTORY_END;
    }
#endif

  };

  // Specializtion for SC=double, LO=int, GO=long long and Node=Serial
  // Used both for Epetra and Tpetra
#ifdef HAVE_XPETRA_INT_LONG_LONG
  template <>
  class CrsMatrixFactory<double, int, long long, Kokkos::Compat::KokkosSerialWrapperNode> {
    typedef double Scalar;
    typedef int LocalOrdinal;
    typedef long long GlobalOrdinal;
    typedef Kokkos::Compat::KokkosSerialWrapperNode Node;

  private:
    //! Private constructor. This is a static class.
    CrsMatrixFactory() {}

  public:

    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> > &rowMap, size_t maxNumEntriesPerRow, Xpetra::ProfileType pftype = Xpetra::DynamicProfile, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, maxNumEntriesPerRow, pftype, plist) );
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<long long, Node>(rowMap, maxNumEntriesPerRow, pftype, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const ArrayRCP< const size_t > &NumEntriesPerRowToAlloc, ProfileType pftype = Xpetra::DynamicProfile,  const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, NumEntriesPerRowToAlloc, pftype, plist) );
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<long long, Node>(rowMap, NumEntriesPerRowToAlloc, pftype, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    //! Constructor specifying column Map and fixed number of entries for each row.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &colMap, size_t maxNumEntriesPerRow, ProfileType pftype=DynamicProfile, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, colMap, maxNumEntriesPerRow, pftype, plist) );
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<long long, Node>(rowMap, colMap, maxNumEntriesPerRow, pftype, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    //! Constructor specifying column Map and number of entries in each row.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &colMap, const ArrayRCP< const size_t > &NumEntriesPerRowToAlloc, ProfileType pftype=DynamicProfile, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, colMap, NumEntriesPerRowToAlloc, pftype, plist) );
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<long long, Node>(rowMap, colMap, NumEntriesPerRowToAlloc, pftype, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    //! Constructor specifying a previously constructed graph.
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const CrsGraph< LocalOrdinal, GlobalOrdinal, Node > > &graph, const Teuchos::RCP< Teuchos::ParameterList > &plist=Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (graph->getRowMap()->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(graph, plist) );
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES
      if (graph->getRowMap()->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<long long, Node>(graph, plist) );
#endif
#endif

      XPETRA_FACTORY_END;
    }


    //! Constructor using FusedImport
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const CrsMatrix< Scalar, LocalOrdinal, GlobalOrdinal, Node > > &sourceMatrix, const Import<LocalOrdinal,GlobalOrdinal,Node> &importer, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & domainMap = Teuchos::null, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & rangeMap = Teuchos::null,const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (sourceMatrix->getRowMap()->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(sourceMatrix,importer,domainMap,rangeMap,params) );

#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES
      if (sourceMatrix->getRowMap()->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<long long, Node>(sourceMatrix,importer,domainMap,rangeMap,params) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

    //! Constructor using FusedExport
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build(const Teuchos::RCP< const CrsMatrix< Scalar, LocalOrdinal, GlobalOrdinal, Node > > &sourceMatrix, const Export<LocalOrdinal,GlobalOrdinal,Node> &exporter, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & domainMap = Teuchos::null, const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & rangeMap = Teuchos::null,const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null) {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (sourceMatrix->getRowMap()->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(sourceMatrix,exporter,domainMap,rangeMap,params) );

#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_64BIT_GLOBAL_INDICES
      if (sourceMatrix->getRowMap()->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<long long, Node>(sourceMatrix,exporter,domainMap,rangeMap,params) );
#endif
#endif

      XPETRA_FACTORY_END;
    }

#ifdef HAVE_XPETRA_KOKKOS_REFACTOR
    static RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > Build (
        const Teuchos::RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> >& rowMap,
        const Teuchos::RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> >& colMap,
        const typename Xpetra::CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>::local_matrix_type& lclMatrix,
        const Teuchos::RCP<Teuchos::ParameterList>& params = null)  {
      XPETRA_MONITOR("CrsMatrixFactory::Build");

#ifdef HAVE_XPETRA_TPETRA
      if (rowMap->lib() == UseTpetra)
        return rcp( new TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node>(rowMap, colMap, lclMatrix, params));
#endif

#ifdef HAVE_XPETRA_EPETRA
#ifndef XPETRA_EPETRA_NO_32BIT_GLOBAL_INDICES
      if (rowMap->lib() == UseEpetra)
        return rcp( new EpetraCrsMatrixT<long long, Node>(rowMap, colMap, lclMatrix, params) );
#endif
#endif
      XPETRA_FACTORY_END;
    }
#endif

  };
#endif // HAVE_XPETRA_INT_LONG_LONG
#endif // HAVE_XPETRA_SERIAL
}

#define XPETRA_CRSMATRIXFACTORY_SHORT
#endif
