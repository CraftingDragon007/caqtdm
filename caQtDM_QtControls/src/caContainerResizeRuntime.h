#ifndef CACONTAINERRESIZERUNTIME_H
#define CACONTAINERRESIZERUNTIME_H

#include <QFont>
#include <QGroupBox>
#include <QString>
#include <QTabWidget>
#include <QVariant>
#include <QVariantList>

// Owns QGroupBox and QTabWidget font presentation during runtime resizing.
// The library retains class dispatch, resize timing, and scale calculation.
class caContainerResizeRuntime
{
public:
    // Returns true when the original negative-font early return applies.
    static bool resizeGroupBox(QGroupBox *box, const QVariantList &geometry,
                               double factX, double factY)
    {
        if (geometry.at(4).toInt() < 0)
            return true; // on android I got -1 for these fonts at initialization, i.e pixelsize

        if (qMin(factX, factY) < 1.0) {
            qreal fontSize = qMin(factX, factY) * (double) geometry.at(4).toInt();
            if (fontSize < 3) fontSize = 3;
            QFont font;
            font.setPointSizeF(fontSize);
            box->setFont(font);
        }
        return false;
    }

    static void captureTabWidgetBaseline(QTabWidget *tabWidget, QVariantList &geometry)
    {
        geometry.insert(4, tabWidget->font().pointSize());
        tabWidget->setProperty("Stylesheet", tabWidget->styleSheet());
    }

    // Returns true when the original negative-font early return applies.
    static bool resizeTabWidget(QTabWidget *tabWidget, const QVariantList &geometry,
                                double factX, double factY)
    {
#ifdef MOBILE
        if (geometry.at(4).toInt() < 0)
            return true; // on android I got -1 for these fonts at initialization, i.e pixelsize

        qreal fontSize = qMin(factX, factY) * (double) geometry.at(4).toInt();
        if (fontSize < 3) fontSize = 3;
        if (fontSize > (double) geometry.at(4).toInt())
            fontSize = (double) geometry.at(4).toInt();

        const qreal height = 1.0;
        QString style = "QTabBar::tab {font: %1pt;  height:%2em; padding: %3px;}";
        style = style.arg(qRound(fontSize)).arg(height).arg(qRound(5.0 * qMin(factX, factY)));

        // Append the stylesheet captured at the original baseline resize.
        const QVariant originalStyle = tabWidget->property("Stylesheet");
        if (!originalStyle.isNull())
            style += originalStyle.toString();
        tabWidget->setStyleSheet(style);
#else
        Q_UNUSED(tabWidget);
        Q_UNUSED(geometry);
        Q_UNUSED(factX);
        Q_UNUSED(factY);
#endif
        return false;
    }
};

#endif // CACONTAINERRESIZERUNTIME_H
