function(cameralink_copy_opencv_runtime target)
    set(opencv_bin_dir "${CMAKE_CURRENT_SOURCE_DIR}/opencv/build/x64/vc16/bin")
    set(opencv_runtime
        "$<IF:$<CONFIG:Debug>,${opencv_bin_dir}/opencv_world481d.dll,${opencv_bin_dir}/opencv_world481.dll>"
    )
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${opencv_runtime}"
            "$<TARGET_FILE_DIR:${target}>"
        COMMENT "Deploying OpenCV runtime for ${target}"
        VERBATIM
    )
endfunction()
function(cameralink_deploy_qt_runtime target)
    get_filename_component(qt_bin_dir "${Qt6_DIR}/../../../bin" ABSOLUTE)
    set(windeployqt "${qt_bin_dir}/windeployqt.exe")
    if(NOT EXISTS "${windeployqt}")
        message(FATAL_ERROR "windeployqt was not found at ${windeployqt}")
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${windeployqt}"
            --no-translations
            --no-compiler-runtime
            --no-system-d3d-compiler
            "$<TARGET_FILE:${target}>"
        COMMENT "Deploying Qt runtime for ${target}"
        VERBATIM
    )
endfunction()
