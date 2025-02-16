// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
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
#include "MueLu_ExplicitInstantiation.hpp"

#include "MueLu_HierarchyHelpers_def.hpp"

#include "TpetraCore_ETIHelperMacros.h"

#define MUELU_LOCAL_INSTANT(S,LO,GO,N) \
        template class MueLu::TopRAPFactory<S,LO,GO,N>; \
        template class MueLu::TopSmootherFactory<S,LO,GO,N>; \
        template class MueLu::HierarchyUtils<S,LO,GO,N>;

TPETRA_ETI_MANGLING_TYPEDEFS()

TPETRA_INSTANTIATE_SLGN_NO_ORDINAL_SCALAR(MUELU_LOCAL_INSTANT)

#ifdef HAVE_MUELU_EPETRA
#ifndef HAVE_MUELU_TPETRA_INST_INT_INT
#ifdef HAVE_TPETRA_INST_CUDA_DEFAULT
    template class MueLu::TopRAPFactory<double,int,int, Kokkos::Compat::KokkosCudaWrapperNode >;
    template class MueLu::TopSmootherFactory<double,int,int, Kokkos::Compat::KokkosCudaWrapperNode >;
    template class MueLu::HierarchyUtils<double,int,int, Kokkos::Compat::KokkosCudaWrapperNode >;
#elseif HAVE_TPETRA_INST_OPENMP_DEFAULT
    template class MueLu::TopRAPFactory<double,int,int, Kokkos::Compat::KokkosOpenMPWrapperNode >;
    template class MueLu::TopSmootherFactory<double,int,int, Kokkos::Compat::KokkosOpenMPWrapperNode >;
    template class MueLu::HierarchyUtils<double,int,int, Kokkos::Compat::KokkosOpenMPWrapperNode >;

#elseif HAVE_TPETRA_INST_SERIAL_DEFAULT
    template class MueLu::TopRAPFactory<double,int,int, Kokkos::Compat::KokkosSerialWrapperNode >;
    template class MueLu::TopSmootherFactory<double,int,int, Kokkos::Compat::KokkosSerialWrapperNode >;
    template class MueLu::HierarchyUtils<double,int,int, Kokkos::Compat::KokkosSerialWrapperNode >;

#elseif HAVE_TPETRA_INST_PTHREAD_DEFAULT
    template class MueLu::TopRAPFactory<double,int,int, Kokkos::Compat::KokkosThreadsWrapperNode >;
    template class MueLu::TopSmootherFactory<double,int,int, Kokkos::Compat::KokkosThreadsWrapperNode >;
    template class MueLu::HierarchyUtils<double,int,int, Kokkos::Compat::KokkosThreadsWrapperNode >;

#else
    // use Serial Node as default if Tpetra is disabled??
  template class MueLu::TopRAPFactory<double,int,int, Kokkos::Compat::KokkosSerialWrapperNode >;
  template class MueLu::TopSmootherFactory<double,int,int, Kokkos::Compat::KokkosSerialWrapperNode >;
  template class MueLu::HierarchyUtils<double,int,int, Kokkos::Compat::KokkosSerialWrapperNode >;

#endif // end IF ELSEIF ...

#endif // end ifndef HAVE_MUELU_TPETRA_INST_INT_INT
#endif // end ifdef HAVE_MUELU_EPETRA
