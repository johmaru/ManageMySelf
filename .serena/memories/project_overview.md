# ManageMySelf — Project Overview (Updated 2025-08-18)

## Purpose & Core Concept
Qt/QML デスクトップアプリケーション。個人の「ワークスペース」作成・管理・日記記録機能を提供。
- **主要機能**: 
  - ワークスペース管理（作成、開く、削除）
  - 日記エントリの作成・閲覧（SQLite 保存、Markdown 対応）
  - 設定管理（言語、テーマ、ウィンドウサイズ）
  - 多言語対応（英語・日本語）
  - マテリアルデザインUI

## Tech Stack & Platform
- **Primary Platform**: Windows (MinGW toolchain)
- **Language**: C++20
- **Build System**: CMake 3.31+, Conan 2.x
- **UI Framework**: Qt 6.x (specifically 6.9.1), QML with Material theme
- **Database**: SQLite with SQLiteCpp 3.3.1
- **Dependencies**: 
  - Qt6 (Core, Gui, Qml, Quick, QuickControls2, QuickLayouts, LinguistTools)
  - SQLiteCpp (via Conan: sqlitecpp/3.3.1)

## Project Structure
```
ManageMySelf/
├── CMakeLists.txt           # Main build configuration
├── conanfile.py            # Dependency management
├── main.cpp                # Application entry point
├── qml.qrc                 # QML resource file
├── control/gui/            # QML UI components
│   ├── Main.qml           # Main application window
│   ├── MainUserPage.qml   # User workspace view
│   ├── Settings.qml       # Settings dialog
│   ├── CreateWorkspaceForm.qml  # Workspace creation form
│   ├── MarkDownViewer.qml # Markdown display component
│   └── qmldir             # QML module definition
├── fs/                     # File system & database layer
│   ├── global_settings.h/cpp    # Application settings manager
│   ├── JsonSettingsBase.h/cpp   # JSON settings base class
│   ├── SqLiteBase.h/cpp         # Main SQLite database handler
│   └── UserSql.h/cpp            # User workspace database handler
├── os/                     # OS utilities
│   └── SqlOS.h/cpp        # OS detection and date/time utilities
└── i18n/                   # Internationalization
    ├── i18n.qrc           # Translation resources
    ├── ManageMySelf_en.ts # English translations
    └── ManageMySelf_ja.ts # Japanese translations
```

## Application Architecture
### Entry Point (main.cpp)
- QGuiApplication + QQmlApplicationEngine setup
- GlobalSettings initialization and context property exposure
- QML import path configuration (with environment variable support)
- Main database initialization via SqLiteBase
- Translation system setup
- Resource loading (qrc:/Main.qml)

### Data Architecture
#### Main Application Data
- **Location**: `%USERPROFILE%/Documents/ManageMySelf/`
- **Files**:
  - `settings.json`: Application settings (theme, language, window size)
  - `ManageMySelf.db`: Main database (recent files, workspaces list)

#### Workspace Data
- **Location**: `<user-selected-path>/<workspace-name>/`
- **Files**:
  - `user.db`: SQLite database with diary entries
  - `diaries/`: Directory containing Markdown diary files
  - `settings.json`: Workspace-specific settings

### Key Classes & Components
#### File System Layer (`fs/`)
- **GlobalSettings**: 
  - QObject-based settings manager
  - Handles application configuration, theme switching, language switching
  - Methods: createWorkspaceFromQml, getWorkspaceWithName, createDiary, etc.
  - Signals: themeChanged, languageChanged, workspaceCreated
- **SqLiteBase**: Main application database handler
- **UserSql**: Workspace-specific database operations
- **JsonSettingsBase**: Base class for JSON configuration handling

#### OS Layer (`os/`)
- **SqlOS**: OS detection and date/time utilities
- Enums: OS types (Windows, Linux, macOS), DateTimeFormat options

### UI Layer (`control/gui/`)
- **Material Design**: Qt Quick Controls 2 with Material theme
- **Main.qml**: ApplicationWindow root component
- **MainUserPage.qml**: Primary user interface for workspace interaction
- **Settings.qml**: Configuration interface
- **CreateWorkspaceForm.qml**: Workspace creation dialog
- **MarkDownViewer.qml**: Markdown content display

## Build System Features
### CMake Configuration
- Automatic MOC/RCC/UIC processing
- Custom targets: `update_translations`, `run`
- MinGW compatibility fixes (libssp.a workaround with -fno-stack-protector)
- Qt6 windeployqt integration for deployment
- Resource compilation with qt_add_resources

### Conan Integration
- Dependency management for SQLiteCpp
- Debug/Release build type support
- Toolchain file generation

## Quick Start Commands
```powershell
# Setup and build
mkdir build -ea 0 | Out-Null
cd build
conan install .. --output-folder=. --build=missing -s build_type=Debug
cmake .. "-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --target run
```

## Runtime Environment
### Environment Variables
- `QT_PREFIX_PATH`: Qt installation path (e.g., "C:/Qt/6.9.1/mingw_64")
- `QT_QML_IMPORT_PATH`: QML module import path override

### Data Locations
- Application data: `%USERPROFILE%/Documents/ManageMySelf/`
- User workspaces: User-defined locations
- Build artifacts: `build/` directory

## Current Status & Known Issues
### Strengths
- Complete Qt/QML application with modern Material UI
- Robust SQLite database integration
- Multi-language support infrastructure
- Comprehensive workspace management
- Clean separation of concerns (UI, data, OS utilities)

### Areas for Improvement
- Hardcoded paths in some configurations
- Test coverage not yet implemented
- Code formatting standards not enforced (.clang-format missing)
- Documentation could be expanded
- Error handling could be more comprehensive

## Development Workflow
1. **Dependencies**: Install via Conan
2. **Configuration**: CMake with toolchain file
3. **Building**: MinGW Makefiles generator
4. **Testing**: Manual testing (automated tests to be implemented)
5. **Translation**: CMake target `update_translations` for i18n updates
6. **Deployment**: Automated via windeployqt

This project represents a mature Qt/QML desktop application with solid architecture for personal workspace and diary management.