include("${CMAKE_CURRENT_LIST_DIR}/../cmake/openssl_qt_hint.cmake")

omodsim_openssl_qt_hint(actual "C:/Qt/6.9.3/msvc2022_64/bin" 3 8)
if(NOT actual STREQUAL "C:/Qt/Tools/OpenSSLv3/Win_x64")
    message(FATAL_ERROR "Unexpected Qt 6 OpenSSL path: ${actual}")
endif()

omodsim_openssl_qt_hint(actual "C:/Qt/5.15.2/msvc2019/bin" 1.1 4)
if(NOT actual STREQUAL "C:/Qt/Tools/OpenSSL11/Win_x86")
    message(FATAL_ERROR "Unexpected Qt 5 OpenSSL path: ${actual}")
endif()
