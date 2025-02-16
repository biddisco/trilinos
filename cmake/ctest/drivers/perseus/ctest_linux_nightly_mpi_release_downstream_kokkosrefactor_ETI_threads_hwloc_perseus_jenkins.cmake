# @HEADER
# ************************************************************************
#
#            Trilinos: An Object-Oriented Solver Framework
#                 Copyright (2001) Sandia Corporation
#
#
# Copyright (2001) Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this
# work by or on behalf of the U.S. Government.  Export of this program
# may require a license from the United States Government.
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the Corporation nor the names of the
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# NOTICE:  The United States Government is granted for itself and others
# acting on its behalf a paid-up, nonexclusive, irrevocable worldwide
# license in this data to reproduce, prepare derivative works, and
# perform publicly and display publicly.  Beginning five (5) years from
# July 25, 2001, the United States Government is granted for itself and
# others acting on its behalf a paid-up, nonexclusive, irrevocable
# worldwide license in this data to reproduce, prepare derivative works,
# distribute copies to the public, perform publicly and display
# publicly, and to permit others to do so.
#
# NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT
# OF ENERGY, NOR SANDIA CORPORATION, NOR ANY OF THEIR EMPLOYEES, MAKES
# ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR
# RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY
# INFORMATION, APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS
# THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.
#
# ************************************************************************
# @HEADER


INCLUDE("${CTEST_SCRIPT_DIRECTORY}/TrilinosCTestDriverCore.perseus.jenkins.cmake")

#
# Set the options specific to this build case
#

SET(COMM_TYPE MPI)
SET(BUILD_TYPE RELEASE)
SET(BUILD_DIR_NAME MPI_RELEASE_DEV_DownStream_KokkosRefactor_ETI_THREADS_HWLOC_INTEL_WITH_KOKKOS_$ENV{KOKKOS_BRANCH})
SET(CTEST_PARALLEL_LEVEL 1)
SET(CTEST_TEST_TYPE Nightly)
SET(CTEST_TEST_TIMEOUT 1800)

SET(Trilinos_PACKAGES Kokkos Tpetra Belos Ifpack2 MueLu Amesos Amesos2 Ifpack Epetra EpetraExt Zoltan Zoltan2)

SET(EXTRA_CONFIGURE_OPTIONS
  "-DTPL_ENABLE_SuperLU=ON"

  "-DTpetra_ENABLE_Kokkos_Refactor:BOOL=ON"

  "-DTrilinos_ENABLE_EXPLICIT_INSTANTIATION:BOOL=ON"
  "-DTeuchos_ENABLE_COMPLEX:BOOL=ON"
  "-DKokkosClassic_DefaultNode:STRING=Kokkos::Compat::KokkosThreadsWrapperNode"
  "-DTPL_ENABLE_Pthread=ON"
  "-DTpetra_INST_PTHREAD=ON"
  "-DTPL_ENABLE_HWLOC:STRING=OFF"
    
  "-DAmesos_ENABLE_TESTS=OFF"
  "-DAmesos_ENABLE_EXAMPLES=OFF"
  "-DIfpack_ENABLE_TESTS=OFF"
  "-DIfpack_ENABLE_EXAMPLES=OFF"
  "-DEpetra_ENABLE_TESTS=OFF"
  "-DEpetra_ENABLE_EXAMPLES=OFF"
  "-DEpetraExt_ENABLE_TESTS=OFF"
  "-DEpetraExt_ENABLE_EXAMPLES=OFF"
  "-DZoltan_ENABLE_TESTS=OFF"
  "-DZoltan_ENABLE_EXAMPLES=OFF"
  "-DZoltan2_ENABLE_TESTS=ON"
  "-DZoltan2_ENABLE_EXAMPLES=ON"
  
)

#"-DMPI_EXEC_POST_NUMPROCS_FLAGS:STRING=-bind-to;socket;--map-by;socket"
#
# Set the rest of the system-specific options and run the dashboard build/test
#

TRILINOS_SYSTEM_SPECIFIC_CTEST_DRIVER()
