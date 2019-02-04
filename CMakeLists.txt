cmake_minimum_required(VERSION 3.9)
project(Collatz)

# Support clang tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(sources
  src/main.cpp)

add_executable(collatz ${sources})

target_compile_options(collatz PUBLIC -O3 -std=c++17 -Wall -Wextra -Wfloat-conversion)

target_include_directories(collatz PUBLIC include)

###############################################################################
## dependencies ###############################################################
###############################################################################

find_package(ImageMagick REQUIRED COMPONENTS Magick++)
target_compile_definitions(collatz PRIVATE
  MAGICKCORE_QUANTUM_DEPTH=16
  MAGICKCORE_HDRI_ENABLE=1
)
target_include_directories(collatz PRIVATE ${ImageMagick_INCLUDE_DIRS})

execute_process(COMMAND Magick++-config --ldflags
  OUTPUT_VARIABLE ImageMagick_LINK_FLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
target_link_libraries(collatz PRIVATE ${ImageMagick_LINK_FLAGS}) 

###############################################################################
## packaging ##################################################################
###############################################################################

install(TARGETS collatz DESTINATION bin)

set(CPACK_PACKAGE_NAME "Collatz")
set(CPACK_PACKAGE_VERSION "1.0.0")

set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)