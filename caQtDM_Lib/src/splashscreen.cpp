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


#include <QSizePolicy>
#include <QPixmap>
#include <QBitmap>
#include <QPainter>
#include <QLabel>
#include <QWidget>
#include "splashscreen.h"
#include <QDebug>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QStyleOptionProgressBarV2>
#include <QDesktopWidget>
#else
#include <QtGui>
#include <QStyleOptionProgressBar>
#endif
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QImageReader>

#define PROGRESS_BAR_AREA_HEIGHT 50

Q_LOGGING_CATEGORY(splashScreenLog, "caqtdm.lib.spashscreen")

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
SplashScreen::SplashScreen(QWidget *parent) : QSplashScreen(parent), m_progress(0)
#else
SplashScreen::SplashScreen(QWidget *parent) : QSplashScreen(), m_progress(0)
#endif

{
    Qt::WindowFlags flags = Qt::WindowStaysOnTopHint | Qt::SplashScreen | Qt::FramelessWindowHint;
    setWindowFlags(flags);

    setAttribute(Qt::WA_TranslucentBackground);

    m_maximum = 100;

    QDate currentDate = QDate::currentDate();
    QString mappedSplashScreen = getMappedSplashScreenImage(currentDate);
    if (!mappedSplashScreen.isEmpty()) {
        pixmapLoad.load(mappedSplashScreen);
    } else {
        pixmapLoad.load(":logo_caqtdm.png");
    }

    int scaledWidth = 425;
#if defined(MOBILE_IOS)
    QSize size = qApp->primaryScreen()->size();
    if(size.height() < 500) {
       scaledWidth = 212;
    } else {
       scaledWidth = 425;
    }
#elif defined(MOBILE_ANDROID)
    scaledWidth = 635;
#endif
    pixmap = pixmapLoad.scaledToWidth(scaledWidth, Qt::SmoothTransformation);

    const int splashWidth = pixmap.width();
    const int splashHeight = pixmap.height() + PROGRESS_BAR_AREA_HEIGHT;

    // Whole image
    QPixmap splashPixmap(splashWidth, splashHeight);
    splashPixmap.fill(Qt::transparent);

    QPainter painter(&splashPixmap);

    // Icon / Logo
    painter.drawPixmap(0, 0, pixmap);

    // Bottom box where text and progress bar are
    painter.setBrush(QColor(200, 200, 200, 255));
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, pixmap.height(), splashWidth, PROGRESS_BAR_AREA_HEIGHT);

    painter.end();


    this->setPixmap(splashPixmap);
    this->setCursor(Qt::BusyCursor);
    this->showMessage("loading include ui files", Qt::AlignBottom, QColor(Qt::black));
}

QString SplashScreen::getMappedSplashScreenImage(QDate &date)
{
    QFile mappingFile(":splashScreenMapping.json");
    if (!mappingFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
       qCCritical(splashScreenLog) << "Couldn't open splashScreen mapping file";
       return "";
    }

    QByteArray mappingData = mappingFile.readAll();
    mappingFile.close();
    QJsonDocument mappingDocument = QJsonDocument::fromJson(mappingData);
    if (mappingDocument.isNull()) {
       qCCritical(splashScreenLog) << "Couldn't parse JSON from splashScreen mapping file";
       return "";
    }

    QJsonObject mappingObject = mappingDocument.object();
    QJsonValue mappedValue = mappingObject.value(date.toString("MM-dd")); // month-day, zero-padded

    if (isEaster(date) && mappingObject.contains("EASTER")) {
        mappedValue = mappingObject.value("EASTER");
    } else if (isCoffeeTime(QTime::currentTime()) && mappingObject.contains("COFFEE")) {
        mappedValue = mappingObject.value("COFFEE");
    } else if (mappedValue.isUndefined() && mappingObject.contains("RANDOM")
               && QRandomGenerator::global()->bounded(100.0) > 99.0) {
        // 1% chance to take one of the randomly available ones
        mappedValue = mappingObject.value("RANDOM");
    }

    if (mappedValue.isUndefined()) {
        return "";
    }

    QString mappedImagePath;
    if (mappedValue.isArray()) {
        QJsonArray mappedValueArray = mappedValue.toArray();
        mappedImagePath = mappedValueArray
                              .at(QRandomGenerator::global()->bounded(mappedValueArray.size()))
                              .toString();
    } else {
        mappedImagePath = mappedValue.toString();
    }

    if (mappedImagePath.isEmpty()) {
        qCCritical(splashScreenLog) << "splashScreen mapped filepath is empty";
        return "";
    }

    QImageReader reader(mappedImagePath);
    if (reader.format() != "png") {
        qCCritical(splashScreenLog) << "mapped splashScreen File is not a valid png: " << reader.fileName()
                    << " error: " << reader.errorString();
        return "";
    }

    return mappedImagePath;
}

bool SplashScreen::isEaster(QDate &date) {
    return easter_gregorian(date.year()) == std::pair<int, int>(date.month(), date.day());
}

bool SplashScreen::isCoffeeTime(QTime time) {
    return time.hour() == 16 && time.minute() == 0;
}

// from https://www.daniweb.com/programming/software-development/threads/463261/c-easter-day-calculation
std::pair<int, int> SplashScreen::easter_gregorian(int y) {
   if (y < 1583 || y > 9999) throw std::out_of_range("Gregorian years only");
   int a = y % 19;
   int b = y / 100, c = y % 100;
   int d = b / 4, e = b % 4;
   int f = (b + 8) / 25;
   int g = (b - f + 1) / 3;
   int h = (19*a + b - d - g + 15) % 30;
   int i = c / 4, k = c % 4;
   int l = (32 + 2*e + 2*i - h - k) % 7;
   int m = (a + 11*h + 22*l) / 451;
   int month = (h + l - 7*m + 114) / 31;          // 3=March, 4=April
   int day   = ((h + l - 7*m + 114) % 31) + 1;
   return {month, day};
}

void SplashScreen::setMaximum(int max)
{
    m_maximum = max;
}

void SplashScreen::drawContents(QPainter *painter)
{
      QSplashScreen::drawContents(painter);
#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
      QStyleOptionProgressBarV2 pbstyle;
      pbstyle.initFrom(this);
      pbstyle.state = QStyle::State_Enabled;
#else
      QStyleOptionProgressBar pbstyle;
      pbstyle.initFrom(this);
      pbstyle.state = QStyle::State_Enabled|QStyle::State_Horizontal;

#endif
      pbstyle.textVisible = false;
      pbstyle.minimum = 0;
      pbstyle.maximum = m_maximum;
      pbstyle.progress = m_progress;
      pbstyle.invertedAppearance = false;
      pbstyle.text = "loading";
      pbstyle.textVisible = true;
      pbstyle.rect = QRect(5, pixmap.height() + 3, pixmap.width() - 10, PROGRESS_BAR_AREA_HEIGHT  / 2 - 5);

      style()->drawControl(QStyle::CE_ProgressBar, &pbstyle, painter, this);

}
