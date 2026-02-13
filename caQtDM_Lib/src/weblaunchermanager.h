#ifndef WEBLAUNCHERMANAGER_H
#define WEBLAUNCHERMANAGER_H

#include <QObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QReadWriteLock>
#include "caQtDM_Lib_global.h"



struct FileChoice {
    QString text;
    QString fileName;

    static FileChoice fromJson(const QJsonObject &obj) {
        return { obj["text"].toString(), obj["file"].toString() };
    }

    QJsonObject toJson() const { return {{"text", text}, {"file", fileName}};  }
};

class CAQTDM_LIBSHARED_EXPORT WebLauncherManager : public QObject
{
    Q_OBJECT
public:
    static WebLauncherManager& instance();

    bool setup(const QString fileName);
    bool isInitialized() const;
    QJsonValue getExpandedLauncherJson() const;
    QJsonValue getLauncherFromUserChoice(QString choice);
    QString getRootFile() const;

private:
    explicit WebLauncherManager(QObject *parent = nullptr);
    ~WebLauncherManager();

    bool m_isInitialized;
    QString m_rootFile;

    QJsonValue m_expandedLauncherJson;

    QMap<QString, FileChoice> m_fileChoices;

    QSet<QString> m_visitedFiles;
    QJsonValue loadAndExpand(QString fileName, bool loadFileChoices);
    QString resolveFilePath(const QString& inputPath);
    void processFileChoices(QJsonObject& obj);
    QJsonDocument parseJsonFile(const QString& fileName);
    QJsonValue expandObject(QJsonObject obj);
    QJsonArray expandArray(const QJsonArray &arr);

    QString getLastElementFromAnywhere(QString input);

    QReadWriteLock m_fileChoiceLock;
    QReadWriteLock m_visitedFilesLock;

    WebLauncherManager(const WebLauncherManager&) = delete;
    WebLauncherManager& operator=(const WebLauncherManager&) = delete;

signals:
};

#endif // WEBLAUNCHERMANAGER_H
