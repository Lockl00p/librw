add_compile_definitions(__EMSCRIPTEN__)
add_compile_definitions(RW_GL3 RW_GLES2)
add_link_options(-sFULL_ES2 --use-port=contrib.glfw3)
add_compile_options(--use-port=contrib.glfw3)
if(HTML)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(SINGLE_FILE)
endif()

function(librw_platform_target TARGET)
    cmake_parse_arguments(LPT "INSTALL" "" "" ${ARGN})

    get_target_property(TARGET_TYPE "${TARGET}" TYPE)
    if(TARGET_TYPE STREQUAL "EXECUTABLE")
        if(SINGLE_FILE)
            target_link_options(${TARGET} PUBLIC '-sSINGLE_FILE')
        endif()
        if(EMBED)
            if(SINGLE_FILE)
                target_link_options(${TARGET} PUBLIC '--embed-file ${CMAKE_CURRENT_SOURCE_DIR}/preload')
            else()
                target_link_options(${TARGET} PUBLIC '--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/preload')
            endif()
        endif()

        if(LIBRW_INSTALL AND LPT_INSTALL)
            get_target_property(TARGET_OUTPUT_NAME ${TARGET} OUTPUT_NAME)
            if(NOT TARGET_OUTPUT_NAME)
                set(TARGET_OUTPUT_NAME "${TARGET}")
            endif()

            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_OUTPUT_NAME}"
                DESTINATION "${CMAKE_INSTALL_BINDIR}"
            )
        endif()
    endif()
endfunction()




