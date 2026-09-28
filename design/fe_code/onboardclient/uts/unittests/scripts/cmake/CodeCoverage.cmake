# Copyright (c) 2012 - 2017, Lars Bilke
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# CHANGES:
#
# 2012-01-31, Lars Bilke
# - Enable Code Coverage
#
# 2013-09-17, Joakim Söderberg
# - Added support for Clang.
# - Some additional usage instructions.
#
# 2016-02-03, Lars Bilke
# - Refactored functions to use named parameters
#
# 2017-06-02, Lars Bilke
# - Merged with modified version from github.com/ufz/ogs
#
#
# USAGE:
#
# 1. Copy this file into your cmake modules path.
#
# 2. Add the following line to your CMakeLists.txt:
#      include(CodeCoverage)
#
# 3. Append necessary compiler flags:
#      APPEND_COVERAGE_COMPILER_FLAGS()
#
# 4. If you need to exclude additional directories from the report, specify them
#    using the COVERAGE_EXCLUDES variable before calling SETUP_TARGET_FOR_COVERAGE.
#    Example:
#      set(COVERAGE_EXCLUDES 'dir1/*' 'dir2/*')
#
# 5. Use the functions described below to create a custom make target which
#    runs your test executable and produces a code coverage report.
#
# 6. Build a Debug build:
#      cmake -DCMAKE_BUILD_TYPE=Debug ..
#      make
#      make my_coverage_target
#

include(CMakeParseArguments)

# Check prereqs
find_program( GCOV_PATH gcov )
find_program( LCOV_PATH lcov )
find_program( GENHTML_PATH genhtml )
find_program( GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/scripts/test)
find_program( SIMPLE_PYTHON_EXECUTABLE python )

find_program( WGET_PATH wget )
find_program( CHMOD_PATH chmod )

if(NOT GCOV_PATH)
    message(FATAL_ERROR "gcov not found! Aborting...")
endif() # NOT GCOV_PATH

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 3)
        message(FATAL_ERROR "Clang version must be 3.0.0 or greater! Aborting...")
    endif()
elseif(NOT CMAKE_COMPILER_IS_GNUCXX)
    message(FATAL_ERROR "Compiler is not GNU gcc! Aborting...")
endif()

set(COVERAGE_COMPILER_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage"
    CACHE INTERNAL "")

set(CMAKE_CXX_FLAGS_COVERAGE
    ${COVERAGE_COMPILER_FLAGS}
    CACHE STRING "Flags used by the C++ compiler during coverage builds."
    FORCE )
set(CMAKE_C_FLAGS_COVERAGE
    ${COVERAGE_COMPILER_FLAGS}
    CACHE STRING "Flags used by the C compiler during coverage builds."
    FORCE )
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used for linking binaries during coverage builds."
    FORCE )
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used by the shared libraries linker during coverage builds."
    FORCE )
mark_as_advanced(
    CMAKE_CXX_FLAGS_COVERAGE
    CMAKE_C_FLAGS_COVERAGE
    CMAKE_EXE_LINKER_FLAGS_COVERAGE
    CMAKE_SHARED_LINKER_FLAGS_COVERAGE )

#if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
#    message(WARNING "Code coverage results with an optimised (non-Debug) build may be misleading")
#endif() # NOT CMAKE_BUILD_TYPE STREQUAL "Debug"

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    link_libraries(gcov)
else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Defines a target for running and collection code coverage information
# Builds dependencies, runs the given executable and outputs reports.
# NOTE! The executable should always have a ZERO as exit code otherwise
# the coverage generation will not complete.
#
# SETUP_TARGET_FOR_COVERAGE(
#     NAME testrunner_coverage                    # New target name
#     EXECUTABLE testrunner -j ${PROCESSOR_COUNT} # Executable in PROJECT_BINARY_DIR
#     DEPENDENCIES testrunner                     # Dependencies to build first
# )
function(SETUP_TARGET_FOR_COVERAGE)

    set(options NONE)
    set(oneValueArgs NAME)
    set(multiValueArgs EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
    cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT LCOV_PATH)
        message(FATAL_ERROR "lcov not found! Aborting...")
    endif() # NOT LCOV_PATH

    if(NOT GENHTML_PATH)
        message(FATAL_ERROR "genhtml not found! Aborting...")
    endif() # NOT GENHTML_PATH

    if( NOT "all_coverage" STREQUAL "${Coverage_NAME}" )
        string(REPLACE "_coverage" "" CoverageModule ${Coverage_NAME})
        set(COVERAGE_WORK_DIR ".${CMAKE_FILES_DIRECTORY}/${CoverageModule}.dir")
    else()
        set(COVERAGE_WORK_DIR ".")
    endif()

    set(LCOV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lcovrc")

    # Setup target
    add_custom_target(${Coverage_NAME}

        COMMAND echo "LCOV CONFIG FILE PATH : ${LCOV_CONF_PATH}"
        # COMMAND echo ${COVERAGE_WORK_DIR}
        # Cleanup lcov
        COMMAND ${LCOV_PATH} --directory ${COVERAGE_WORK_DIR} --zerocounters
        # Create baseline to make sure untouched files show up in the report
        # COMMAND ${LCOV_PATH} --rc lcov_branch_coverage=1 -c -i -d ${COVERAGE_WORK_DIR} -o ${Coverage_NAME}.base
        COMMAND ${LCOV_PATH} -c --config-file ${LCOV_CONF_PATH} -i -d ${COVERAGE_WORK_DIR} -o ${Coverage_NAME}.base

        # Run test
        #COMMAND ${Coverage_EXECUTABLE}

        COMMAND ${CMAKE_COMMAND} -E make_directory reports
        COMMAND ${CMAKE_COMMAND} -DMACHINE_QEMU_ARM=${MACHINE_QEMU_ARM} -DCMAKE_SYSROOT=${CMAKE_SYSROOT} -DCXX_COMPILER=${CMAKE_CXX_COMPILER} -DTARGET_PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY} -DTARGET_BIN="${Coverage_EXECUTABLE}" -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/TargetRun.cmake

        # Capturing lcov counters and generating report
        #COMMAND ${LCOV_PATH} --directory ${COVERAGE_WORK_DIR} --capture --output-file ${Coverage_NAME}.info
        COMMAND ${LCOV_PATH} -c --config-file ${LCOV_CONF_PATH} --directory ${COVERAGE_WORK_DIR} --output-file ${Coverage_NAME}.info

        # add baseline counters
        COMMAND ${LCOV_PATH} -a --config-file ${LCOV_CONF_PATH} ${Coverage_NAME}.base -a ${Coverage_NAME}.info --output-file ${Coverage_NAME}.total
        COMMAND ${LCOV_PATH} -r --config-file ${LCOV_CONF_PATH} ${Coverage_NAME}.total ${COVERAGE_EXCLUDES} --output-file ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned
        COMMAND ${GENHTML_PATH} -o ${Coverage_NAME} --branch-coverage ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned
        COMMAND ${CMAKE_COMMAND} -E copy ${Coverage_NAME}.info ${Coverage_NAME}.total ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned ${Coverage_NAME}/
        COMMAND ${CMAKE_COMMAND} -E remove ${Coverage_NAME}.base ${Coverage_NAME}.info ${Coverage_NAME}.total ${PROJECT_BINARY_DIR}/${Coverage_NAME}.info.cleaned

        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        DEPENDS ${Coverage_DEPENDENCIES}
        COMMENT "Resetting code coverage counters to zero.\nProcessing code coverage counters and generating report."
    )

    # Show info where to find the report
    add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
        COMMAND ;
        COMMENT "Open ./${Coverage_NAME}/index.html in your browser to view the coverage report."
    )


endfunction() # SETUP_TARGET_FOR_COVERAGE

# Comment for SETUP_TARGET_FOR_UTS_RUNNER
#
# SETUP_TARGET_FOR_UTS_RUNNER(..... parameter.....)
#
function(SETUP_TARGET_FOR_UTS_RUNNER)

     set(options NONE)
     set(oneValueArgs NAME)
     set(multiValueArgs EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
     cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

     string(TIMESTAMP TODAY "%Y%m%d")

     set(BOARD_COMMAND2 ${CHMOD_PATH} +x ./run-coverage_arm_for_gleaner.sh)
     set(BOARD_COMMAND ${WGET_PATH} -c http://swta.lge.com/utsrunner/run-coverage_arm_for_gleaner.sh && ${BOARD_COMMAND2})
     set(BOARD_RUN bash ./run-coverage_arm_for_gleaner.sh)
     set(HOST_RUN make all_coverage && cp ./reports/*.xml ./bin/)
     set(UTSRUNNER_HOST ./gleaner.sh dgtype=all createBD=no filtercase=$(target))
     set(UTSRUNNER_TARGET ./gleaner.sh dgtype=all createBD=no filtercase=$(target))

     #setting for all or part
     if( NOT "uts_run" STREQUAL "${Coverage_NAME}" )
        message("The Target name is wrong.(EarlyReturn)")
        return()
     else()
       set(UTSRUNNER_COMMAND cd ../gleaner && ./gleaner.sh dgtype=all)
     endif()

    # Setup target
    add_custom_target(${Coverage_NAME}

    COMMAND echo "-----------------------------"
    COMMAND if [ $(board) = 'y' ]\; then ${BOARD_COMMAND}\; ${BOARD_RUN}\; else ${HOST_RUN}\; fi\;
    DEPENDS ${Coverage_DEPENDENCIES}
    )

# Show info where to find the report
    add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
        COMMAND echo "-------------[target=$(target), board=$(board)]"
        COMMAND if [ $(board) = 'y' ]\; then echo ${UTSRUNNER_TARGET}\; ${UTSRUNNER_TARGET}\;  else echo ${UTSRUNNER_HOST}\; ${UTSRUNNER_HOST}\; fi\;
        COMMAND echo "-------------------------------------"

        COMMAND;
        COMMENT "Open ./bin/report/${TODAY} in your browser to view the result."
        WORKING_DIRECTORY "../gleaner"

   )

endfunction()


function (ABC)
    message("ABC")
endfunction()

# Helper function to get the language of a source file.
function (codecov_lang_of_source FILE RETURN_VAR)
  get_filename_component(FILE_EXT "${FILE}" EXT)
  string(TOLOWER "${FILE_EXT}" FILE_EXT)
  string(SUBSTRING "${FILE_EXT}" 1 -1 FILE_EXT)

  get_property(ENABLED_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
  foreach (LANG ${ENABLED_LANGUAGES})
    list(FIND CMAKE_${LANG}_SOURCE_FILE_EXTENSIONS "${FILE_EXT}" TEMP)
    if(NOT ${TEMP} EQUAL -1)
      set(${RETURN_VAR} "${LANG}" PARENT_SCOPE)
      return()
    endif()
  endforeach()

  set(${RETURN_VAR} "" PARENT_SCOPE)
endfunction()


# Defines a target for running and collection code coverage information
# Builds dependencies, runs the given executable and outputs reports.
# NOTE! The executable should always have a ZERO as exit code otherwise
# the coverage generation will not complete.
#
# SETUP_TARGET_FOR_COVERAGE_COBERTURA(
#     NAME ctest_coverage                    # New target name
#     EXECUTABLE ctest -j ${PROCESSOR_COUNT} # Executable in PROJECT_BINARY_DIR
#     DEPENDENCIES executable_target         # Dependencies to build first
# )
function(SETUP_TARGET_FOR_COVERAGE_COBERTURA)

    set(options NONE)
    set(oneValueArgs NAME)
    set(multiValueArgs EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
    cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT SIMPLE_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "python not found! Aborting...")
    endif() # NOT SIMPLE_PYTHON_EXECUTABLE

    if(NOT GCOVR_PATH)
        message(FATAL_ERROR "gcovr not found! Aborting...")
    endif() # NOT GCOVR_PATH

    # Combine excludes to several -e arguments
    set(COBERTURA_EXCLUDES "")
    foreach(EXCLUDE ${COVERAGE_EXCLUDES})
        set(COBERTURA_EXCLUDES "-e ${EXCLUDE} ${COBERTURA_EXCLUDES}")
    endforeach()

    add_custom_target(${Coverage_NAME}

        # Run tests
        ${Coverage_EXECUTABLE}

        # Running gcovr
        COMMAND ${GCOVR_PATH} -x -r ${CMAKE_SOURCE_DIR} ${COBERTURA_EXCLUDES}
            -o ${Coverage_NAME}.xml
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        DEPENDS ${Coverage_DEPENDENCIES}
        COMMENT "Running gcovr to produce Cobertura code coverage report."
    )

    # Show info where to find the report
    add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
        COMMAND ;
        COMMENT "Cobertura code coverage report saved in ${Coverage_NAME}.xml."
    )

endfunction() # SETUP_TARGET_FOR_COVERAGE_COBERTURA

function(APPEND_COVERAGE_COMPILER_FLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_COMPILER_FLAGS}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}" PARENT_SCOPE)
    message(STATUS "Appending code coverage compiler flags: ${COVERAGE_COMPILER_FLAGS}")
endfunction() # APPEND_COVERAGE_COMPILER_FLAGS
