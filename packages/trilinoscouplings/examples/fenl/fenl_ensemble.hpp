/*
//@HEADER
// ************************************************************************
//
//   Kokkos: Manycore Performance-Portable Multidimensional Arrays
//              Copyright (2012) Sandia Corporation
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
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOS_EXAMPLE_FENL_ENSEMBLE_HPP
#define KOKKOS_EXAMPLE_FENL_ENSEMBLE_HPP

//----------------------------------------------------------------------------
// Specializations for ensemble scalar type
//----------------------------------------------------------------------------

#include "Stokhos_Tpetra_MP_Vector.hpp"

#if defined( HAVE_STOKHOS_BELOS )
#include "Belos_TpetraAdapter_MP_Vector.hpp"
#endif

#if defined( HAVE_STOKHOS_MUELU )
#include "Stokhos_MueLu_MP_Vector.hpp"
#endif


#include <fenl.hpp>
#include <fenl_impl.hpp>

namespace Kokkos {
namespace Example {
namespace FENL {

#if defined( KOKKOS_HAVE_CUDA )
template <typename ViewType>
struct LocalViewTraits<
  ViewType,
  typename Kokkos::Impl::enable_if<
    Kokkos::Impl::is_same< typename ViewType::execution_space,
                           Kokkos::Cuda >::value &&
    Kokkos::Impl::is_same< typename ViewType::specialize,
                           Kokkos::Impl::ViewMPVectorContiguous >::value >::type
  > {
  typedef ViewType view_type;
  typedef typename Kokkos::LocalMPVectorView<view_type,1>::type local_view_type;
  typedef typename local_view_type::value_type local_value_type;
  static const bool use_team = true;

  KOKKOS_INLINE_FUNCTION
  static local_view_type create_local_view(const view_type& v,
                                           const unsigned local_rank)
  {
    local_view_type local_v =
      Kokkos::partition<local_view_type>(v, local_rank, local_rank+1);
    return local_v;
  }
};

// Compute DeviceConfig struct's based on scalar type
template <typename StorageType>
struct CreateDeviceConfigs< Sacado::MP::Vector<StorageType> > {
  typedef typename StorageType::execution_space execution_space;
  static void eval( Kokkos::DeviceConfig& dev_config_elem,
                    Kokkos::DeviceConfig& dev_config_gath,
                    Kokkos::DeviceConfig& dev_config_bc ) {
    static const unsigned VectorSize = StorageType::static_size;
    if ( Kokkos::Impl::is_same< execution_space, Kokkos::Cuda >::value ) {
      dev_config_elem = Kokkos::DeviceConfig( 0 , VectorSize , 64/VectorSize  );
      dev_config_gath = Kokkos::DeviceConfig( 0 , VectorSize , 128/VectorSize );
      dev_config_bc   = Kokkos::DeviceConfig( 0 , VectorSize , 256/VectorSize );
    }
    else {
      dev_config_elem = Kokkos::DeviceConfig( 0 , 1 , 1 );
      dev_config_gath = Kokkos::DeviceConfig( 0 , 1 , 1 );
      dev_config_bc   = Kokkos::DeviceConfig( 0 , 1 , 1 );
    }
  }
};
#endif

} /* namespace FENL */
} /* namespace Example */
} /* namespace Kokkos */

#endif /* #ifndef KOKKOS_EXAMPLE_FENL_ENSEMBLE_HPP */
