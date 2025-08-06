//
// Created by Johma on 25/07/22.
//

#ifndef MANAGEMYSELF_SQL_H
#define MANAGEMYSELF_SQL_H

#include <QString>
#include <QDebug>
#include <qdatetime.h>

class SqlOS {
public:
    SqlOS();
    ~SqlOS();

    enum class OS {
        Windows,
        Linux,
        macOS,
        Unknown
    };

    enum class DateTimeFormat {
        YYYY_MM_DD,
        DD_MM_YYYY,
        MM_DD_YYYY,
        YYYY_MM_DD_HH_MM_SS,
    };

    [[nodiscard]] QString getCurrentDateTimeString(DateTimeFormat format = DateTimeFormat::YYYY_MM_DD) const {
        QDateTime currentDateTime = QDateTime::currentDateTime();
        switch (format) {
            case DateTimeFormat::YYYY_MM_DD:
                return currentDateTime.toString("yyyy-MM-dd");
            case DateTimeFormat::DD_MM_YYYY:
                return currentDateTime.toString("dd-MM-yyyy");
            case DateTimeFormat::MM_DD_YYYY:
                return currentDateTime.toString("MM-dd-yyyy");
            case DateTimeFormat::YYYY_MM_DD_HH_MM_SS:
                return currentDateTime.toString("yyyy-MM-dd HH:mm:ss");
            default:
                return currentDateTime.toString();
        }
    }

    [[nodiscard]] OS getCurrentOS() const {
        if (m_currentOs == "Windows") {
            return OS::Windows;
        } else if (m_currentOs == "Linux") {
            return OS::Linux;
        } else if (m_currentOs == "macOS") {
            return OS::macOS;
        } else {
            return OS::Unknown;
        }
    }

    [[nodiscard]] QString getCurrentOSName() const {
        return m_currentOs;
    }

private:
    QString m_currentOs =
    #if defined (_WIN32)
        "Windows";
    #elif defined (__linux__)
        "Linux";
    #elif defined (__APPLE__) || defined (__MACH__)
        "macOS";
    #else
        "Unknown";
    #endif
};

#endif // MANAGEMYSELF_SQL_H