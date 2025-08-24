cmake_minimum_required(VERSION 3.16)
project(astra LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Set LLVM directory - adjust path if needed
set(LLVM_DIR "/opt/homebrew/opt/llvm" CACHE PATH "LLVM installation directory")

# Add LLVM include directory
include_directories(${LLVM_DIR}/include)

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Find all source files
file(GLOB_RECURSE C_SOURCES "src/*.c")
file(GLOB_RECURSE CXX_SOURCES "src/*.cpp")

# Create the executable
add_executable(astra ${C_SOURCES} ${CXX_SOURCES})

# Set library paths
target_link_directories(astra PRIVATE ${LLVM_DIR}/lib)

# Get LLVM libraries using llvm-config
execute_process(
    COMMAND ${LLVM_DIR}/bin/llvm-config --libs all
    OUTPUT_VARIABLE LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE LLVM_LIBS_RESULT
)

execute_process(
    COMMAND ${LLVM_DIR}/bin/llvm-config --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE LLVM_SYSTEM_LIBS_RESULT
)

# Check if llvm-config commands succeeded
if(NOT LLVM_LIBS_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to run llvm-config --libs all")
endif()

if(NOT LLVM_SYSTEM_LIBS_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to run llvm-config --system-libs")
endif()

# Process LLVM libraries if they exist
if(LLVM_LIBS)
    # Convert to list and remove -l prefix
    string(REPLACE " " ";" LLVM_LIBS_LIST ${LLVM_LIBS})
    foreach(lib ${LLVM_LIBS_LIST})
        string(REGEX REPLACE "^-l" "" clean_lib ${lib})
        list(APPEND LLVM_LIBS_CLEAN ${clean_lib})
    endforeach()
endif()

# Process LLVM system libraries if they exist
if(LLVM_SYSTEM_LIBS)
    # Convert to list and remove -l prefix
    string(REPLACE " " ";" LLVM_SYSTEM_LIBS_LIST ${LLVM_SYSTEM_LIBS})
    foreach(lib ${LLVM_SYSTEM_LIBS_LIST})
        string(REGEX REPLACE "^-l" "" clean_lib ${lib})
        list(APPEND LLVM_SYSTEM_LIBS_CLEAN ${clean_lib})
    endforeach()
endif()

# Link libraries
if(LLVM_LIBS_CLEAN OR LLVM_SYSTEM_LIBS_CLEAN)
    target_link_libraries(astra PRIVATE ${LLVM_LIBS_CLEAN} ${LLVM_SYSTEM_LIBS_CLEAN})
else()
    message(WARNING "No LLVM libraries found via llvm-config")
endif()

# Set rpath for runtime library location
set_target_properties(astra PROPERTIES
    INSTALL_RPATH "${LLVM_DIR}/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)