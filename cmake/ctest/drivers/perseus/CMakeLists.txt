TRILINOS_DRIVER_SETUP()

#TRILINOS_DRIVER_ADD_DASHBOARD(
#  MPI_Tpetra_KokkosRefactor_GCC
#  ctest_linux_nightly_mpi_release_tpetra_kokkosrefactor_perseus.cmake 
#  CTEST_INSTALLER_TYPE release
#  RUN_SERIAL
#  TIMEOUT_MINUTES 60
#)

#TRILINOS_DRIVER_ADD_DASHBOARD(
#  MPI_Tpetra_KokkosRefactor_ETI_CUDA_OPENMP_HWLOC_GCC
#  ctest_linux_nightly_mpi_release_tpetra_kokkosrefactor_ETI_cuda_openmp_hwloc_gcc_perseus.cmake 
#  CTEST_INSTALLER_TYPE release
#  RUN_SERIAL
#  TIMEOUT_MINUTES 60
#)

#TRILINOS_DRIVER_ADD_DASHBOARD(
#  MPI_Tpetra_KokkosRefactor_CUDA_OPENMP_HWLOC_GCC
#  ctest_linux_nightly_mpi_release_tpetra_kokkosrefactor_cuda_openmp_hwloc_gcc_perseus.cmake 
#  CTEST_INSTALLER_TYPE release
#  RUN_SERIAL
#  TIMEOUT_MINUTES 60
#)

if (DEFINED ENV{FROM_JENKINS})

TRILINOS_DRIVER_ADD_DASHBOARD(
  MPI_Downstream_KokkosRefactor_ETI_OPENMP_HWLOC_WITH_KOKKOS_$ENV{KOKKOS_BRANCH}
  ctest_linux_nightly_mpi_release_downstream_kokkosrefactor_ETI_openmp_hwloc_perseus_jenkins.cmake
  CTEST_INSTALLER_TYPE release
  RUN_SERIAL
  TIMEOUT_MINUTES 180
)

# TRILINOS_DRIVER_ADD_DASHBOARD(
#   MPI_Downstream_KokkosRefactor_ETI_THREADS_WITH_KOKKOS_$ENV{KOKKOS_BRANCH}
#   ctest_linux_nightly_mpi_release_downstream_kokkosrefactor_ETI_threads_hwloc_perseus_jenkins.cmake
#   CTEST_INSTALLER_TYPE release
#   RUN_SERIAL
#   TIMEOUT_MINUTES 180
# )

# TRILINOS_DRIVER_ADD_DASHBOARD(
#   MPI_Downstream_KokkosRefactor_ETI_SERIAL_GCC
#   ctest_linux_nightly_mpi_debug_muelu_extratypes_ei_negima_jenkins.cmake
#   CTEST_INSTALLER_TYPE release
#   RUN_SERIAL
#   TIMEOUT_MINUTES 180
# )

else ()

TRILINOS_DRIVER_ADD_DASHBOARD(
  MPI_Downstream_KokkosRefactor_ETI_OPENMP_HWLOC_GCC
  ctest_linux_nightly_mpi_release_downstream_kokkosrefactor_ETI_openmp_hwloc_gcc_perseus.cmake
  CTEST_INSTALLER_TYPE release
  RUN_SERIAL
  TIMEOUT_MINUTES 180
)

TRILINOS_DRIVER_ADD_DASHBOARD(
  MPI_Downstream_KokkosRefactor_ETI_THREADS_INTEL
  ctest_linux_nightly_mpi_release_downstream_kokkosrefactor_ETI_threads_hwloc_intel_perseus.cmake
  CTEST_INSTALLER_TYPE release
  RUN_SERIAL
  TIMEOUT_MINUTES 180
)

TRILINOS_DRIVER_ADD_DASHBOARD(
  MPI_Downstream_KokkosRefactor_ETI_SERIAL_GCC
  ctest_linux_nightly_mpi_debug_muelu_extratypes_ei_negima.cmake
  CTEST_INSTALLER_TYPE release
  RUN_SERIAL
  TIMEOUT_MINUTES 180
)

endif ()


#TRILINOS_DRIVER_ADD_DASHBOARD(
#  MPI_Downstream_KokkosRefactor_ETI_SERIAL_Intel
#  ctest_linux_nightly_mpi_release_downstream_kokkosrefactor_ETI_serial_intel_perseus.cmake
#  CTEST_INSTALLER_TYPE release
#  RUN_SERIAL
#  TIMEOUT_MINUTES 180
#)


TRILINOS_ADD_REQUIRED_CMAKE_INSTALLS()
