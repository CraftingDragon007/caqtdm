/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef CA3DCONFIG_H
#define CA3DCONFIG_H

#include <QList>
#include <QColor>
#include <QRect>
#include <QSizeF>
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

struct QTCON_EXPORT ca3DBindingConfig
{
    enum BindingTarget {
        TranslationX,
        TranslationY,
        TranslationZ,
        RotationX,
        RotationY,
        RotationZ,
        InvalidTarget
    };

    enum BindingMode { Relative, Absolute };

    QString channel;
    QString targetName;
    BindingTarget target = InvalidTarget;
    BindingMode mode = Relative;
    double scale = 1.0;
    double offset = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
    bool hasMinimum = false;
    bool hasMaximum = false;
};

struct QTCON_EXPORT ca3DObjectConfig
{
    QString id;
    QString masterObjectId;
    QString mesh;
    QString meshResolved;
    QString type;
    QString texture;
    QString textureResolved;
    QColor materialColor;
    bool hasMaterialColor = false;
    QVector3D position;
    QVector3D rotation;
    double scale = 1.0;
    QVector3D configuredOriginPosition;
    QVector3D configuredOriginRotation;
    QList<ca3DAxisConfig> axes;
    QList<ca3DBindingConfig> bindings;
};

struct QTCON_EXPORT ca3DOverlayConfig
{
    enum VisibilityMode { PresetOnly, InView, AlwaysWhenInView };

    QString id;
    QString includeFile;
    QString includeFileResolved;
    QString macro;
    QVector3D position;
    QVector3D rotation;
    QSizeF size;
    VisibilityMode visibilityMode = PresetOnly;
    QRect fallbackGeometry;
    bool transparentBackground = true;
};

struct QTCON_EXPORT ca3DCameraPresetConfig
{
    int id = 0;
    QString name;
    QVector3D position;
    QVector3D viewCenter;
    QVector3D upVector = QVector3D(0.0f, 1.0f, 0.0f);
    double yaw = 0.0;
    double pitch = 0.0;
    double fov = 45.0;
    bool hasViewCenter = false;
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
