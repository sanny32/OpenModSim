cmake_minimum_required(VERSION 3.28.4)
set(CMAKE_MIN_REQ_VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

project(omodsim
        VERSION 2.2.0
        DESCRIPTION "An Open Source Modbus Slave (Server) Utility"
        LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()
set(PRODUCT_NAME "Open ModSim")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_version.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ui_normalization.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/help_generator.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/target_windows.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/target_macos.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/target_linux.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/generated_files.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/qlementine.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/coverage.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/tests.cmake")

option(OMODSIM_BUILD_TESTS "Build the unit tests" ON)

omodsim_apply_project_version_suffix()

set(OMODSIM_APP_DEFINITIONS
    APP_NAME="${PRODUCT_NAME} ${VERSION_MAJOR}"
    APP_PRODUCT_NAME="${PRODUCT_NAME}"
    APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
    APP_VERSION_MAJOR="${VERSION_MAJOR}"
    APP_VERSION_MINOR="${VERSION_MINOR}"
    APP_VERSION_PATCH="${VERSION_PATCH}"
    APP_VERSION="${PROJECT_VERSION}"
    BUILD_YEAR="${CURRENT_YEAR}"
)

if(OMODSIM_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(core)
add_subdirectory(ui)
add_subdirectory(app)
if(OMODSIM_BUILD_TESTS)
    add_subdirectory(tests)
endif()

omodsim_configure_uninstall_target()
