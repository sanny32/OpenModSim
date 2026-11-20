add_executable(${PROJECT_NAME}
    main.cpp
    resources.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE omodsim_ui)
omodsim_deploy_openssl(${PROJECT_NAME})
omodsim_apply_coverage(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)

omodsim_generate_files()
include("${PROJECT_SOURCE_DIR}/cmake/translations.cmake")

if(WIN32)
    omodsim_configure_target_windows(${PROJECT_NAME})
elseif(APPLE)
    omodsim_configure_target_macos(${PROJECT_NAME})
elseif(LINUX)
    omodsim_configure_target_linux(${PROJECT_NAME})
endif()

omodsim_configure_help_generator(${PROJECT_NAME})

if(WIN32)
    include("${PROJECT_SOURCE_DIR}/cmake/cmake_install_windows.cmake")
elseif(APPLE)
    include("${PROJECT_SOURCE_DIR}/cmake/cmake_install_macos.cmake")
elseif(LINUX)
    include("${PROJECT_SOURCE_DIR}/cmake/cmake_install_linux.cmake")
endif()
