cmake_minimum_required(VERSION 3.31)
project(ManageMySelf)
set(QT_QML_GENERATE_QMLLS_INI ON)

set(CMAKE_C_COMPILER "C:/Qt/Tools/mingw1310_64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/Qt/Tools/mingw1310_64/bin/g++.exe")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    LinguistTools 
    Gui 
    Qml 
    Quick 
    QuickControls2 
    QuickLayouts
)

qt_policy(SET QTP0001 NEW)

set(QT_QMLLS_PLUGINS ON)

# lupdateとlreleaseの実行可能ファイルパスを取得
get_target_property(Qt6_LUPDATE_EXECUTABLE Qt6::lupdate IMPORTED_LOCATION)
get_target_property(Qt6_LRELEASE_EXECUTABLE Qt6::lrelease IMPORTED_LOCATION)

# 翻訳ファイルの定義
set(TS_FILES
    ${CMAKE_SOURCE_DIR}/i18n/ManageMySelf_ja.ts
    ${CMAKE_SOURCE_DIR}/i18n/ManageMySelf_en.ts
)

# .qmファイルの出力ディレクトリを設定
set(QM_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${QM_DIR})

# .qmファイルの生成
set(QM_FILES)
foreach(TS_FILE ${TS_FILES})
    get_filename_component(TS_NAME ${TS_FILE} NAME_WE)
    set(QM_FILE ${QM_DIR}/${TS_NAME}.qm)
    list(APPEND QM_FILES ${QM_FILE})

    # lreleaseの実行可能ファイルが見つかった場合のみカスタムコマンドを追加
    if(Qt6_LRELEASE_EXECUTABLE)
        add_custom_command(
            OUTPUT ${QM_FILE}
            COMMAND ${Qt6_LRELEASE_EXECUTABLE} ${TS_FILE} -qm ${QM_FILE}
            DEPENDS ${TS_FILE}
            COMMENT "Generating ${QM_FILE}"
        )
    else()
        message(WARNING "Qt lrelease executable not found. Translation files will not be generated.")
    endif()
endforeach()

get_target_property(Qt6_RCC_EXECUTABLE Qt6::rcc IMPORTED_LOCATION)
set(I18N_QRC_FILE ${CMAKE_SOURCE_DIR}/i18n/i18n.qrc)
set(I18N_RCC_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/qrc_translations.cpp)

# ビルドディレクトリにQRCファイルをコピーする
set(I18N_QRC_FILE_BUILD ${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc)
add_custom_command(
        OUTPUT ${I18N_QRC_FILE_BUILD}
        COMMAND ${CMAKE_COMMAND} -E copy ${I18N_QRC_FILE} ${I18N_QRC_FILE_BUILD}
        DEPENDS ${I18N_QRC_FILE}
        COMMENT "Copying QRC file to build directory"
)

# コピーされたQRCファイルを使ってrccを実行する
add_custom_command(
        OUTPUT ${I18N_RCC_OUTPUT}
        COMMAND ${Qt6_RCC_EXECUTABLE} -g cpp -o ${I18N_RCC_OUTPUT} ${I18N_QRC_FILE_BUILD}
        DEPENDS ${I18N_QRC_FILE_BUILD} ${QM_FILES}
        WORKING_DIRECTORY ${QM_DIR}
        COMMENT "Compiling translations QRC ${I18N_QRC_FILE}"
)

# QMLリソースの追加
qt_add_resources(QM_FILES "qml.qrc")

# 翻訳ファイルの更新用カスタムターゲット
if(Qt6_LUPDATE_EXECUTABLE)
    add_custom_target(update_translations
        COMMAND ${Qt6_LUPDATE_EXECUTABLE} ${CMAKE_SOURCE_DIR} -ts ${TS_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Updating translation files..."
    )
else()
    add_custom_target(update_translations
        COMMAND ${CMAKE_COMMAND} -E echo "Qt lupdate executable not found. Please install Qt Linguist tools."
        COMMENT "Qt lupdate not available"
    )
endif()

# 実行
add_custom_target(run
    COMMAND ${CMAKE_PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ManageMySelf..."
)

add_executable(ManageMySelf
        ${QM_FILES}
        main.cpp
        fs/global_settings.cpp
        fs/JsonSettingsBase.cpp
        ${I18N_RCC_OUTPUT}
)

target_include_directories(ManageMySelf PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(ManageMySelf
        Qt::Core
        Qt::Widgets
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickLayouts
)

if(Qt6_Qml_FOUND)
    get_target_property(QT_QML_IMPORT_PATH Qt6::Qml IMPORTED_LOCATION)
    get_filename_component(QT_QML_IMPORT_PATH "${QT_QML_IMPORT_PATH}/../../qml" ABSOLUTE)
    set(QML_IMPORT_PATH "${QT_QML_IMPORT_PATH}" CACHE STRING "Qt QML import paths" FORCE)
endif()

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