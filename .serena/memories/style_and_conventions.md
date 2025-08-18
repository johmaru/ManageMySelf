# Style & Conventions (Updated 2025-08-18)

## Language & Standards
- **C++20** with Qt coding conventions
- **Qt 6.x** framework patterns and best practices
- **Modern C++** features where appropriate

## Naming Conventions
### C++ Classes & Methods
- **Classes**: PascalCase
  - Examples: `GlobalSettings`, `SqLiteBase`, `UserSql`, `SqlOS`
- **Methods**: camelCase
  - Examples: `createWorkspaceFromQml`, `getWorkspaces`, `getDiariesByMonthJson`
- **Member Variables**: m_ prefix
  - Examples: `m_language`, `m_windowSize`, `m_theme`, `m_translator`
- **Constants**: UPPER_SNAKE_CASE
  - Examples: `SETTINGS_DIR_NAME`
- **Enums**: PascalCase for enum class, UPPER_SNAKE_CASE for values
  - Examples: `OS::Windows`, `DateTimeFormat::YYYY_MM_DD`

### QML Components
- **Files**: PascalCase (Main.qml, Settings.qml, CreateWorkspaceForm.qml)
- **Properties**: camelCase following Qt conventions
- **Functions**: camelCase

## Qt Framework Patterns
### Qt Object System
- **Q_OBJECT** macro for signal-slot system
- **Q_PROPERTY** for QML-accessible properties
- **Q_INVOKABLE** for methods callable from QML
- **Q_ENUM** for exposing enums to QML

### Signals & Slots
- **Signals**: descriptive names with "Changed" suffix for property changes
  - Examples: `windowSizeChanged`, `themeChanged`, `languageChanged`, `workspaceCreated`
- **Slots**: action-oriented method names
- **Connections**: Prefer new-style connect syntax

## Error Handling Patterns
### Return Value Conventions
- **Integer returns**: 
  - `>= 0`: Success (often with meaningful value)
  - `< 0` or specific error values: Various error states
- **Enum returns**: Custom enum classes for specific error types
  - Example: `WorkspaceResult` enum with Success, FailedToCreateDirectory, etc.

### Logging Standards
- **qInfo()**: General information, successful operations
- **qWarning()**: Non-critical issues, fallback scenarios
- **qCritical()**: Critical errors that may cause application failure
- **qDebug()**: Development debugging (should be minimal in production)

## File & Path Handling
### Path Management
- **Base directories**: Use `QStandardPaths` (particularly `DocumentsLocation`)
- **Path operations**: Prefer `QDir` and `QFileInfo` over string manipulation
- **File operations**: Use `QFile`, `QTextStream` for text files
- **Cross-platform**: Avoid hardcoded path separators, use QDir::separator()

### Resource Management
- **QRC files**: Organized by functionality (qml.qrc, i18n.qrc)
- **Resource paths**: Use qrc:/ prefix consistently
- **Asset organization**: Logical grouping by feature/module

## QML/UI Standards
### UI Framework
- **Qt Quick Controls 2** with **Material** theme
- **Responsive design**: Adapt to different screen sizes
- **Accessibility**: Consider screen readers and keyboard navigation

### Internationalization
- **Translation strings**: Use `qsTr()` for all user-visible text
- **Dynamic language switching**: Support runtime language changes
- **Resource organization**: Separate .ts files per language
- **Context**: Provide meaningful context for translators

### Component Structure
- **Modular design**: Break down complex UI into reusable components
- **Property bindings**: Leverage QML's binding system
- **State management**: Use Qt's state machine when appropriate

## Database Conventions
### SQLite Usage
- **Connection management**: Proper connection lifecycle
- **Transaction handling**: Use transactions for multi-operation changes
- **Error checking**: Check return values from all database operations
- **Schema management**: Version control for database schema changes

### Data Access Patterns
- **Separation of concerns**: Database classes separate from UI logic
- **Async operations**: Consider threading for long-running DB operations
- **Data validation**: Validate data before database operations

## Build System Standards
### CMake Organization
- **Version requirement**: CMake 3.31+ minimum
- **Qt integration**: Use Qt6 CMake functions (qt_add_resources, etc.)
- **Automatic processing**: Enable AUTOMOC, AUTOUIC, AUTORCC
- **Custom targets**: Meaningful target names (update_translations, run)

### Dependency Management
- **Conan integration**: Pin specific versions for reproducible builds
- **Platform compatibility**: Consider MinGW-specific workarounds
- **Environment variables**: Support standard Qt environment variables

## Code Organization Principles
### Module Structure
- **Logical separation**: UI, data access, utilities in separate directories
- **Clear interfaces**: Well-defined public APIs between modules
- **Minimal dependencies**: Avoid circular dependencies

### Header/Implementation Split
- **Header files**: Interface declarations, minimal implementation
- **Implementation files**: All implementation details
- **Forward declarations**: Prefer forward declarations to reduce compile time

## Documentation Standards
### Code Documentation
- **Class documentation**: Purpose and usage patterns
- **Method documentation**: Parameters, return values, side effects
- **Complex algorithms**: Explain the approach and reasoning
- **TODO/FIXME**: Use consistent markers for known issues

### Project Documentation
- **README**: Comprehensive setup and usage instructions
- **Architecture**: High-level system design documentation
- **Changelog**: Track significant changes and version history

## Quality Assurance
### Code Review Guidelines
- **Functionality**: Verify code meets requirements
- **Style**: Consistent with established conventions
- **Performance**: Consider efficiency implications
- **Security**: Review for potential vulnerabilities
- **Maintainability**: Code should be readable and modifiable

### Testing Approach (Future Implementation)
- **Unit tests**: Test individual components in isolation
- **Integration tests**: Test component interactions
- **UI tests**: Automated UI interaction testing
- **Manual testing**: Systematic testing of user workflows