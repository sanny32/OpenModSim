include("${CMAKE_CURRENT_LIST_DIR}/openssl_qt_abi.cmake")
if(Qt6_FOUND)
    set(qt_package Qt6)
else()
    set(qt_package Qt5)
endif()
omodsim_openssl_qt_abi(openssl_abi "${qt_package}")
omodsim_openssl_version_range(openssl_version_range "${openssl_abi}")

if(WIN32 AND openssl_abi MATCHES "^(1\\.1|3)$" AND
   NOT DEFINED OPENSSL_ROOT_DIR AND NOT DEFINED ENV{OPENSSL_ROOT_DIR})
    include("${CMAKE_CURRENT_LIST_DIR}/openssl_qt_hint.cmake")
    omodsim_openssl_qt_hint(omodsim_openssl_hint "${QT_BINARY_DIR}" "${openssl_abi}" "${CMAKE_SIZEOF_VOID_P}")
    if(EXISTS "${omodsim_openssl_hint}/include/openssl/opensslv.h")
        set(OPENSSL_ROOT_DIR "${omodsim_openssl_hint}")
    endif()
endif()

if(OMODSIM_OPENSSL_PROBE)
    find_package(OpenSSL ${openssl_version_range} QUIET COMPONENTS Crypto)
    if(OpenSSL_FOUND)
        file(WRITE "${OMODSIM_OPENSSL_PROBE}" "found")
    else()
        file(WRITE "${OMODSIM_OPENSSL_PROBE}" "${openssl_abi}")
    endif()
    return()
endif()

if(APPLE)
    find_package(OpenSSL ${openssl_version_range} REQUIRED COMPONENTS Crypto SSL)
else()
    find_package(OpenSSL ${openssl_version_range} REQUIRED COMPONENTS Crypto)
endif()
set(omodsim_openssl_license "OpenSSL.txt")
if(OPENSSL_VERSION VERSION_LESS "3")
    set(omodsim_openssl_license "OpenSSL-1.1.txt")
endif()

function(omodsim_deploy_openssl target_name)
    if(WIN32)
        get_filename_component(openssl_bin "${OPENSSL_INCLUDE_DIR}/../bin" ABSOLUTE)
        file(GLOB openssl_runtime "${openssl_bin}/libcrypto-*.dll" "${openssl_bin}/libssl-*.dll")
        if(NOT openssl_runtime)
            message(FATAL_ERROR "OpenSSL runtime DLLs not found in ${openssl_bin}")
        endif()
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${openssl_runtime} "$<TARGET_FILE_DIR:${target_name}>"
            VERBATIM
        )
    endif()
endfunction()

configure_file("${PROJECT_SOURCE_DIR}/../third_party/licenses/${omodsim_openssl_license}"
               "${PROJECT_BINARY_DIR}/docs/licenses/OpenSSL.txt" COPYONLY)
