function(omodsim_configure_target_linux target_name)
    target_link_options(${target_name} PRIVATE -static-libgcc -static-libstdc++)
endfunction()
