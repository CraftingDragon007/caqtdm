/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef CA3DCONFIG_H
#define CA3DCONFIG_H

#include <QList>
#include <QColor>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QVector3D>
#include <qtcontrols_global.h>

struct QTCON_EXPORT ca3DAxisConfig
{
    enum AxisType { Translation, Rotation };

    QString id;
    AxisType type = Translation;
    QVector3D vector;
    double factor = 1.0;
};

struct QTCON_EXPORT ca3DObjectConfig
{
    QString id;
    QString mesh;
    QString meshResolved;
    QString type;
    QString texture;
    QString textureResolved;
    QColor materialColor;
    bool hasMaterialColor = false;
    QVector3D position;
    QVector3D rotation;
    QVector3D configuredOriginPosition;
    QVector3D configuredOriginRotation;
    QList<ca3DAxisConfig> axes;
};

struct QTCON_EXPORT ca3DOverlayConfig
{
    enum VisibilityMode { PresetOnly, InView, AlwaysWhenInView };

    QString id;
    QString includeFile;
    QString includeFileResolved;
    QVector3D position;
    QVector3D rotation;
    VisibilityMode visibilityMode = PresetOnly;
    int cameraPreset = 0;
    QRect fallbackGeometry;
    bool transparentBackground = true;
};

struct QTCON_EXPORT ca3DCameraPresetConfig
{
    int id = 0;
    QString name;
    QVector3D position;
    double yaw = 0.0;
    double pitch = 0.0;
    QString snapshot;
    QString snapshotResolved;
    QStringList overlays;
};

struct QTCON_EXPORT ca3DSceneConfig
{
    QList<ca3DObjectConfig> objects;
    QList<ca3DOverlayConfig> overlays;
    QList<ca3DCameraPresetConfig> cameraPresets;
    QColor backgroundColor = QColor(30, 34, 40);

    void clear();
    bool isEmpty() const;
};

class QTCON_EXPORT ca3DConfigParser
{
public:
    static bool parse(const QString &json, ca3DSceneConfig *config, QStringList *errors = Q_NULLPTR);
    static QString resolveDisplayFile(const QString &fileName);
};

#endif
