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


INCLUDE("${CTEST_SCRIPT_DIRECTORY}/../../TrilinosCTestDriverCore.cmake")

#
# Platform/compiler specific options for geminga using gcc
#

MACRO(TRILINOS_SYSTEM_SPECIFIC_CTEST_DRIVER)

  # Base of Trilinos/cmake/ctest then BUILD_DIR_NAME

  SET(CTEST_DASHBOARD_ROOT  "${TRILINOS_CMAKE_DIR}/../../${BUILD_DIR_NAME}" )
  SET(CTEST_NOTES_FILES     "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}" )
  SET(CTEST_BUILD_FLAGS     "-j35 -i" )

  SET_DEFAULT(CTEST_PARALLEL_LEVEL                  "35" )
  SET_DEFAULT(Trilinos_ENABLE_SECONDARY_STABLE_CODE ON)
  SET_DEFAULT(Trilinos_EXCLUDE_PACKAGES             ${EXTRA_EXCLUDE_PACKAGES} TriKota Optika)

  SET(EXTRA_SYSTEM_CONFIGURE_OPTIONS
    "-DBUILD_SHARED_LIBS=ON"
    "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
    "-DCMAKE_VERBOSE_MAKEFILE=ON"

    "-DTrilinos_ENABLE_Fortran=OFF"

    "-DSuperLU_INCLUDE_DIRS=/home/aprokop/local/opt/superlu-4.3/include"
    "-DSuperLU_LIBRARY_DIRS=/home/aprokop/local/opt/superlu-4.3/lib"
    "-DSuperLU_LIBRARY_NAMES=superlu_4.3"
    )

  SET_DEFAULT(COMPILER_VERSION "GCC-5.2.0")

  # Ensure that MPI is on for all parallel builds that might be run.
  IF(COMM_TYPE STREQUAL MPI)

    SET(EXTRA_SYSTEM_CONFIGURE_OPTIONS
        ${EXTRA_SYSTEM_CONFIGURE_OPTIONS}
        "-DTPL_ENABLE_MPI=ON"
            "-DMPI_BASE_DIR=/home/aprokop/local/opt/openmpi-1.10.0"
       )

    SET(CTEST_MEMORYCHECK_COMMAND_OPTIONS
        "--gen-suppressions=all --error-limit=no --log-file=nightly_suppressions.txt" ${CTEST_MEMORYCHECK_COMMAND_OPTIONS} )

  ELSE()

    SET( EXTRA_SYSTEM_CONFIGURE_OPTIONS
      ${EXTRA_SYSTEM_CONFIGURE_OPTIONS}
      "-DCMAKE_CXX_COMPILER=/home/aprokop/local/opt/gcc-5.2.0/bin/g++"
      "-DCMAKE_C_COMPILER=/home/aprokop/local/opt/gcc-5.2.0/bin/gcc"
      )

  ENDIF()

  TRILINOS_CTEST_DRIVER()

ENDMACRO()
