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

bool WebLauncherManager::setup(const QString& fileName) {
    m_rootFile = fileName;

    QWriteLocker locker(&m_visitedFilesLock);
    m_visitedFiles.clear();

    QJsonValue result = loadAndExpand(fileName, true);
    if (result.isNull() || result.toObject().isEmpty()) {
        qWarning() << "caQtDM_Web_Launcher -- Error parsing or empty file:" << fileName;
        return false;
    }

    m_expandedLauncherJson = result.toObject();
    m_isInitialized = true;
    return true;
}

QJsonValue WebLauncherManager::loadAndExpand(const QString& fileName, bool loadFileChoices) {
    QString resolvedPath = resolveFilePath(fileName);

    if (resolvedPath.isEmpty()) {
        qWarning() << "caQtDM_Web_Launcher -- [File Error] Could not find or resolve path for:" << fileName;
        return QJsonValue();
    }

    if (m_visitedFiles.contains(resolvedPath)) {
        qWarning() << "caQtDM_Web_Launcher -- [Recursion Error] Circular dependency blocked for:" << resolvedPath;
        return QJsonValue();
    }

    qDebug() << "caQtDM_Web_Launcher -- [Info] Expanding file:" << resolvedPath;

    m_visitedFiles.insert(resolvedPath);

    QJsonDocument doc = parseJsonFile(resolvedPath);
    if (doc.isNull()) {
        qWarning() << "caQtDM_Web_Launcher -- [JSON Error] Failed to parse or empty file:" << resolvedPath;
        m_visitedFiles.remove(resolvedPath);
        return QJsonValue();
    }

    QJsonValue result;
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (loadFileChoices && obj.contains("file-choice")) {
            processFileChoices(obj);
        }
        result = expandObject(obj);
    } else if (doc.isArray()) {
        result = expandArray(doc.array());
    }

    // Important: remove from visited list after processing children
    // to allow other branches to use this file if needed.
    m_visitedFiles.remove(resolvedPath);

    return result;
}

QString WebLauncherManager::resolveFilePath(const QString& inputPath) {
    fileFunctions fileFunc;
    fileFunc.checkFileAndDownload(inputPath);

    searchFile searcher(inputPath);
    QString foundPath = searcher.findFile();

    if (foundPath.isEmpty()) {
        QString fallback = getLastElementFromAnywhere(inputPath);
        if (!fallback.isEmpty()) {
            fileFunc.checkFileAndDownload(fallback);
            searchFile fallbackSearcher(fallback);
            foundPath = fallbackSearcher.findFile();
        }
    }
    return foundPath;
}

QJsonDocument WebLauncherManager::parseJsonFile(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "caQtDM_Web_Launcher -- [IO Error] Cannot open file for reading:" << fileName;
        return QJsonDocument();
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "caQtDM_Web_Launcher -- [Syntax Error] in" << fileName << ":" << error.errorString() << "at offset" << error.offset;
    }
    return doc;
}

void WebLauncherManager::processFileChoices(QJsonObject& obj) {
    QJsonArray fileChoicesArr = obj["file-choice"].toArray();
    QWriteLocker locker(&m_fileChoiceLock);

    for (const QJsonValue &value : std::as_const(fileChoicesArr)) {
        QJsonObject choiceObj = value.toObject();
        QString rawPath = choiceObj["file"].toString();

        if (rawPath.isEmpty()) continue;

        QString resolvedPath = resolveFilePath(rawPath);
        if (resolvedPath.isEmpty()) {
            qWarning() << "caQtDM_Web_Launcher -- Launcher file not found:" << rawPath;
            continue;
        }

        QString baseName = QFileInfo(resolvedPath).fileName();
        if (m_fileChoices.contains(baseName)) continue;

        FileChoice choice = FileChoice::fromJson(choiceObj);
        choice.fileName = resolvedPath;
        m_fileChoices.insert(baseName, choice);
        qDebug() << "caQtDM_Web -- Added launcher file:" << baseName;
    }

    // Update the JSON object to reflect the internal map
    obj.remove("file-choice");
    QJsonArray newChoices;
    for (auto it = m_fileChoices.begin(); it != m_fileChoices.end(); ++it) {
        newChoices.append(QJsonObject{{"text", it.value().text}, {"file", it.key()}});
    }
    obj.insert("file-choice", newChoices);
}

QJsonValue WebLauncherManager::getLauncherFromUserChoice(const QString& choice) {
    FileChoice choiceItem;
    if (choice == "root") {
        return m_expandedLauncherJson;
    }

    {
        QReadLocker locker(&m_fileChoiceLock);
        if (choice.isEmpty() || !m_fileChoices.contains(choice)) return QJsonValue();

        choiceItem = m_fileChoices.value(choice);
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
    // If this object has a menu array, expand the items within that array
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

        // Check if this item is a sub-menu reference
        if (item["type"].toString() == "menu" && item.contains("file")) {
            QString subPath = item["file"].toString();

            qDebug().noquote().nospace() << "caQtDM_Web_Launcher -- [Info] Item '" << item["text"].toString() << "' pulls from: " << subPath;

            QJsonValue subContent = loadAndExpand(subPath, false);

            if (!subContent.isNull() && subContent.isObject()) {
                QJsonObject subObj = subContent.toObject();

                if (subObj.contains("menu") && subObj["menu"].isArray()) {
                    item["menu"] = subObj["menu"].toArray();
                    item.remove("file");
                    qDebug() << "caQtDM_Web_Launcher -- [Info] Successfully merged" << subPath << "into menu.";
                } else {
                    qWarning() << "caQtDM_Web_Launcher -- [Structure Error] File" << subPath << "loaded but contains no 'menu' array.";
                }
            } else {
                qWarning().noquote().nospace() << "caQtDM_Web_Launcher -- [Expansion Failed] Sub-menu item '" << item["text"].toString() << "' could not be populated from: " << subPath;
            }
        }
        result.append(item);
    }
    return result;
}