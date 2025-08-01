# Warning

If Mingw not exsist an 'libssp.a' file in Mingw.
Please add the that file.

If want compile the this project,Which need an Conan PackageManager.

# How to use

1. 
    - changed a path in CMakeLists.txt for Qt folder
    - reload CMake project
    - `cd build`
    - `conan install .. --output-folder=. --build=missing -s build_type=Debug`
    - `cmake --build . --target run`


# How to clean a CmakeLists data

Excute in `./build`

- `Remove-Item * -Recurse -Force`

If you not valid that path,Can change
- `$env:CC="C:/Qt/Tools/mingw1310_64/bin/gcc.exe"`

- `$env:CXX="C:/Qt/Tools/mingw1310_64/bin/g++.exe"`

- `conan install .. --build=missing -of . `

Can change -DCMAKE_PREFIX_PATH
- `cmake .. "-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake" -DCMAKE_PREFIX_PATH="C:/Qt/6.9.1/mingw_64" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug`

- `cmake --build .`