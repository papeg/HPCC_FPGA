# This file contains the default configuration for the Nallatech 520N board
# for the use with single precision floating point values.
# To use this configuration file, call cmake with the parameter
#
#     cmake [...] -DHPCC_FPGA_CONFIG="path to this file"
#


set(USE_MPI Yes CACHE BOOL "" FORCE)
set(USE_SVM No CACHE BOOL "" FORCE)
set(USE_HBM No CACHE BOOL "" FORCE)
set(USE_ACCL Yes CACHE BOOL "" FORCE)
set(USE_XRT_HOST Yes CACHE BOOL "" FORCE)
set(USE_OCL_HOST No CACHE BOOL "" FORCE)
set(FPGA_BOARD_NAME "xilinx_u55c_gen3x16_xdma_3_202210_1" CACHE STRING "" FORCE)

# LINPACK specific options
set(DEFAULT_MATRIX_SIZE 1024 CACHE STRING "Default matrix size" FORCE)
set(LOCAL_MEM_BLOCK_LOG 8 CACHE STRING "Used to define the width and height of the block stored in local memory" FORCE)
set(REGISTER_BLOCK_LOG 3 CACHE STRING "Size of the block that will be manipulated in registers" FORCE)
set(NUM_REPLICATIONS 1 CACHE STRING "Number of times the matrix multiplication kernel will be replicated" FORCE)

set(XILINX_ADDITIONAL_COMPILE_FLAGS -g CACHE STRING "" FORCE)
set(XILINX_ADDITIONAL_LINK_FLAGS -g --kernel_frequency 250 CACHE STRING "" FORCE)
set(XILINX_KERNEL_NAMES lu top_update left_update inner_update_mm0 CACHE STRING "Names of all compute kernels" FORCE)
set(XILINX_COMPILE_SETTINGS_FILE "${CMAKE_SOURCE_DIR}/settings/settings.compile.xilinx.hpl_torus_pcie.ddr.ini" CACHE STRING "Compile settings file" FORCE)
set(XILINX_LINK_SETTINGS_FILE "${CMAKE_SOURCE_DIR}/settings/settings.link.xilinx.hpl_torus_accl.hbm.u55c.profile.generator.ini" CACHE STRING "Link settings file" FORCE)

set(USE_DEPRECATED_HPP_HEADER Yes CACHE BOOL "Use cl.hpp instead of cl2.hpp" FORCE)
set(USE_PCIE_MPI_COMMUNICATION Yes CACHE BOOL "Use PCIe and MPI for communication between FPGAs" FORCE)

