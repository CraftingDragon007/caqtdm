/*
 *  This file is part of the caQtDM Framework.
 */

#include "ca3dwidget.h"

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

ca3DWidget::ca3DWidget(QWidget *parent)
    : QWidget(parent)
    , thisStatusLabel(new QLabel(this))
    , thisCameraPreset(0)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    , thisFallbackMode(true)
#else
    , thisFallbackMode(false)
#endif
{
    setMinimumSize(120, 80);
    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(30, 34, 40));
    pal.setColor(QPalette::WindowText, Qt::white);
    setPalette(pal);

    thisStatusLabel->setAlignment(Qt::AlignCenter);
    thisStatusLabel->setWordWrap(true);
    thisStatusLabel->setFrameShape(QFrame::Box);
    thisStatusLabel->setFrameShadow(QFrame::Sunken);
    thisStatusLabel->setMargin(8);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(thisStatusLabel);

    updatePlaceholderText();
}

void ca3DWidget::setSceneConfig(const QString &config)
{
    if (thisSceneConfig == config) {
        return;
    }

    thisSceneConfig = config;
    updatePlaceholderText();
}

void ca3DWidget::setCameraPreset(int preset)
{
    if (preset < 0) {
        return;
    }

    thisCameraPreset = preset;
    updatePlaceholderText();
}

void ca3DWidget::setObjectAxisValue(const QString &objectId, const QString &axisId, double value)
{
    Q_UNUSED(objectId);
    Q_UNUSED(axisId);
    Q_UNUSED(value);
}

void ca3DWidget::setObjectTranslation(const QString &objectId, double x, double y, double z)
{
    Q_UNUSED(objectId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
}

void ca3DWidget::setObjectRotation(const QString &objectId, double rx, double ry, double rz)
{
    Q_UNUSED(objectId);
    Q_UNUSED(rx);
    Q_UNUSED(ry);
    Q_UNUSED(rz);
}

void ca3DWidget::updatePlaceholderText()
{
    const QString mode = thisFallbackMode ? QStringLiteral("2D fallback") : QStringLiteral("Qt6 3D");
    const QString configState = thisSceneConfig.trimmed().isEmpty()
                                ? QStringLiteral("no sceneConfig")
                                : QStringLiteral("sceneConfig set");

    thisStatusLabel->setText(QStringLiteral("ca3DWidget\n%1\nPreset: %2\n%3")
                             .arg(mode)
                             .arg(thisCameraPreset)
                             .arg(configState));
}
