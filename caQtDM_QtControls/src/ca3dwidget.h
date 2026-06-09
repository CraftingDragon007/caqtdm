/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef CA3DWIDGET_H
#define CA3DWIDGET_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <qtcontrols_global.h>

#include "ca3dconfig.h"

class QLabel;

class QTCON_EXPORT ca3DWidget : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString sceneConfig READ getSceneConfig WRITE setSceneConfig)
    Q_PROPERTY(bool fallbackMode READ getFallbackMode DESIGNABLE false)
    Q_PROPERTY(bool configValid READ getConfigValid DESIGNABLE false)
    Q_PROPERTY(QStringList configErrors READ getConfigErrors DESIGNABLE false)

public:
    explicit ca3DWidget(QWidget *parent = 0);

    QString getSceneConfig() const { return thisSceneConfig; }
    void setSceneConfig(const QString &config);

    bool getFallbackMode() const { return thisFallbackMode; }
    bool getConfigValid() const { return thisConfigValid; }
    QStringList getConfigErrors() const { return thisConfigErrors; }

public slots:
    void setCameraPreset(int preset);
    void setObjectAxisValue(const QString &objectId, const QString &axisId, double value);
    void setObjectTranslation(const QString &objectId, double x, double y, double z);
    void setObjectRotation(const QString &objectId, double rx, double ry, double rz);

private:
    void updatePlaceholderText();

    QLabel *thisStatusLabel;
    QString thisSceneConfig;
    ca3DSceneConfig thisConfig;
    QStringList thisConfigErrors;
    int thisCameraPreset;
    bool thisFallbackMode;
    bool thisConfigValid;
};

#endif
