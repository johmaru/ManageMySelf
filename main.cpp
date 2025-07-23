#include <QStandardPaths>
#include <Qstring>
#include <QDir>
#include <QFile>
#include <QApplication>
#include <QWidget>
#include <QJsonDocument>
#include "global_settings.h"

int SettingsInit(const QString &filePath);

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    GlobalSettings settings;

    const QString filePath = settings.getFilePath();
    if (filePath.isEmpty()) {
        qWarning() << "Failed to get the file path";
        return -1;
    }

    if (SettingsInit(filePath) != 0) {
        return -1;
    }


    settings.loadFromFile(filePath);

    QWidget window;
    window.resize(settings.windowSize);
    window.setWindowTitle("Manage my self");
    window.show();

    return QApplication::exec();
}

int SettingsInit(const QString &filePath) {

    QFile settingsFile(filePath);
    if (!settingsFile.exists()) {
        qInfo() << "Can't find the setting file, which will be created now.";
        if (!settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to open the file: " << settingsFile.errorString();
            return -1;
        }

        GlobalSettings settings;
        settings.language = "ja";
        settings.windowSize = QSize(800,600);

        if (!settings.saveToFile(filePath)) {
            qWarning() << "Failed to save to the file";
            return -1;
        }
    }
    return 0;
}