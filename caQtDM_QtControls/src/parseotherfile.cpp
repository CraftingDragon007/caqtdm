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

#include <QtGui>
#include <math.h>
#include "parseotherfile.h"

Q_LOGGING_CATEGORY(parseOtherFileLog, "caqtdm.widgets.parseotherfile");

ParseOtherFile::ParseOtherFile(QString fileName, bool &ok, QString &errorString)
{
    QString outFileName;
    QString newFileName = fileName;
    buffer = new QBuffer();
    ok = false;
    fileExists = false;

    const bool isMedmFile = fileName.endsWith (".adl");
    const bool isEdmFile = fileName.endsWith (".edl");

    if(isMedmFile || isEdmFile) {
        qCDebug(parseOtherFileLog) << "caQtDM -- parseotherfile" << fileName << "is a file to convert";

        // file to convert
        QFile* aFile = new QFile(fileName);

        if(!aFile->exists() ) {
            qCDebug(parseOtherFileLog) << "caQtDM -- parseotherfile" << fileName << "does not exist, sorry";
            errorString = tr("sorry -- specified file %1 is not found").arg(fileName);
        } else {
            // file to convert to locally
            qCDebug(parseOtherFileLog) << "caQtDM -- parseotherfile" << fileName << "file to convert exists";
            QFileInfo fileInfo(aFile->fileName());

            outFileName=QDir::tempPath()+QDir::separator()+fileInfo.fileName();
            outFileName.chop (3);
            outFileName.append ("ui");
            outFileName=QDir::fromNativeSeparators(outFileName);

            QFile* newFile = new QFile(outFileName);
            if(newFile->exists() ) {
                qCDebug(parseOtherFileLog) << "caQtDM -- parseotherfile uifile" << outFileName << "exists, no parsing done & file will be used";
                errorString = tr("file %1 already exists, no parsing done and use existing file").arg(outFileName);
                ok = true;
                fileExists= true;
            } else {
                qCDebug(parseOtherFileLog) << "caQtDM -- parseotherfile uifile" << outFileName << "will be written";
                if( isMedmFile ){
                    // medm conversion
                    myParser converter;
                    converter.setTmp_directory(QDir::tempPath());
                    converter.adl2ui(fileName);
                }
#ifndef _MSC_VER
                else if ( isEdmFile ){
                    // edm conversion
                    myParserEDM converter;
                    converter.edl2ui(fileName);
                }
#endif
                ok = true;
            }
            delete newFile;
        }
        delete aFile;
    } else {
        qCDebug(parseOtherFileLog) << "caQtDM -- parseotherfile" << fileName << "is not a medm or edm file";
        errorString = tr("sorry -- specified file %1 is not a med or edm file").arg(fileName);
    }

    if(ok) {
        // open the converted file, that was generated locally, or file already present
        QFile f(outFileName);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ok = false;
            return;
        }

        // fill buffer with the filedata
        buffer->open(QIODevice::ReadWrite);
        buffer->seek(0);
        buffer->write(f.readAll());
        buffer->close();
        f.close();

        // get rid of file;
        if(!fileExists) {
            if ( QFile::exists(outFileName)) {
                QFile::remove(outFileName);
            }
        }
    }
    fflush(stdout);
}

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
QWidget* ParseOtherFile::load(QWidget *parent)
{
    QWidget *widget;
    QUiLoader loader;
    buffer->open(QIODevice::ReadOnly);
    buffer->seek(0);
    //QString str=buffer->buffer();
    //qCDebug(parseOtherFileLog) << str;
    widget=loader.load(buffer, parent);
    buffer->close();
    return widget;
}




