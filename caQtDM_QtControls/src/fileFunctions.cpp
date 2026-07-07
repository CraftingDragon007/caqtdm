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
 *  Copyright (c) 2010 - 2014
 *
 *  Author:
 *    Anton Mezger
 *  Contact details:
 *    anton.mezger@psi.ch
 */

#include "fileFunctions.h"
#include "networkaccess.h"
#include "searchfile.h"
#include "specialFunctions.h"

Q_LOGGING_CATEGORY(fileFunctionsLog, "caqtdm.widgets.filefunctions")

fileFunctions::fileFunctions()
{
}

const QString fileFunctions::lastError()
{
    if(errorString.length() > 0) return errorString;
    else return Q_NULLPTR;
}

const QString fileFunctions::lastInfo()
{
    if(infoString.length() > 0) return "Info: " + infoString;
    else return Q_NULLPTR;
}

int fileFunctions::checkFileAndDownload(const QString &fileName, const QString &url)
{
    QString displayPath;
    bool succeeded = false;
    errorString = "";
    infoString = "";

    qCDebug(fileFunctionsLog) << qgetenv("CAQTDM_DISPLAY_PATH");
    qCDebug(fileFunctionsLog) << "checkFileAndDownload: " << fileName;

    searchFile *s = new searchFile(fileName);
    QString fileNameFound = s->findFile();
    if(!fileNameFound.isNull()) {
        qCDebug(fileFunctionsLog) << "checkFileAndDownload file <" << fileName << ">locally found";
        s->deleteLater();
        return true;
    } else {
        qCDebug(fileFunctionsLog) << "checkFileAndDownload file <" << fileName << "> not locally found";
    }
    s->deleteLater();

    // use specified url
    if(url.size() > 0) {
       displayPath = url;
    // otherwise use url from environment variable
    } else {
       displayPath = (QString)  qgetenv("CAQTDM_URL_DISPLAY_PATH");
    }

    if(displayPath.length() < 1) return succeeded;

    qCDebug(fileFunctionsLog) << "filename to download" << fileName;

    displayPath.append("/");
    QString displayPathWithoutFile = displayPath;
    displayPath.append(fileName);
    QUrl displayUrl(displayPath);
    infoString = "download file " + displayPath;

    NetworkAccess *displayGet = new NetworkAccess();
    succeeded |= displayGet->requestUrl(displayUrl, fileName);
    if (!succeeded && !fileName.endsWith(".ui")) {
        QString secondTryFileName = fileName;
        if (secondTryFileName.endsWith(".adl") || secondTryFileName.endsWith(".edl")) {
            secondTryFileName.replace(".adl", ".ui").replace(".edl", ".ui");
        } else {
            secondTryFileName += ".ui";
        }
        displayUrl = QUrl(displayPathWithoutFile + secondTryFileName);

        qCDebug(fileFunctionsLog) << "checkFileAndDownload second try with different extension: " << secondTryFileName;
        displayGet->deleteLater();
        displayGet = new NetworkAccess();
        succeeded |= displayGet->requestUrl(displayUrl, secondTryFileName);
    }

    if(!succeeded) {
        errorString = displayGet->lastError();
    }

    displayGet->deleteLater();
    return succeeded;
}

bool fileFunctions::removeFilesInTree(const QString &dirName)
    {
        QStringList fileFilter;
        bool result = true;
        fileFilter << "ui" << "prc" << "gif" << "jpg" << "png";
        QDir dir(dirName);

        if (dir.exists(dirName)) {
            foreach(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
                if (info.isDir()) {
                    result = removeFilesInTree(info.absoluteFilePath());
                }
                else {
                    QString suffix = info.suffix();
                    if(fileFilter.contains(suffix)) {
                        qCDebug(fileFunctionsLog) << "remove" << info.absoluteFilePath();
                        QFile::remove(info.absoluteFilePath());
                    }
                }

                if (!result) {
                    return result;
                }
            }
        }

        return result;
    }

