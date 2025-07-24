cmake_minimum_required(VERSION 3.31)
project(ManageMySelf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Qml Quick QuickControls2)

add_executable(ManageMySelf main.cpp
        qml.qrc
        fs/global_settings.cpp
        fs/global_settings.h
        fs/JsonSettingsBase.cpp
        fs/JsonSettingsBase.h)

#set_target_properties(ManageMySelf PROPERTIES WIN32_EXECUTABLE ON)

target_link_libraries(ManageMySelf
        Qt::Core
        Qt::Widgets
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

if(WIN32)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND $<TARGET_FILE:Qt6::windeployqt>
            ARGS
            --dir $<TARGET_FILE_DIR:${PROJECT_NAME}>
            --qmldir ${CMAKE_CURRENT_SOURCE_DIR}
            $<TARGET_FILE:${PROJECT_NAME}>
            VERBATIM
    )
endif()