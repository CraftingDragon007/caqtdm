/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2026
 *
 *  Author:
 *    Julian Houba
 *  Contact details:
 *    julian.houba@psi.ch
 */

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

    bool setup(const QString& fileName);
    bool isInitialized() const;
    QJsonValue getExpandedLauncherJson() const;
    QJsonValue getLauncherFromUserChoice(const QString& choice);
    QString getRootFile() const;

private:
    explicit WebLauncherManager(QObject *parent = nullptr);
    ~WebLauncherManager();

    bool m_isInitialized;
    QString m_rootFile;

    QJsonValue m_expandedLauncherJson;

    QMap<QString, FileChoice> m_fileChoices;

    QSet<QString> m_visitedFiles;
    QJsonValue loadAndExpand(const QString& fileName, bool loadFileChoices);
    QString resolveFilePath(const QString& inputPath);
    void processFileChoices(QJsonObject& obj);
    QJsonDocument parseJsonFile(const QString& fileName);
    QJsonValue expandObject(QJsonObject obj);
    QJsonArray expandArray(const QJsonArray &arr);
    void normalizePanelPaths(QJsonObject& obj);
    void normalizePanelPath(QJsonObject& obj, const QString& key);

    QString getLastElementFromAnywhere(QString input);

    QReadWriteLock m_fileChoiceLock;
    QReadWriteLock m_visitedFilesLock;

    WebLauncherManager(const WebLauncherManager&) = delete;
    WebLauncherManager& operator=(const WebLauncherManager&) = delete;

signals:
};

#endif // WEBLAUNCHERMANAGER_H
