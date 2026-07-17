file(GLOB TOOL_SRCS CONFIGURE_DEPENDS src/*.cpp)

foreach(SRC ${TOOL_SRCS})
    get_filename_component(TOOL_NAME ${SRC} NAME_WE)

    add_executable(${TOOL_NAME} ${SRC})

    target_link_libraries(${TOOL_NAME} PRIVATE mylibc)

    target_link_options(${TOOL_NAME} PRIVATE
        -nostdlib
        -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    )

    set_target_properties(${TOOL_NAME} PROPERTIES
        LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
        FOLDER "mytools"
    )

    add_custom_command(TARGET ${TOOL_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BOOT_OUT_DIR}
        COMMAND objcopy -O binary
            --only-section=.text --only-section=.rodata --only-section=.data
            $<TARGET_FILE:${TOOL_NAME}>
            ${BOOT_OUT_DIR}/${TOOL_NAME}.o
        VERBATIM
    )

    list(APPEND TOOL_TARGETS ${TOOL_NAME})
endforeach()

# makeimg 가 의존할 수 있도록 tool 실행파일들을 하나로 집계
add_custom_target(tools_all DEPENDS ${TOOL_TARGETS})
