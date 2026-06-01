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

#if defined(_MSC_VER)
   #define _MATH_DEFINES_DEFINED
   #define NOMINMAX
   #include <windows.h>
   #define strdup _strdup
#endif

#include "searchfile.h"

#include "fileopenwindow.h"
#include "fileFunctions.h"
#include "QDebug"
#include <QFileDialog>
#include <QLocale>
#include <signal.h>
#include <iostream>
#include <stdlib.h>
#include "pipereader.h"
#include "loggingcategories.h"

#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
#include <QApplication>

#ifndef CAQTDM_NO_CUSTOM_LOGHANDLER
#include <logging/generalloghandler.h>
#endif

#else
#include <QtGui/QApplication>
#endif

#if defined(linux) || defined(__FreeBSD__)
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
   #define CAQTDM_X11 Q_WS_X11
#else
   #if QT_VERSION < QT_VERSION_CHECK(6,0,0)
       #ifndef MOBILE_ANDROID
          #define CAQTDM_X11 Q_OS_UNIX
       #endif
   #endif
#endif
#endif

#ifdef CAQTDM_X11
        #include <QX11Info>
        #include <X11/Xutil.h>
        #include <X11/Xlib.h>
        #include <X11/Xatom.h>
#endif //CAQTDM_X11

#ifdef MOBILE
#include <QStyleFactory>
#endif

Q_LOGGING_CATEGORY(caQtDMLog, "caqtdm.viewer.caqtdm")

static void unixSignalHandler(int signum) {

    Q_UNUSED(signum);

    /*
     * Make sure your Qt application gracefully quits.
     * NOTE - purpose for calling qApp->exit(0):
     *      1. Forces the Qt framework's "main event loop `qApp->exec()`" to quit looping.
     *      2. Also emits the QCoreApplication::aboutToQuit() signal. This signal is used for cleanup code.
     */
    QCoreApplication::exit(0);
}

extern bool HTTPCONFIGURATOR;

static void createMap(QMap<QString, QString> &map, const QString& option)
{
    qCDebug(caQtDMLog) << "treat option" << option;
    // option of type KEY1=VALUE1,KEY2=VALUE2,KEY3=VALUE3
    if(option != Q_NULLPTR) {
        QStringList vars = option.split(",", SKIP_EMPTY_PARTS);
        for(int i=0; i< vars.count(); i++) {
            int pos = vars.at(i).indexOf("=");
            if(pos != -1) {
                QString key = vars.at(i).mid(0, pos);
                QString value = vars.at(i).mid(pos+1);
                map.insert(key.trimmed(), value);
            } else {
                qCDebug(caQtDMLog) <<"option" <<  option << "could not be parsed";
            }
        }
    }
    qCDebug(caQtDMLog) << "inserted int map from option:" << option;
    qCDebug(caQtDMLog) << "resulting map=" << map;
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(caQtDM);
#ifdef MOBILE
    Q_INIT_RESOURCE(qtcontrols);
#endif

    // we do not want numbers with a group separators
    QLocale loc = QLocale::system();
    loc.setNumberOptions(QLocale::OmitGroupSeparator);
    loc.setDefault(loc);

    QString theme = "";
    int styleIndex = -1;
    QString fileNameStylesheet = "";
    QString fileName = "";
    QString macroString = "";
    QString geometry = "";
    QString macroFile = "";
    QMap<QString, QString> options;
    options.clear();

#if defined(_MSC_VER)
    if (AttachConsole(ATTACH_PARENT_PROCESS)){
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),ENABLE_QUICK_EDIT_MODE| ENABLE_EXTENDED_FLAGS);
        freopen("CON", "w", stdout);
        freopen("CON", "w", stderr);
        freopen("CON", "r", stdin);
    }
#endif

    int	in, numargs;
    bool attach = false;
    bool minimize= false;
    bool printscreen = false;
    bool savetoimage = false;
    bool resizing = true;

    // Here follow some printfs, they should not be replaced by qInfo() because the custom logger is not initialized yet and printf is more consistent across plattforms than regular qInfo() with default logger.
    QStringList arguments;
    arguments.reserve(argc);
    for (numargs = argc, in = 1; in < numargs; in++) {
        arguments.append(argv[in]);
        if ( strcmp (argv[in], "-display" ) == 0 ) {
            in++;
            printf("caQtDM -- display <%s>\n", argv[in]);
        } else if ( strcmp (argv[in], "-macro" ) == 0 ) {
            in++;
            printf("caQtDM -- macro <%s>\n", argv[in]);
            macroString = QString(argv[in]);
        } else if ( strcmp (argv[in], "-attach" ) == 0 ) {
            printf("caQtDM -- will attach to another caQtDM instance if running\n");
            attach = true;
        } else if ( strcmp (argv[in], "-noMsg" ) == 0 ) {
            printf("caQtDM -- will minimize its main windows\n");
            minimize = true;
        } else if( strcmp (argv[in], "-macrodefs" ) == 0) {
            in++;
            printf("caQtDM -- will load macro string from file <%s>\n", argv[in]);
            macroFile = QString(argv[in]);
        } else if ( strcmp (argv[in], "-style" ) == 0 ) {
            styleIndex = in;
            in++;
            printf("caQtDM -- using qt theme <%s>\n", argv[in]);
            theme = QString(argv[in]);
        } else if ( strcmp (argv[in], "-stylefile" ) == 0 ) {
            in++;
            printf("caQtDM -- will replace the default stylesheet with stylesheet <%s>\n", argv[in]);
            fileNameStylesheet = QString(argv[in]);
        } else if ( strcmp (argv[in], "-x" ) == 0 ) {

        } else if ( strcmp (argv[in], "-displayFont" ) == 0 ) {
             in++;
        } else if(!strcmp(argv[in],"-help") || !strcmp(argv[in],"-h") || !strcmp(argv[in],"-?")) {
             in++;
                 printf("Usage:\n"
                   "  caQtDM[X options]\n"
                   "  [-help | -h | -?] describe the options\n"
                   "  [-x] has no effect (MEDM’s execute-only mode)\n"
                   "  [-attach] attach to a running caQtDM instance\n"
                   "  [-noMsg] iconize the main window\n"
                   "  [-stylefile filename] will replace the default stylesheet with the specified file (works only when not attaching)\n"
                   "  [-macro \"xxx=aaa,yyy=bbb, ...\"] apply macro substitution to replace occurrences of $(xxx) with value aaa\n"
                   "  [-macrodefs filename] will load macro definitions from file\n"
                   "  [-dg [<width>x<height>][+<xoffset>-<yoffset>] specifies the geometry (location and size) of the synoptic display\n"
                   "  [-httpconfig] will display a network configuration screen at startup\n"
                   "  [-print] will print file and exit\n"
                   "  [-savetoimage] will save image file and exit\n"
                   "  [-noResize] will prevent resizing\n"
                   "  [-cs defaultcontrolsystempluginname] will override the default epics3 datasource\n"
                   "  [-option \"xxx=aaa,yyy=bbb, ...\"] various options,\n"
                   "  \t e.g. -option \"updatetype=direct\" will set the updatetype to Direct\n"
                   "  \t options for bsread:\n "
                   "  \t\t bsmodulo,bsoffset,\n"
                   "  \t\t bsinconsistency(drop|keep-as-is|adjust-individual|adjust-global),\n"
                   "  \t\t bsmapping(provide-as-is|drop|fill-null)\n"
                   "  \t\t bsstrategy(complete-all|complete-latest)\n"
                   "  [-url url] will look for files on the specified url and download them to a local directory\n"
                   "  [-emptycache] will empty the local cache used for downloading"
                   "  [file] UI file to open\n"
                   "  [&]\n"
                   "\n"
                   "  -x -displayFont -display are ignored !\n\n"
                   "  on linux platforms, ui data can be piped to caQtDM, but then -httpconfig & -attach will not work\n\n");
                 exit(1);
        } else if((!strcmp(argv[in],"-displayGeometry")) || (!strcmp(argv[in],"-dg"))) {
            // [-dg [xpos[xypos]][+xoffset[+yoffset]]
             in++;
             geometry = QString(argv[in]);
        } else if(!strcmp(argv[in], "-print")) {
             printscreen = true;
             minimize = true;
             resizing = false;
        } else if(!strcmp(argv[in], "-savetoimage")) {
            savetoimage = true;
            minimize = true;
            resizing = false;
        } else if(!strcmp(argv[in], "-noResize")) {
            resizing = false;
        } else if(!strcmp(argv[in], "-httpconfig")) {
            HTTPCONFIGURATOR = true;
        } else if(!strcmp(argv[in], "-url")) {
            in++;
            setenv("CAQTDM_URL_DISPLAY_PATH", argv[in], 1);
        } else if(!strcmp(argv[in], "-emptycache")) {
            Specials specials;
            QString path =  specials.getStdPath();
            fileFunctions filefunction;
            path.append("/");
            filefunction.removeFilesInTree(path);
            printf("caQtDM -- cache @ %s with ui & graphic files has been emptied\n", qasc(path));
        } else if(!strcmp(argv[in], "-cs")) {
            in++;
            options.insert("defaultPlugin", QString(argv[in]));
        } else if ( strcmp (argv[in], "-option" ) == 0 ) {
            in++;
            printf("caQtDM -- option <%s>\n", argv[in]);
            createMap(options, QString(argv[in]));
        } else if (strncmp (argv[in], "-" , 1) == 0) {
            /* unknown application argument */
            printf("caQtDM -- Argument %d = [%s] is unknown!, possible -attach -macro -noMsg -stylefile -dg -x -print -httpconfig -noResize -option\n",in,argv[in]);
        } else {
            printf("caQtDM -- file = <%s>\n", argv[in]);
            fileName = QString(argv[in]);
            break;
        }
    }

    // prevents QApplication from handling style on it's own
    if (!theme.isEmpty() && styleIndex > -1) {
        if (styleIndex < argc - 1) {
            for (int j = styleIndex; j < argc - 2; ++j) {
                argv[j] = argv[j + 2];
            }
            argc -= 2;
        } else if (styleIndex == argc - 1) {
            delete argv[styleIndex];
            --argc;
        }
    }

#if defined(_MSC_VER)
#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
    // to avoid an error output: "Qt WebEngine seems to be initialized from a plugin"
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif
#endif

    QApplication app(argc, argv);
    QApplication::setOrganizationName("Paul Scherrer Institut");
    QApplication::setApplicationName("caQtDM");

#ifndef CAQTDM_NO_CUSTOM_LOGHANDLER
    // From hereon, everything logged via qDebug or its siblings will be captured by the custom LogHandler.
    GeneralLogHandler::initialize();
    qCInfo(caQtDMLog) << "caQtDM -- initialized logger";
    // Log all arguments the application was started with (before they were processed)
    for (int i = 0; i < arguments.size(); i++) {
        qCDebug(caQtDMLog).nospace() << "Argument: " << i << ": " << arguments[i];
    }
#endif

#ifdef MOBILE_ANDROID
    qCDebug(caQtDMLog) << QStyleFactory::keys();
    app.setStyle(QStyleFactory::create("Fusion"));
#endif

    if (!theme.isEmpty()) {
        QStringList availableThemes = QStyleFactory::keys();

        if (availableThemes.contains(theme, Qt::CaseInsensitive)) {
            QApplication::setStyle(QStyleFactory::create(theme));
        } else {
            qCWarning(caQtDMLog) << "caQtDM -- Invalid theme" << theme << "specified, falling back to default system theme";
        }
    }

    searchFile *searchDefaultStyleSheet = new searchFile("caQtDM_stylesheet.qss");
    QString fileNameFound = searchDefaultStyleSheet->findFile();
    if(fileNameFound.isNull()) {
        qCInfo(caQtDMLog) << QString("caQtDM -- file <caQtDM_stylesheet.qss> could not be loaded, is 'CAQTDM_DISPLAY_PATH' <%1> defined?").arg(searchDefaultStyleSheet->displayPath());
    } else {
        QFile file(fileNameFound);
        file.open(QFile::ReadOnly);
        QString StyleSheet = QLatin1String(file.readAll());
        qCInfo(caQtDMLog) << "caQtDM -- file <caQtDM_stylesheet.qss> loaded as the default application stylesheet";
        app.setStyleSheet(StyleSheet);
        file.close();
    }
    delete searchDefaultStyleSheet;

    // get data from pipe if any (ui data can be piped to this application, for linux at this time)
    // only when no file is given, attaching is not allowed, in order to get rid of the temporary file when exit
#if defined linux || defined TARGET_OS_MAC
    if(fileName.length() <= 0) {

        QEventLoop *loop = new QEventLoop();
        PipeReader *reader = new PipeReader(loop);
        loop->exec();

        QString newFilename = reader->getTemporaryFilename();
        if(newFilename.size() > 0) {
            fileName = newFilename;
            attach = false;
            HTTPCONFIGURATOR = false;
        }
        delete reader;
        delete loop;
        qCDebug(caQtDMLog) << "use now file" << fileName;
    }
#endif

    // must be always true for mobile platforms
#ifdef MOBILE
     HTTPCONFIGURATOR = true;
#endif

     if(fileNameStylesheet.length() > 0) {
        searchFile *searchCustomStyleSheet = new searchFile(fileNameStylesheet);
        fileNameFound = searchCustomStyleSheet->findFile();
        if(fileNameFound.isNull()) {
            qCInfo(caQtDMLog) << QString("caQtDM -- custom stylesheet file <%1> could not be loaded, is 'CAQTDM_DISPLAY_PATH' <%2> defined?").arg(fileNameStylesheet).arg(searchCustomStyleSheet->displayPath());
        } else {
            QFile file(fileNameFound);
            file.open(QFile::ReadOnly);
            QString StyleSheet = QLatin1String(file.readAll());
            qCInfo(caQtDMLog) << QString("caQtDM -- custom stylesheet file <%1> replaced the default stylesheet").arg(fileNameStylesheet);
            fflush(stdout);
            app.setStyleSheet(StyleSheet);
            qApp->setProperty("user_defined_stylesheet", fileNameStylesheet);
            file.close();
        }
        delete searchCustomStyleSheet;
    }

    // load macro definitions from file (located in this directory or in the caQTDM_DISPLAY_PATH)
    if(macroFile.length() > 0) {
        searchFile *searchMacroFile = new searchFile(macroFile);
        fileNameFound = searchMacroFile->findFile();
        if(fileNameFound.isNull()) {
            qCInfo(caQtDMLog) << QString("caQtDM -- custom macro file <%1> could not be loaded, is 'CAQTDM_DISPLAY_PATH' <%2> defined?").arg(macroFile).arg(searchMacroFile->displayPath());
        } else {
            QFile file(fileNameFound);
            file.open(QFile::ReadOnly);
            qCInfo(caQtDMLog) << QString("caQtDM -- macro definitions were read from custom macro file <%1>").arg(macroFile);
            macroString = QLatin1String(file.readAll());
            macroString = macroString.simplified().trimmed();
            file.close();
        }
        delete searchMacroFile;
    }

#ifdef IO_OPTIMIZED_FOR_TABWIDGETS
    qCInfo(caQtDMLog) << "caQtDM -- viewer will disable monitors for hidden pages of QTabWidgets, in "
                      "case of problems\n          you may disable this by not defining "
                      "IO_OPTIMIZED_FOR_TABWIDGETS in qtdefs.pri";
#else
    qCInfo(caQtDMLog)
        << "caQtDM -- viewer will not disable monitors for hidden pages of QTabWidgets\n          "
           "you may enable this by defining IO_OPTIMIZED_FOR_TABWIDGETS in qtdefs.pri";
#endif

#ifndef CONFIGURATOR
    QString displayPath = (QString)  qgetenv("CAQTDM_URL_DISPLAY_PATH");
    if(displayPath.length() > 0) {
        qCInfo(caQtDMLog) << QString("caQtDM -- files will be downloaded from <%1> when not locally found").arg(displayPath);
    } else {
        qCInfo(caQtDMLog) << "caQtDM -- files will not be downloaded from an url when not locally found, while CAQTDM_URL_DISPLAY_PATH is not defined";
    }
#endif

    FileOpenWindow fileOpenWindow (0, fileName, macroString, attach, minimize, geometry, printscreen, resizing, options);
    fileOpenWindow.setWindowIcon (QIcon(":/caQtDM.ico"));
    if (savetoimage) fileOpenWindow.setProperty("savetoimage", true);
    fileOpenWindow.show();
#ifdef CAQTDM_X11
    #if QT_VERSION > QT_VERSION_CHECK(5,0,0)
        if (qApp->platformName()== QLatin1String("xcb")){
    #endif
    #if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QString X_Server_Check=ServerVendor(QX11Info::display());
    #else
        QString X_Server_Check=ServerVendor(QNativeInterface::QX11Application::display());
    #endif

    if (X_Server_Check.contains("Colin Harrison")){ //Xming Server on Windows, yes this is a quickfix!
       fileOpenWindow.move(10,30);// 0,0 is outside the visible areas on the taget
    }else{
       fileOpenWindow.move(0,0);
    }
    #if QT_VERSION > QT_VERSION_CHECK(5,0,0)
        }else{
            fileOpenWindow.move(0,0);
        }
    #endif
#else
    fileOpenWindow.move(0,0);
#endif


    if (signal(SIGINT, unixSignalHandler) == SIG_ERR) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
            qCFatal(caQtDMLog) << "An error occurred while setting a signal handler";
#else
	    qFatal("An error occurred while setting a signal handler");
#endif
    }
    if (signal(SIGTERM, unixSignalHandler) == SIG_ERR) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        qCFatal(caQtDMLog) << "An error occurred while setting a signal handler";
#else
	    qFatal("An error occurred while setting a signal handler");
#endif
    }

    QObject::connect(&app, SIGNAL(aboutToQuit()), &fileOpenWindow, SLOT(doSomething()));

    int exitCode = 0;

    // Put this into a try catch statement to catch all errors
    // Note: This won't work always, as some exeptions, such as segfaults, cannot be caught.
    try {
        exitCode = app.exec();
    } catch (const std::exception& e) {
        exitCode = EXIT_FAILURE;
        qCCritical(caQtDMLog) << e.what();
    }

    return exitCode;
}
