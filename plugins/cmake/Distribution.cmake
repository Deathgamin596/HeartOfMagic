# Distribution.cmake
# Assembles the release distribution folder after all targets are built.
# Requires: DIST_VERSION_DIR (from top-level CMakeLists.txt)
#           PAPYRUS_ISL_SOURCES, PAPYRUS_SPELLLEARNING_SOURCES, PAPYRUS_OUTPUT_DIR
#           (from Papyrus.cmake, if Papyrus compiler is available)
#
# Delegates all work to tools/dist_assemble.bat to avoid MSBuild post-build
# issues with cmake -E commands and Windows shell command path handling.

if(NOT DEFINED DIST_VERSION_DIR)
    message(STATUS "DIST_VERSION_DIR not defined - distribution assembly disabled")
    return()
endif()

if(NOT DEFINED PAPYRUS_OUTPUT_DIR)
    set(PAPYRUS_OUTPUT_DIR "")
endif()

# ============================================================================
# Distribution target - assembles release folder after all DLLs are built
# ============================================================================

add_custom_target(assemble_dist ALL
    COMMENT "Assembling release distribution...")

add_dependencies(assemble_dist SpellLearning DontEatSpellTomes SL_BookXP)

if(TARGET papyrus_build)
    add_dependencies(assemble_dist papyrus_build)
endif()

# ============================================================================
# Single custom command that delegates to dist_assemble.bat
# ============================================================================

get_filename_component(_dist_folder_name "${DIST_VERSION_DIR}" NAME)

add_custom_command(TARGET assemble_dist POST_BUILD
    COMMAND "${CMAKE_SOURCE_DIR}/tools/dist_assemble.bat"
        "${CMAKE_BINARY_DIR}"
        "${DIST_VERSION_DIR}"
        "${DIST_DIR}"
        "${CMAKE_SOURCE_DIR}"
        "${PAPYRUS_OUTPUT_DIR}"
        "$<TARGET_FILE:SpellLearning>"
        "$<TARGET_FILE:SL_BookXP>"
        "$<TARGET_FILE:DontEatSpellTomes>"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Running distribution assembly script..."
    VERBATIM
)

message(STATUS "Distribution assembly configured:")
message(STATUS "  Output:   ${DIST_VERSION_DIR}")
message(STATUS "  Archive:  ${DIST_DIR}/${_dist_folder_name}.zip")
