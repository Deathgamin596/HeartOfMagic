# Update the mod version tag in UI HTML files from the VERSION file
# Called by the update_ui_version custom target in CMakeLists.txt

set(VER "${VER}")

set(INDEX_HTML "${SRC_DIR}/PrismaUI/views/SpellLearning/SpellLearningPanel/index.html")
set(DEV_HTML "${SRC_DIR}/PrismaUI/views/SpellLearning/SpellLearningPanel/dev-harness.html")

file(READ "${INDEX_HTML}" index_content)
string(REGEX REPLACE "v1\\.0\\.\\d+" "v${VER}" index_content "${index_content}")
file(WRITE "${INDEX_HTML}" "${index_content}")
message(STATUS "Updated ${INDEX_HTML} to v${VER}")

file(READ "${DEV_HTML}" dev_content)
string(REGEX REPLACE "v1\\.0\\.\\d+-dev" "v${VER}-dev" dev_content "${dev_content}")
file(WRITE "${DEV_HTML}" "${dev_content}")
message(STATUS "Updated ${DEV_HTML} to v${VER}-dev")
