cmake_minimum_required(VERSION 3.28.4)
set(CMAKE_MIN_REQ_VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

project(omodsim
        VERSION 2.0.0
        DESCRIPTION "An Open Source Modbus Slave (Server) Utility"
        LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
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

# Define target
add_executable(${PROJECT_NAME})

omodsim_apply_project_version_suffix()
omodsim_generate_files()

# Target compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_NAME="${PRODUCT_NAME} ${VERSION_MAJOR}"
    APP_PRODUCT_NAME="${PRODUCT_NAME}"
    APP_DESCRIPTION="${PROJECT_DESCRIPTION}"
    APP_VERSION_MAJOR="${VERSION_MAJOR}"
    APP_VERSION_MINOR="${VERSION_MINOR}"
    APP_VERSION_PATCH="${VERSION_PATCH}"
    APP_VERSION="${PROJECT_VERSION}"
    BUILD_YEAR="${CURRENT_YEAR}"
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/controls
    ${CMAKE_CURRENT_SOURCE_DIR}/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/jsobjects
    ${CMAKE_CURRENT_SOURCE_DIR}/modbusmessages
)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/sources.cmake")
omodsim_configure_ui_enum_normalization(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)
target_sources(${PROJECT_NAME} PRIVATE resources.qrc ${HEADERS} ${SOURCES} ${UI_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt::Widgets Qt::Network Qt::Xml Qt::PrintSupport Qt::SerialBus Qt::SerialPort Qt::Qml Qt::Help Qt::Svg)

omodsim_configure_qt_target(${PROJECT_NAME})
omodsim_configure_qlementine_app_style(${PROJECT_NAME})

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/translations.cmake")

if(WIN32)
    omodsim_configure_target_windows(${PROJECT_NAME})
elseif(APPLE)
    omodsim_configure_target_macos(${PROJECT_NAME})
elseif(LINUX)
    omodsim_configure_target_linux(${PROJECT_NAME})
endif()

omodsim_configure_help_generator(${PROJECT_NAME})
omodsim_configure_uninstall_target()

# Installation rules
if(WIN32)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_install_windows.cmake")
elseif(APPLE)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_install_macos.cmake")
elseif(LINUX)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_install_linux.cmake")
endif()

if(OMODSIM_BUILD_TESTS)
    enable_testing()
    omodsim_configure_tests()
endif()
