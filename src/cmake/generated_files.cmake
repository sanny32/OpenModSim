# Generate license.txt
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/res/license.txt.in
    ${CMAKE_CURRENT_SOURCE_DIR}/res/license.txt
    @ONLY
)

# Generate README.md
configure_file(
    ${PROJECT_SOURCE_DIR}/../README.md.in
    ${PROJECT_SOURCE_DIR}/../README.md
    @ONLY
)
