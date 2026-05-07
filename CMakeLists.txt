cmake_minimum_required(VERSION 3.16)
project(YGsoftware) 

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_definitions("-DUNICODE" "-D_UNICODE")                                   # Windows API 使用宽字符版本
    add_compile_options(/utf-8)                                                 # 源文件按 UTF-8 解析
endif()
    
add_library(imgui
    thirdParty/imgui/imgui.cpp
    thirdParty/imgui/imgui_draw.cpp
    thirdParty/imgui/imgui_widgets.cpp
    thirdParty/imgui/imgui_tables.cpp
    thirdParty/imgui/backends/imgui_impl_win32.cpp
    thirdParty/imgui/backends/imgui_impl_dx11.cpp
 )

target_include_directories(imgui PUBLIC
    thirdParty/imgui
    thirdParty/imgui/backends
)
# ================= projects =================
set(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/source")

# 获取 source 下所有子目录（project1、project2...）
file(GLOB CHILDREN RELATIVE ${PROJECT_ROOT} ${PROJECT_ROOT}/*)

foreach(CHILD ${CHILDREN})

    set(PROJECT_DIR "${PROJECT_ROOT}/${CHILD}")

    # 只处理目录
    if(IS_DIRECTORY ${PROJECT_DIR})

        # 收集当前 project 下所有 cpp
        file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
            "${PROJECT_DIR}/*.cpp"
        )

        if(PROJECT_SOURCES)

            # 创建独立 exe（无控制台）
            add_executable(${CHILD} WIN32 ${PROJECT_SOURCES})

            # 当前 project 头文件目录
            target_include_directories(${CHILD} PRIVATE
                "${PROJECT_DIR}"
            )

            # 链接库
            target_link_libraries(${CHILD} PRIVATE
                d3d11
                dxgi
                d3dcompiler
                imgui
            )

            # 输出目录
            set_target_properties(${CHILD} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CHILD}"
            )

            # ================= shader copy =================
            file(GLOB_RECURSE HLSL_SHADERS
                "${PROJECT_DIR}/*.hlsl"
            )

            if(HLSL_SHADERS)
                add_custom_command(TARGET ${CHILD} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${HLSL_SHADERS}
                        $<TARGET_FILE_DIR:${CHILD}>
                    COMMENT "Copying shaders for ${CHILD}"
                )
            endif()

        endif()
    endif()

endforeach()