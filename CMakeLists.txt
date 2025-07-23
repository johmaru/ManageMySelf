cmake_minimum_required(VERSION 3.31)
project(ManageMySelf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 REQUIRED COMPONENTS Core Widgets CoreTools)

add_executable(ManageMySelf main.cpp
        global_settings.cpp
        global_settings.h
        JsonSettingsBase.cpp
        JsonSettingsBase.h)

set_target_properties(ManageMySelf PROPERTIES WIN32_EXECUTABLE ON)

target_link_libraries(ManageMySelf
        Qt::Core
        Qt::Widgets
)

if(WIN32)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND $<TARGET_FILE:Qt6::windeployqt>
        ARGS --dir $<TARGET_FILE_DIR:${PROJECT_NAME}> $<TARGET_FILE:${PROJECT_NAME}>
        VERBATIM
    )
endif()
