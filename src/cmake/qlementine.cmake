if(APPLE)
    option(USE_QLEMENTINE_APP_STYLE "Use Qlementine application style when available (requires Qt6 >= 6.8)" ON)
else()
    option(USE_QLEMENTINE_APP_STYLE "Use Qlementine application style when available (requires Qt6 >= 6.8)" OFF)
endif()

if(USE_QLEMENTINE_APP_STYLE AND Qt6_FOUND AND Qt6_VERSION VERSION_GREATER_EQUAL "6.8")
    include(FetchContent)

    set(QLEMENTINE_VERSION "1.4.2")
    FetchContent_Declare(qlementine
        GIT_REPOSITORY "https://github.com/oclero/qlementine.git"
        GIT_TAG        "v${QLEMENTINE_VERSION}"
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(qlementine)

    set(QLEMENTINE_ICONS_VERSION "1.15.0")
    FetchContent_Declare(qlementine-icons
        GIT_REPOSITORY "https://github.com/oclero/qlementine-icons.git"
        GIT_TAG        "v${QLEMENTINE_ICONS_VERSION}"
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(qlementine-icons)
endif()

if(USE_QLEMENTINE_APP_STYLE AND
   NOT (Qt6_FOUND AND Qt6_VERSION VERSION_GREATER_EQUAL "6.8" AND TARGET qlementine AND TARGET qlementine-icons))
    set(USE_QLEMENTINE_APP_STYLE OFF CACHE BOOL
        "Use Qlementine application style when available (requires Qt6 >= 6.8)" FORCE)
endif()

function(omodsim_configure_qlementine_app_style target_name)
    if(USE_QLEMENTINE_APP_STYLE)
        target_compile_definitions(${target_name} PRIVATE
            HAVE_QLEMENTINE_APP_STYLE
            QLEMENTINE_VERSION="${QLEMENTINE_VERSION}"
            QLEMENTINE_ICONS_VERSION="${QLEMENTINE_ICONS_VERSION}"
        )
        target_link_libraries(${target_name} PRIVATE qlementine qlementine-icons)
    endif()
endfunction()
