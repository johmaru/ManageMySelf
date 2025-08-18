# Done Checklist (after completing a task)

- Build
  - cmake --build . が成功するか
- Run smoke test
  - cmake --build . --target run が起動し、Main.qml が表示されるか
- DB migrations
  - 初回起動時に Documents/ManageMySelf/ManageMySelf.db が作成され、recent_files/workspaces テーブルがあるか
- QML resource
  - qml.qrc に Main.qml 等が含まれており、実行時に QML がロードされるか
- Settings persistence
  - Documents/ManageMySelf/settings.json が生成/更新されるか (language/theme/windowSize)
- Translations
  - i18n/ *.ts の更新と .qm 生成 (update_translations ターゲット)
- Packaging (Windows)
  - POST_BUILD の windeployqt が成功して DLL 類が配置されるか
