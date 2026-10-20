#ifndef CAFRAME_RESIZE_RUNTIME_H
#define CAFRAME_RESIZE_RUNTIME_H

#include <QFrame>
#include <QLoggingCategory>
#include <QString>
#include <QVariantList>

// Runtime helpers for the frame-specific part of display resizing. The
// dispatcher still decides when a widget is eligible for resizing.
class caFrameResizeRuntime
{
public:
    static bool isEligible(const QString &className)
    {
        return className == QLatin1String("caFrame") ||
               className == QLatin1String("QFrame");
    }

    static void captureBaseline(QFrame *frame, QVariantList &geometry)
    {
        geometry.insert(4, frame->lineWidth());
        geometry.insert(5, frame->frameWidth());
    }

    static void resize(const QString &className, QWidget *widget,
                       const QVariantList &geometry, double factX, double factY,
                       const QLoggingCategory &logCategory)
    {
        if (!isEligible(className))
            return;

        QFrame *frame = static_cast<QFrame *>(widget);
        double lineWidth;
        if (frame->frameShape() == QFrame::HLine ||
            frame->frameShape() == QFrame::VLine) {
            if (frame->frameShape() != QFrame::HLine) {
                lineWidth = static_cast<double>(geometry.at(4).toInt()) * factX;
                qCDebug(logCategory) << "resize vertical line" << widget << lineWidth;
            } else {
                lineWidth = static_cast<double>(geometry.at(4).toInt()) * factY;
                qCDebug(logCategory) << "resize horizontal line" << widget << lineWidth;
            }
            frame->setLineWidth(qRound(lineWidth));
        } else {
            qCDebug(logCategory) << "resize frame" << widget
                                 << static_cast<double>(geometry.at(5).toInt()) * qMin(factX, factY);
            if ((geometry.count() >= 5) && geometry.at(4).isValid()) {
                lineWidth = static_cast<double>(geometry.at(4).toInt()) * qMin(factX, factY);
                frame->setLineWidth(qRound(lineWidth));
            }
        }
    }
};

#endif // CAFRAME_RESIZE_RUNTIME_H
