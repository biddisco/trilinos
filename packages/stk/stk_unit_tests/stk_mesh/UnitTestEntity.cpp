// Copyright (c) 2013, Sandia Corporation.
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
// 
//     * Neither the name of Sandia Corporation nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#include <stk_mesh/base/EntityKey.hpp>  // for EntityKey
#include <stk_mesh/base/Types.hpp>      // for PartVector
#include <stk_util/parallel/Parallel.hpp>  // for ParallelMachine
#include <gtest/gtest.h>
#include "stk_topology/topology.hpp"    // for topology, etc
#include <stdexcept>

namespace stk { namespace mesh { class Bucket; } }
namespace stk { namespace mesh { class BulkData; } }
namespace stk { namespace mesh { class MetaData; } }
namespace stk { namespace mesh { class Part; } }
namespace stk { namespace mesh { namespace impl { class EntityRepository; } } }
namespace stk { namespace mesh { namespace impl { class PartRepository; } } }
namespace stk { namespace mesh { struct Entity; } }





using stk::ParallelMachine;
using stk::mesh::MetaData;
using stk::mesh::BulkData;
using stk::mesh::Part;
using stk::mesh::PartVector;
using stk::mesh::EntityKey;
using stk::mesh::Entity;
using stk::mesh::Bucket;
using stk::mesh::impl::PartRepository;
using stk::mesh::impl::EntityRepository;

namespace {

//----------------------------------------------------------------------

TEST(UnitTestEntity,testEntityKey)
{
  EntityKey key_bad_zero = EntityKey();
  EntityKey key_good_0_1 = EntityKey( stk::topology::NODE_RANK , 1 );
  EntityKey key_good_1_1 = EntityKey( stk::topology::EDGE_RANK , 1 );
  EntityKey key_good_2_10 = EntityKey( stk::topology::FACE_RANK , 10);
  EntityKey key_order_1_12 = EntityKey( stk::topology::EDGE_RANK , 12 );
  EntityKey key_order_2_10 = EntityKey( stk::topology::FACE_RANK , 10 );

  ASSERT_TRUE( ! key_bad_zero.is_valid() );
  ASSERT_TRUE(   key_good_0_1.is_valid() );
  ASSERT_TRUE(   key_good_1_1.is_valid() );
  ASSERT_TRUE(   key_good_2_10.is_valid() );

  ASSERT_TRUE( stk::topology::NODE_RANK  == key_good_0_1.rank());
  ASSERT_TRUE( stk::topology::EDGE_RANK  == key_good_1_1.rank() );
  ASSERT_TRUE( stk::topology::FACE_RANK  == key_good_2_10.rank() );
  ASSERT_TRUE( 1  == key_good_0_1.id() );
  ASSERT_TRUE( 1  == key_good_1_1.id() );
  ASSERT_TRUE( 10 == key_good_2_10.id() );

  ASSERT_TRUE(  key_order_1_12 <  key_order_2_10);
  ASSERT_TRUE( !( key_order_1_12 >  key_order_2_10));

#ifndef NDEBUG
  ASSERT_THROW( EntityKey( stk::topology::INVALID_RANK, 1 ) , std::logic_error );
  ASSERT_THROW( EntityKey( stk::topology::NODE_RANK , ~0ull ) , std::logic_error );
#endif // NDEBUG
}

//----------------------------------------------------------------------
}//namespace <anonymous>

