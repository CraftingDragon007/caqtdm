#include "weblaunchermanager.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonParseError>
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

QString WebLauncherManager::getRootFile() const {
    return this->m_rootFile;
}

bool WebLauncherManager::setup(const QString fileName) {
    m_rootFile = fileName;
    QWriteLocker locker(&m_visitedFilesLock);
    m_visitedFiles.clear();
    QJsonObject result = loadAndExpand(fileName, true).toObject();
    if (result.isEmpty()) {
        qWarning() << "caQtDM_Web_Launcher -- Something went wrong whilst parsing or file is empty:" << fileName << ", web launcher is now disabled";
        return false;
    }
    m_expandedLauncherJson = result;
    m_isInitialized = true;
    return true;
}

QJsonValue WebLauncherManager::loadAndExpand(QString fileName, bool loadFileChoices) {
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
        QJsonObject obj = doc.object();
        if (loadFileChoices) {
            if (obj.contains("file-choice") && obj["file-choice"].isArray()) {
                QJsonArray fileChoices = obj["file-choice"].toArray();
                QWriteLocker locker(&m_fileChoiceLock);
                foreach (const QJsonValue &choice, fileChoices) {
                    if (choice.isObject()) {
                        QJsonObject choiceObject = choice.toObject();
                        if (choiceObject.contains("text") && choiceObject["text"].isString() && choiceObject.contains("file")
                            && choiceObject["file"].isString()) {

                            QString file = choiceObject["file"].toString();

                            fileFunctions filefunction;
                            filefunction.checkFileAndDownload(file);

                            searchFile *filecheck = new searchFile(file);
                            file = filecheck->findFile();
                            filecheck->deleteLater();

                            if (file.isNull()) {
                                file = getLastElementFromAnywhere(choiceObject["file"].toString());
                                if (!file.isNull()) {
                                    filefunction.checkFileAndDownload(file);

                                    filecheck = new searchFile(file);
                                    file = filecheck->findFile();
                                    filecheck->deleteLater();
                                }

                                if (file.isNull()) {
                                    qWarning() << "caQtDM_Web -- Warning: Launcher file" << choiceObject["file"].toString() << "not found, it won't be available inside the web launcher.";
                                    continue;
                                }
                            }

                            QFileInfo info(file);

                            QString name = info.fileName();
                            qDebug() << "caQtDM_Web -- Adding launcher file" << name;

                            if (m_fileChoices.contains(name)) {
                                qWarning() << "caQtDM_Web -- Warning: Launcher file" << choiceObject["file"].toString() << "was skipped because another file with the same name was already added.";
                                continue;
                            }

                            FileChoice choice = FileChoice::fromJson(choiceObject);
                            choice.fileName = file;

                            m_fileChoices.insert(name, choice);
                        }
                    }
                }

                obj.remove("file-choice");
                QJsonArray newFileChoices;
                for (auto it = m_fileChoices.begin(); it != m_fileChoices.end(); ++it) {
                    QJsonObject value = {{"text", it.value().text}, {"file", it.key()}};
                    newFileChoices.append(value);
                }
                obj.insert("file-choice", newFileChoices);
            }
        } else obj.remove("file-choice");
        return expandObject(obj);
    } else if (doc.isArray()) {
        return expandArray(doc.array());
    }

    return QJsonValue();
}

QJsonValue WebLauncherManager::getLauncherFromUserChoice(QString choice) {
    FileChoice choiceItem;
    if (choice == "root") {
        return m_expandedLauncherJson;
    }

    {
        QReadLocker locker(&m_fileChoiceLock);
        if (choice.isEmpty() || !m_fileChoices.contains(choice)) return QJsonValue();

        choiceItem = m_fileChoices[choice];
    }

    {
        QWriteLocker locker(&m_visitedFilesLock);
        m_visitedFiles.clear();
        return loadAndExpand(choiceItem.fileName, false);
    }
}

QString WebLauncherManager::getLastElementFromAnywhere(QString input) {
    if (input.isEmpty()) return QString();
    QUrl url(input);
    QString pathOnly = url.isValid() ? url.path() : input;

    QFileInfo info(pathOnly);

    return info.fileName().isEmpty() ? info.baseName() : info.fileName();
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
            QJsonValue subContent = loadAndExpand(subPath, false);

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
