# Improvement Ideas

- Hardcoded paths
  - main.cpp の engine.addImportPath("C:/Qt/6.9.1/mingw_64/qml") を CMAKE_PREFIX_PATH 由来に置換 (環境変数や QLibraryInfo を利用)。
- Robust DB directory creation
  - SqLiteBase::checkMainDatabaseAndCreate で Documents/ManageMySelf が無い場合に mkdir する (現状 -2 を返すのみ)。
- Error code enum
  - 明確な enum class でエラーコードを統一、QML 側にマッピング用関数を提供。
- Tests
  - 最低限のユニットテスト (e.g., getFilePath, createWorkspace, diary CRUD)。
- CI
  - GitHub Actions で CMake + Conan キャッシュ付きビルド。
- Code style
  - .clang-format 追加。
- i18n
  - ts 更新用スクリプトや翻訳キーの整合チェック。
- QML structure
  - 型安全な QML モジュール化 (qmldir は既にあり)。
- Settings schema
  - GlobalSettings の loadFromJson でのデフォルト補完は済、スキーマバージョン付与を検討。
