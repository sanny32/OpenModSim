find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)

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

configure_file("${PROJECT_SOURCE_DIR}/../third_party/licenses/OpenSSL.txt"
               "${PROJECT_BINARY_DIR}/docs/licenses/OpenSSL.txt" COPYONLY)
