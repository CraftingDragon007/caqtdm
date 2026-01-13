#include "weblaunchermanager.h"

#include <QFile>
#include <QJsonObject>
#include <fileFunctions.h>
#include <searchfile.h>

WebLauncherManager::WebLauncherManager(QObject *parent) : QObject(parent), m_isInitialized(false)
{}

WebLauncherManager& WebLauncherManager::instance() {
    static WebLauncherManager manager;
    return manager;
}

WebLauncherManager::~WebLauncherManager() {}

bool WebLauncherManager::isInitialized() const {
    return this->m_isInitialized;
}

QJsonValue WebLauncherManager::getExpandedLauncherJson() const {
    return this->m_expandedLauncherJson;
}

bool WebLauncherManager::setup(const QString fileName) {
    m_visitedFiles.clear();
    QJsonObject result = loadAndExpand(fileName).toObject();
    if (result.isEmpty()) {
        qWarning() << "caQtDM_Web_Launcher -- Something went wrong whilst parsing or file is empty:" << fileName << ", web launcher is now disabled";
        return false;
    }
    m_expandedLauncherJson = result;
    return true;
}

QJsonValue WebLauncherManager::loadAndExpand(QString fileName) {
    fileFunctions filefunction;
    filefunction.checkFileAndDownload(fileName);

    searchFile *filecheck = new searchFile(fileName);
    fileName = filecheck->findFile();
    filecheck->deleteLater();

    if (m_visitedFiles.contains(fileName)) {
        qWarning() << "caQtDM_Web_Launcher -- Circular dependent included launcher file" << fileName << ", skipping it";
        return QJsonValue();
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "caQtDM_Web_Launcher -- File could not be opened:" << fileName << "Error:" << file.errorString();
        return QJsonValue();
    }

    m_visitedFiles.insert(fileName);

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON Parse Error in" << fileName << ":" << error.errorString();
    }

    if (doc.isObject()) {
        return expandObject(doc.object());
    } else if (doc.isArray()) {
        return expandArray(doc.array());
    }

    return QJsonValue();
}

QJsonValue WebLauncherManager::expandObject(QJsonObject obj) {
    if (obj.contains("menu") && obj["menu"].isArray()) {
        obj["menu"] = expandArray(obj["menu"].toArray());
    }
    return obj;
}

QJsonArray WebLauncherManager::expandArray(const QJsonArray &arr) {
    QJsonArray result;
    for (const QJsonValue &val : arr) {
        if (!val.isObject()) {
            result.append(val);
            continue;
        }

        QJsonObject item = val.toObject();
        if (item["type"].toString() == "menu" && item.contains("file")) {
            QString subPath = item["file"].toString();
            QJsonValue subContent = loadAndExpand(subPath);

            if (!subContent.isNull() && subContent.isObject()) {
                QJsonObject subObj = subContent.toObject();
                if (subObj.contains("menu") && subObj["menu"].isArray()) {
                    item["menu"] = subObj["menu"].toArray();
                    item.remove("file");
                }
            }
        }
        result.append(item);
    }
    return result;
}
