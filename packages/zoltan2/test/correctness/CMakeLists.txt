#
# Tests that check correctness of RCB.
#


## This test calls zdrive functions to read
## in the same input data and run some of
## the same tests that Zoltan runs.
 
IF (${PACKAGE_NAME}_ENABLE_Experimental)
    TRIBITS_ADD_EXECUTABLE_AND_TEST(
      zoltanCompareRCBs
      SOURCES zoltanCompareRCBs.cpp
      NUM_MPI_PROCS 4
      COMM serial mpi
      PASS_REGULAR_EXPRESSION "PASS"
      FAIL_REGULAR_EXPRESSION "FAIL"
    )
ENDIF()

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  zoltanCompare
  SOURCES zoltanCompare.cpp
  NUM_MPI_PROCS 8 
  COMM serial mpi
  PASS_REGULAR_EXPRESSION "PASS"
  FAIL_REGULAR_EXPRESSION "FAIL"
)

# Command that copies files to the executable directory.
#TRIBITS_COPY_FILES_TO_BINARY_DIR(copy_files_for_correctness_tests
#  SOURCE_FILES
#    simple.mtx
#    simple_coord.mtx
#  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../data
#  DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}
#)
