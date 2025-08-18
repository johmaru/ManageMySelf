# Suggested Commands & Workflows (Updated 2025-08-18)

## Development Environment Setup

### Prerequisites Installation
```powershell
# Ensure you have:
# - Windows with MinGW toolchain
# - Qt 6.x (e.g., 6.9.1) with MinGW
# - Conan 2.x
# - CMake 3.31+
```

### Environment Variables (Optional but Recommended)
```powershell
# Set Qt path for CMake discovery
$env:QT_PREFIX_PATH = "C:/Qt/6.9.1/mingw_64"

# Set QML import path for runtime (if needed)
$env:QT_QML_IMPORT_PATH = "C:/Qt/6.9.1/mingw_64/qml"
```

## Project Setup & Build

### Initial Setup (First Time)
```powershell
# Clone and navigate to project
cd C:\Users\Johma_sub\CLionProjects\ManageMySelf

# Create build directory
mkdir build -ea 0 | Out-Null
cd build

# Install dependencies via Conan
conan install .. --output-folder=. --build=missing -s build_type=Debug

# Configure CMake with Conan toolchain
cmake .. "-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
```

### Regular Development Workflow

#### Build Commands
```powershell
# Standard build
cmake --build .

# Build and run (using custom CMake target)
cmake --build . --target run

# Build with verbose output (for debugging build issues)
cmake --build . --verbose

# Clean build
cmake --build . --target clean
```

#### Development Iterations
```powershell
# After code changes - quick build and test
cmake --build . --target run

# After QML changes only (no need to rebuild C++)
cmake --build . --target run
```

## Translation Management

### Update Translation Files
```powershell
# Generate/update .ts files from source code
cmake --build . --target update_translations

# After updating .ts files with linguist or manually
# Rebuild to generate .qm files
cmake --build .
```

### Language Testing
```powershell
# Run with specific language (if supported via command line)
cmake --build . --target run
# Then change language in application settings
```

## Database Management

### Reset Application Data
```powershell
# Remove all application data (careful!)
Remove-Item "$env:USERPROFILE\Documents\ManageMySelf" -Recurse -Force -ErrorAction SilentlyContinue

# Application will recreate database on next run
cmake --build . --target run
```

### Backup Application Data
```powershell
# Backup all application data
$backupDate = Get-Date -Format "yyyyMMdd_HHmmss"
Copy-Item "$env:USERPROFILE\Documents\ManageMySelf" "$env:USERPROFILE\Documents\ManageMySelf_backup_$backupDate" -Recurse
```

## Debugging & Troubleshooting

### Build Issues
```powershell
# Clean entire build directory
cd build
Remove-Item * -Recurse -Force

# Full rebuild from scratch
conan install .. --output-folder=. --build=missing -s build_type=Debug
cmake .. "-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build . --target run
```

### Runtime Issues
```powershell
# Check QML import paths
$env:QT_QML_DEBUG = "1"
cmake --build . --target run

# Verify DLL dependencies (if runtime errors)
# The built executable should be in build/ManageMySelf.exe
# windeployqt runs automatically post-build
```

### Database Debugging
```powershell
# Check if main database exists
Test-Path "$env:USERPROFILE\Documents\ManageMySelf\ManageMySelf.db"

# View SQLite database content (requires SQLite CLI)
sqlite3 "$env:USERPROFILE\Documents\ManageMySelf\ManageMySelf.db" ".tables"
sqlite3 "$env:USERPROFILE\Documents\ManageMySelf\ManageMySelf.db" "SELECT * FROM workspaces;"
```

## Development Tools & Utilities

### Code Analysis
```powershell
# Generate compile_commands.json is automatic with CMake
# Located at: build/compile_commands.json
# Useful for IDEs and static analysis tools
```

### Resource Verification
```powershell
# Check if QML resources are properly embedded
# Look for qrc_*.cpp files in build directory
ls build/qrc_*.cpp

# Verify translation resources
ls build/*.qm
```

## Deployment Preparation

### Release Build
```powershell
# Configure for Release
conan install .. --output-folder=. --build=missing -s build_type=Release
cmake .. "-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Build release version
cmake --build . --config Release
```

### Package Verification
```powershell
# After build, verify all Qt dependencies are deployed
# windeployqt runs automatically, check for Qt DLLs in build directory
ls build/*.dll

# Test executable independently
cd build
./ManageMySelf.exe
```

## Performance & Optimization

### Build Performance
```powershell
# Parallel build (utilize multiple cores)
cmake --build . --parallel 4

# Monitor build time
Measure-Command { cmake --build . }
```

### Runtime Performance
```powershell
# Enable Qt logging for performance analysis
$env:QT_LOGGING_RULES = "qt.qml.binding.removal.info=true"
cmake --build . --target run
```

## Version Control Integration

### Pre-commit Checks
```powershell
# Verify build works before committing
cmake --build . --target clean
cmake --build . --target run

# Update translations before committing i18n changes
cmake --build . --target update_translations
```

### Branch Switching
```powershell
# After switching branches, clean and rebuild
cmake --build . --target clean
cmake --build .
```

## Common Environment Issues

### MinGW libssp.a Missing
- **Automatic handling**: CMakeLists.txt includes `-fno-stack-protector` workaround
- **Alternative**: Install libssp manually for MinGW

### Qt Path Issues
- **Solution**: Set `QT_PREFIX_PATH` environment variable
- **Verification**: Check CMAKE_PREFIX_PATH in CMake output

### QML Module Loading Issues
- **Solution**: Set `QT_QML_IMPORT_PATH` environment variable
- **Debug**: Enable QML debugging with `QT_QML_DEBUG=1`

### Conan Issues
- **Profile problems**: Check conan profile with `conan profile detect --force`
- **Cache issues**: Clear conan cache with `conan cache clean "*"`

## Daily Development Commands (Quick Reference)
```powershell
# Most common workflow:
cd build
cmake --build . --target run

# After significant changes:
cmake --build . --target clean
cmake --build . --target run

# Translation updates:
cmake --build . --target update_translations
cmake --build .
```