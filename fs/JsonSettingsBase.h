//
// Created by Johma on 25/07/22.
//

#ifndef MANAGEMYSELF_JSONSETTINGSBASE_H
#define MANAGEMYSELF_JSONSETTINGSBASE_H

#include <QJsonObject>

class JsonSettingsBase {
public:
    virtual ~JsonSettingsBase() = default;

    [[nodiscard]] virtual QJsonObject toJson()const = 0;

    virtual void loadFromJson(const QJsonObject &json) = 0;

    [[nodiscard]] bool saveToFile(const QString &filePath) const;

    bool loadFromFile(const QString &filePath);

    [[nodiscard]] virtual QString getFilePath() const = 0;

    [[nodiscard]] static int saveToFileAny(const QString &filePath, const QJsonObject &json);

    [[nodiscard]] static QJsonObject loadFromFileAny(const QString &filePath);
};

#endif //MANAGEMYSELF_JSONSETTINGSBASE_H
