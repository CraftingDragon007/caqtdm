/*
 *  This file is part of the caQtDM Framework.
 */

#include "ca3dconfig.h"

#include "searchfile.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace
{
QVector3D vectorFromArray(const QJsonValue &value, const QString &fieldName, QStringList *errors)
{
    if (value.isUndefined() || value.isNull()) {
        return QVector3D();
    }

    const QJsonArray array = value.toArray();
    if (array.size() != 3) {
        if (errors) {
            errors->append(QStringLiteral("%1 must contain exactly 3 numbers").arg(fieldName));
        }
        return QVector3D();
    }

    return QVector3D(static_cast<float>(array.at(0).toDouble()),
                     static_cast<float>(array.at(1).toDouble()),
                     static_cast<float>(array.at(2).toDouble()));
}

QRect rectFromArray(const QJsonValue &value, const QString &fieldName, QStringList *errors)
{
    if (value.isUndefined() || value.isNull()) {
        return QRect();
    }

    const QJsonArray array = value.toArray();
    if (array.size() != 4) {
        if (errors) {
            errors->append(QStringLiteral("%1 must contain exactly 4 numbers").arg(fieldName));
        }
        return QRect();
    }

    return QRect(array.at(0).toInt(), array.at(1).toInt(), array.at(2).toInt(), array.at(3).toInt());
}

QString stringFromObject(const QJsonObject &object, const QString &name)
{
    return object.value(name).toString().trimmed();
}

QString inferMeshType(const QString &mesh)
{
    const QString suffix = QFileInfo(mesh).suffix().toLower();
    if (suffix == QStringLiteral("stl") || suffix == QStringLiteral("obj")) {
        return suffix;
    }
    return QString();
}

ca3DOverlayConfig::VisibilityMode visibilityModeFromString(const QString &value, QStringList *errors)
{
    const QString mode = value.trimmed();
    if (mode.isEmpty() || mode == QStringLiteral("presetOnly")) {
        return ca3DOverlayConfig::PresetOnly;
    }
    if (mode == QStringLiteral("inView")) {
        return ca3DOverlayConfig::InView;
    }
    if (mode == QStringLiteral("alwaysWhenInView")) {
        return ca3DOverlayConfig::AlwaysWhenInView;
    }

    if (errors) {
        errors->append(QStringLiteral("Unknown overlay visibilityMode '%1'").arg(value));
    }
    return ca3DOverlayConfig::PresetOnly;
}

void appendMissingFileError(const QString &fieldName, const QString &fileName, QStringList *errors)
{
    if (errors && !fileName.trimmed().isEmpty()) {
        errors->append(QStringLiteral("%1 '%2' was not found in CAQTDM_DISPLAY_PATH").arg(fieldName, fileName));
    }
}
}

void ca3DSceneConfig::clear()
{
    objects.clear();
    overlays.clear();
    cameraPresets.clear();
}

bool ca3DSceneConfig::isEmpty() const
{
    return objects.isEmpty() && overlays.isEmpty() && cameraPresets.isEmpty();
}

bool ca3DConfigParser::parse(const QString &json, ca3DSceneConfig *config, QStringList *errors)
{
    if (!config) {
        return false;
    }

    config->clear();
    if (errors) {
        errors->clear();
    }

    if (json.trimmed().isEmpty()) {
        return true;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errors) {
            errors->append(QStringLiteral("Invalid sceneConfig JSON: %1").arg(parseError.errorString()));
        }
        return false;
    }

    const QJsonObject root = document.object();

    const QJsonArray objects = root.value(QStringLiteral("objects")).toArray();
    for (const QJsonValue &value : objects) {
        const QJsonObject object = value.toObject();
        ca3DObjectConfig item;
        item.id = stringFromObject(object, QStringLiteral("id"));
        item.mesh = stringFromObject(object, QStringLiteral("mesh"));
        item.meshResolved = resolveDisplayFile(item.mesh);
        item.type = stringFromObject(object, QStringLiteral("type"));
        if (item.type.isEmpty()) {
            item.type = inferMeshType(item.mesh);
        }
        item.texture = stringFromObject(object, QStringLiteral("texture"));
        item.textureResolved = resolveDisplayFile(item.texture);
        item.position = vectorFromArray(object.value(QStringLiteral("position")), QStringLiteral("object.position"), errors);
        item.rotation = vectorFromArray(object.value(QStringLiteral("rotation")), QStringLiteral("object.rotation"), errors);
        item.configuredOriginPosition = vectorFromArray(object.value(QStringLiteral("configuredOriginPosition")), QStringLiteral("object.configuredOriginPosition"), errors);
        item.configuredOriginRotation = vectorFromArray(object.value(QStringLiteral("configuredOriginRotation")), QStringLiteral("object.configuredOriginRotation"), errors);

        if (item.id.isEmpty() && errors) {
            errors->append(QStringLiteral("Object without id"));
        }
        if (item.meshResolved.isEmpty()) {
            appendMissingFileError(QStringLiteral("mesh"), item.mesh, errors);
        }

        const QJsonArray axes = object.value(QStringLiteral("axes")).toArray();
        for (const QJsonValue &axisValue : axes) {
            const QJsonObject axisObject = axisValue.toObject();
            ca3DAxisConfig axis;
            axis.id = stringFromObject(axisObject, QStringLiteral("id"));
            const QString type = stringFromObject(axisObject, QStringLiteral("type"));
            axis.type = type == QStringLiteral("rotation") ? ca3DAxisConfig::Rotation : ca3DAxisConfig::Translation;
            axis.vector = vectorFromArray(axisObject.value(axis.type == ca3DAxisConfig::Rotation ? QStringLiteral("axis") : QStringLiteral("vector")),
                                          QStringLiteral("axis.vector"),
                                          errors);
            axis.factor = axisObject.value(QStringLiteral("factor")).toDouble(1.0);
            if (axis.id.isEmpty() && errors) {
                errors->append(QStringLiteral("Axis without id on object '%1'").arg(item.id));
            }
            item.axes.append(axis);
        }

        config->objects.append(item);
    }

    const QJsonArray overlays = root.value(QStringLiteral("overlays")).toArray();
    for (const QJsonValue &value : overlays) {
        const QJsonObject object = value.toObject();
        ca3DOverlayConfig item;
        item.id = stringFromObject(object, QStringLiteral("id"));
        item.includeFile = stringFromObject(object, QStringLiteral("includeFile"));
        item.includeFileResolved = resolveDisplayFile(item.includeFile);
        item.position = vectorFromArray(object.value(QStringLiteral("position")), QStringLiteral("overlay.position"), errors);
        item.rotation = vectorFromArray(object.value(QStringLiteral("rotation")), QStringLiteral("overlay.rotation"), errors);
        item.visibilityMode = visibilityModeFromString(stringFromObject(object, QStringLiteral("visibilityMode")), errors);
        item.cameraPreset = object.value(QStringLiteral("cameraPreset")).toInt(0);
        item.fallbackGeometry = rectFromArray(object.value(QStringLiteral("fallbackGeometry")), QStringLiteral("overlay.fallbackGeometry"), errors);
        item.transparentBackground = object.value(QStringLiteral("transparentBackground")).toBool(true);

        if (item.id.isEmpty() && errors) {
            errors->append(QStringLiteral("Overlay without id"));
        }
        if (item.includeFileResolved.isEmpty()) {
            appendMissingFileError(QStringLiteral("includeFile"), item.includeFile, errors);
        }

        config->overlays.append(item);
    }

    const QJsonArray presets = root.value(QStringLiteral("cameraPresets")).toArray();
    for (const QJsonValue &value : presets) {
        const QJsonObject object = value.toObject();
        ca3DCameraPresetConfig item;
        item.id = object.value(QStringLiteral("id")).toInt(0);
        item.name = stringFromObject(object, QStringLiteral("name"));
        item.position = vectorFromArray(object.value(QStringLiteral("position")), QStringLiteral("cameraPreset.position"), errors);
        item.yaw = object.value(QStringLiteral("yaw")).toDouble(0.0);
        item.pitch = object.value(QStringLiteral("pitch")).toDouble(0.0);
        item.snapshot = stringFromObject(object, QStringLiteral("snapshot"));
        item.snapshotResolved = resolveDisplayFile(item.snapshot);

        const QJsonArray overlayIds = object.value(QStringLiteral("overlays")).toArray();
        for (const QJsonValue &overlayId : overlayIds) {
            item.overlays.append(overlayId.toString());
        }

        if (item.id <= 0 && errors) {
            errors->append(QStringLiteral("Camera preset without positive id"));
        }
        if (item.snapshotResolved.isEmpty()) {
            appendMissingFileError(QStringLiteral("snapshot"), item.snapshot, errors);
        }

        config->cameraPresets.append(item);
    }

    return errors ? errors->isEmpty() : true;
}

QString ca3DConfigParser::resolveDisplayFile(const QString &fileName)
{
    if (fileName.trimmed().isEmpty()) {
        return QString();
    }

    searchFile search(fileName);
    return search.findFile();
}
