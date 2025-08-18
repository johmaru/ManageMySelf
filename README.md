# ManageMySelf (Qt/QML, C++20)

A Windows desktop app to manage personal workspaces and diaries. Uses Qt 6, QML (Material), SQLite (SQLiteCpp), CMake, and Conan.

## Prerequisites
- Windows + MinGW toolchain
- Qt 6.x (e.g., 6.9.1) with MinGW
- Conan 2.x
- CMake 3.31+

Note: If MinGW lacks `libssp.a`, this project disables stack protector via `-fno-stack-protector` in CMake. Alternatively install `libssp`.

## Quick Start (PowerShell)

```powershell
# 1) Create build dir
mkdir build -ea 0 | Out-Null
cd build

# 2) Install dependencies via Conan
conan install .. --output-folder=. --build=missing -s build_type=Debug

# 3) Configure CMake (Qt path from env QT_PREFIX_PATH if set)
cmake .. "-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

# 4) Build + Run
cmake --build . --target run
```

Tips:
- To point CMake to your Qt, set env variable before step (3):
  ```powershell
  $env:QT_PREFIX_PATH = "C:/Qt/6.9.1/mingw_64"
  ```
- To override QML import path at runtime, set:
  ```powershell
  $env:QT_QML_IMPORT_PATH = "C:/Qt/6.9.1/mingw_64/qml"
  ```

## Cleaning build directory
```powershell
cd build
Remove-Item * -Recurse -Force
```

## Project Structure
- `control/gui`: QML UI (Main.qml, Settings.qml, CreateWorkspaceForm.qml, MainUserPage.qml)
- `fs`: GlobalSettings, JsonSettingsBase, SqLiteBase, UserSql
- `os`: SqlOS utilities
- `i18n`: Translations; `update_translations` CMake target available

## Runtime data locations
- `%USERPROFILE%/Documents/ManageMySelf/settings.json`
- `%USERPROFILE%/Documents/ManageMySelf/ManageMySelf.db`
- Each workspace: `<chosen>/<workspace>/user.db`, `diaries/`, `settings.json`

## Troubleshooting
- QML fails to load: verify `qml.qrc` resources and QML import paths. You can set `$env:QT_QML_IMPORT_PATH`.
- Qt not found by CMake: set `$env:QT_PREFIX_PATH`.
- First run DB dir: automatically created now; check `Documents/ManageMySelf` exists.
