# Suggested Commands (Windows PowerShell)

## Setup / Configure
- (一度だけ) Conan 依存解決
  - cd build
  - conan install .. --output-folder=. --build=missing -s build_type=Debug

- CMake 生成 (必要なら Qt のパスを調整)
  - cmake .. "-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake" -DCMAKE_PREFIX_PATH="C:/Qt/6.9.1/mingw_64" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug

## Build / Run
- ビルド
  - cmake --build .

- 実行 (CMake の custom target)
  - cmake --build . --target run

## Clean (ビルドディレクトリ内)
- Remove-Item * -Recurse -Force

## Translations
- 翻訳ファイルの更新 (CMake ターゲット)
  - cmake --build . --target update_translations

## Notes
- MinGW で libssp.a が無い場合は CMakeLists により -fno-stack-protector を付与済み。
- Qt の QML import path が main.cpp と CMakeLists で固定指定のため、Qt バージョン/パス変更時は修正が必要。
