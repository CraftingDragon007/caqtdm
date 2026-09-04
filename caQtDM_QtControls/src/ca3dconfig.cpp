/*
 *  This file is part of the caQtDM Framework.
 */

#include "ca3dconfig.h"

#include "searchfile.h"

#include <QFileInfo>
#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
QColor colorFromValue(const QJsonValue &value, const QColor &fallback, const QString &fieldName, QStringList *errors);

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

bool finiteVectorFromArray(const QJsonValue &value, const QString &fieldName, QVector3D *result, QStringList *errors)
{
    if (value.isUndefined() || value.isNull()) {
        return false;
    }
    const QJsonArray array = value.toArray();
    if (array.size() != 3) {
        if (errors) {
            errors->append(QStringLiteral("%1 must contain exactly 3 numbers").arg(fieldName));
        }
        return false;
    }
    QVector3D vector;
    for (int index = 0; index < 3; ++index) {
        if (!array.at(index).isDouble() || !std::isfinite(array.at(index).toDouble())) {
            if (errors) {
                errors->append(QStringLiteral("%1 must contain finite numbers").arg(fieldName));
            }
            return false;
        }
        vector[index] = static_cast<float>(array.at(index).toDouble());
    }
    if (result) {
        *result = vector;
    }
    return true;
}

void parseLightIntensity(const QJsonObject &object, const QString &fieldName, double *intensity, QStringList *errors)
{
    if (!object.contains(QStringLiteral("intensity"))) {
        return;
    }
    const QJsonValue value = object.value(QStringLiteral("intensity"));
    const double parsed = value.toDouble(std::numeric_limits<double>::quiet_NaN());
    if (!value.isDouble() || !std::isfinite(parsed)) {
        if (errors) {
            errors->append(QStringLiteral("%1.intensity must be a number").arg(fieldName));
        }
        return;
    }
    if (parsed < 0.0) {
        if (errors) {
            errors->append(QStringLiteral("%1.intensity must not be negative").arg(fieldName));
        }
        return;
    }
    *intensity = parsed;
}

void parseLightEnabled(const QJsonObject &object, const QString &fieldName, bool *enabled, QStringList *errors)
{
    if (!object.contains(QStringLiteral("enabled"))) {
        return;
    }
    if (!object.value(QStringLiteral("enabled")).isBool()) {
        if (errors) {
            errors->append(QStringLiteral("%1.enabled must be boolean").arg(fieldName));
        }
        return;
    }
    *enabled = object.value(QStringLiteral("enabled")).toBool();
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

QSizeF sizeFromArray(const QJsonValue &value, const QString &fieldName, QStringList *errors)
{
    if (value.isUndefined() || value.isNull()) {
        return QSizeF();
    }

    const QJsonArray array = value.toArray();
    if (array.size() != 2) {
        if (errors) {
            errors->append(QStringLiteral("%1 must contain exactly 2 numbers").arg(fieldName));
        }
        return QSizeF();
    }

    return QSizeF(array.at(0).toDouble(), array.at(1).toDouble());
}

QString stringFromObject(const QJsonObject &object, const QString &name)
{
    return object.value(name).toString().trimmed();
}

QPair<int, int> lineAndColumnForOffset(const QString &text, int offset)
{
    int line = 1;
    int column = 1;
    const int boundedOffset = std::clamp(offset, 0, int(text.toUtf8().size()));
    int byteOffset = 0;

    for (const QChar &character : text) {
        if (byteOffset >= boundedOffset) {
            break;
        }
        byteOffset += int(QString(character).toUtf8().size());
        if (character == QLatin1Char('\n')) {
            line++;
            column = 1;
        } else {
            column++;
        }
    }

    return qMakePair(line, column);
}

QString inferMeshType(const QString &mesh)
{
    QString suffix = QFileInfo(mesh).suffix().toLower();
    if (suffix == QStringLiteral("stl") || suffix == QStringLiteral("obj")) {
        return suffix;
    }
    return QString();
}

QColor colorFromValue(const QJsonValue &value, const QColor &fallback, const QString &fieldName, QStringList *errors)
{
    if (value.isUndefined() || value.isNull()) {
        return fallback;
    }

    if (value.isString()) {
        const QColor color(value.toString());
        if (color.isValid()) {
            return color;
        }
    } else if (value.isArray()) {
        const QJsonArray array = value.toArray();
        if (array.size() == 3) {
            const QColor color(array.at(0).toInt(), array.at(1).toInt(), array.at(2).toInt());
            if (color.isValid()) {
                return color;
            }
        }
    }

    if (errors) {
        errors->append(QStringLiteral("%1 must be a valid color string or [r,g,b] array").arg(fieldName));
    }
    return fallback;
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

ca3DBindingConfig::BindingTarget bindingTargetFromString(const QString &value, QStringList *errors)
{
    const QString target = value.trimmed();
    if (target == QStringLiteral("translation.x")) {
        return ca3DBindingConfig::TranslationX;
    }
    if (target == QStringLiteral("translation.y")) {
        return ca3DBindingConfig::TranslationY;
    }
    if (target == QStringLiteral("translation.z")) {
        return ca3DBindingConfig::TranslationZ;
    }
    if (target == QStringLiteral("rotation.x")) {
        return ca3DBindingConfig::RotationX;
    }
    if (target == QStringLiteral("rotation.y")) {
        return ca3DBindingConfig::RotationY;
    }
    if (target == QStringLiteral("rotation.z")) {
        return ca3DBindingConfig::RotationZ;
    }
    if (target == QStringLiteral("enabled")) return ca3DBindingConfig::LightEnabled;
    if (target == QStringLiteral("intensity")) return ca3DBindingConfig::LightIntensity;
    if (target == QStringLiteral("direction.x")) return ca3DBindingConfig::LightDirectionX;
    if (target == QStringLiteral("direction.y")) return ca3DBindingConfig::LightDirectionY;
    if (target == QStringLiteral("direction.z")) return ca3DBindingConfig::LightDirectionZ;
    if (target == QStringLiteral("position.x")) return ca3DBindingConfig::LightPositionX;
    if (target == QStringLiteral("position.y")) return ca3DBindingConfig::LightPositionY;
    if (target == QStringLiteral("position.z")) return ca3DBindingConfig::LightPositionZ;

    if (errors) {
        errors->append(QStringLiteral("Unknown binding target '%1'").arg(value));
    }
    return ca3DBindingConfig::InvalidTarget;
}

ca3DBindingConfig::BindingMode bindingModeFromString(const QString &value, QStringList *errors)
{
    const QString mode = value.trimmed();
    if (mode.isEmpty() || mode == QStringLiteral("relative")) {
        return ca3DBindingConfig::Relative;
    }
    if (mode == QStringLiteral("absolute")) {
        return ca3DBindingConfig::Absolute;
    }

    if (errors) {
        errors->append(QStringLiteral("Unknown object binding mode '%1'").arg(value));
    }
    return ca3DBindingConfig::Relative;
}

void appendMissingFileError(const QString &fieldName, const QString &fileName, QStringList *errors)
{
    if (errors && !fileName.trimmed().isEmpty()) {
        errors->append(QStringLiteral("%1 '%2' was not found in CAQTDM_DISPLAY_PATH").arg(fieldName, fileName));
    }
}

bool hasObjectLinkCycle(const QString &objectId,
                        const QMap<QString, QString> &mastersByObject,
                        QSet<QString> *visiting,
                        QSet<QString> *visited)
{
    if (visited->contains(objectId)) {
        return false;
    }
    if (visiting->contains(objectId)) {
        return true;
    }

    visiting->insert(objectId);
    const QString masterId = mastersByObject.value(objectId);
    if (!masterId.isEmpty() && mastersByObject.contains(masterId)) {
        if (hasObjectLinkCycle(masterId, mastersByObject, visiting, visited)) {
            return true;
        }
    }
    visiting->remove(objectId);
    visited->insert(objectId);
    return false;
}

void validateObjectLinks(const QList<ca3DObjectConfig> &objects, QStringList *errors)
{
    if (!errors) {
        return;
    }

    QSet<QString> seenIds;
    QSet<QString> duplicateIds;
    QMap<QString, QString> mastersByObject;
    for (const ca3DObjectConfig &object : objects) {
        if (object.id.isEmpty()) {
            continue;
        }
        if (seenIds.contains(object.id)) {
            duplicateIds.insert(object.id);
        }
        seenIds.insert(object.id);
        mastersByObject.insert(object.id, object.masterObjectId);
    }

    for (const QString &id : duplicateIds) {
        errors->append(QStringLiteral("Duplicate object id '%1'").arg(id));
    }

    for (const ca3DObjectConfig &object : objects) {
        if (object.masterObjectId.isEmpty()) {
            continue;
        }
        if (object.masterObjectId == object.id) {
            errors->append(QStringLiteral("Object '%1' cannot use itself as masterObject").arg(object.id));
        } else if (!seenIds.contains(object.masterObjectId)) {
            errors->append(QStringLiteral("Object '%1' references unknown masterObject '%2'")
                               .arg(object.id, object.masterObjectId));
        }
    }

    QSet<QString> visiting;
    QSet<QString> visited;
    for (auto it = mastersByObject.cbegin(); it != mastersByObject.cend(); ++it) {
        if (hasObjectLinkCycle(it.key(), mastersByObject, &visiting, &visited)) {
            errors->append(QStringLiteral("Object link cycle detected at '%1'").arg(it.key()));
            return;
        }
    }
}

} // namespace

QColor ca3DContrastingTextColor(const QColor &background)
{
    const auto linearChannel = [](int channel) {
        const double value = static_cast<double>(channel) / 255.0;
        return value <= 0.03928 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    const double luminance = 0.2126 * linearChannel(background.red())
                             + 0.7152 * linearChannel(background.green())
                             + 0.0722 * linearChannel(background.blue());
    const double whiteContrast = 1.05 / (luminance + 0.05);
    const double blackContrast = (luminance + 0.05) / 0.05;
    return whiteContrast >= blackContrast ? QColor(Qt::white) : QColor(Qt::black);
}

void ca3DSceneConfig::clear()
{
    objects.clear();
    overlays.clear();
    cameraPresets.clear();
    backgroundColor = QColor(30, 34, 40);
    lightingEnabled = true;
    ambientLight = ca3DAmbientLightConfig();
    lights.clear();
    ca3DLightConfig key;
    key.id = QStringLiteral("directional");
    key.type = ca3DLightConfig::Directional;
    lights.append(key);
    ca3DLightConfig point;
    point.id = QStringLiteral("point");
    point.type = ca3DLightConfig::Point;
    lights.append(point);
    ca3DLightConfig rim;
    rim.id = QStringLiteral("fill");
    rim.type = ca3DLightConfig::Directional;
    rim.enabled = false;
    rim.direction = QVector3D(0.4f, 0.8f, 0.6f);
    lights.append(rim);
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
            const QPair<int, int> position = lineAndColumnForOffset(json, parseError.offset);
            errors->append(QStringLiteral("Invalid sceneConfig JSON at line %1, character %2: %3")
                               .arg(position.first)
                               .arg(position.second)
                               .arg(parseError.errorString()));
        }
        return false;
    }

    const QJsonObject root = document.object();
    config->backgroundColor = colorFromValue(root.value(QStringLiteral("backgroundColor")),
                                             config->backgroundColor,
                                             QStringLiteral("backgroundColor"),
                                             errors);

    const QJsonObject lighting = root.value(QStringLiteral("lighting")).toObject();
    if (lighting.contains(QStringLiteral("enabled"))) {
        if (!lighting.value(QStringLiteral("enabled")).isBool()) {
            if (errors) {
                errors->append(QStringLiteral("lighting.enabled must be boolean"));
            }
        } else {
            config->lightingEnabled = lighting.value(QStringLiteral("enabled")).toBool();
        }
    }
    const QJsonObject ambient = lighting.value(QStringLiteral("ambient")).toObject();
    config->ambientLight.color = colorFromValue(ambient.value(QStringLiteral("color")),
                                                config->ambientLight.color,
                                                QStringLiteral("lighting.ambient.color"),
                                                errors);
    parseLightIntensity(ambient, QStringLiteral("lighting.ambient"), &config->ambientLight.intensity, errors);

    for (auto it = lighting.constBegin(); it != lighting.constEnd(); ++it) {
        const QString &name = it.key();
        if (name != QStringLiteral("enabled") && name != QStringLiteral("ambient") && name != QStringLiteral("lights")
            && name.endsWith(QStringLiteral("Light")) && errors) {
            errors->append(QStringLiteral("lighting.%1 is obsolete; use lighting.lights instead").arg(name));
        }
    }

    const QJsonArray lights = lighting.value(QStringLiteral("lights")).toArray();
    if (lighting.contains(QStringLiteral("lights"))) {
        config->lights.clear();
    }
    QSet<QString> lightIds;
    for (const auto &lightValue : lights) {
        const QJsonObject lightObject = lightValue.toObject();
        ca3DLightConfig light;
        light.id = stringFromObject(lightObject, QStringLiteral("id"));
        const QString type = stringFromObject(lightObject, QStringLiteral("type"));
        if (type == QStringLiteral("point")) {
            light.type = ca3DLightConfig::Point;
        } else if (type == QStringLiteral("spot")) {
            light.type = ca3DLightConfig::Spot;
        } else if (type == QStringLiteral("directional") || type.isEmpty()) {
            light.type = ca3DLightConfig::Directional;
        } else if (errors) {
            errors->append(QStringLiteral("Unknown light type '%1' for light '%2'").arg(type, light.id));
        }
        if (light.id.isEmpty() && errors) {
            errors->append(QStringLiteral("Light without id"));
        } else if (lightIds.contains(light.id) && errors) {
            errors->append(QStringLiteral("Duplicate light id '%1'").arg(light.id));
        }
        lightIds.insert(light.id);
        const QString field = QStringLiteral("lighting.lights[%1]").arg(config->lights.size());
        parseLightEnabled(lightObject, field, &light.enabled, errors);
        light.color = colorFromValue(lightObject.value(QStringLiteral("color")), light.color, field + QStringLiteral(".color"), errors);
        parseLightIntensity(lightObject, field, &light.intensity, errors);
        QVector3D vector;
        if (finiteVectorFromArray(lightObject.value(QStringLiteral("direction")), field + QStringLiteral(".direction"), &vector, errors)) {
            if (vector.lengthSquared() == 0.0f && light.type != ca3DLightConfig::Point && errors)
                errors->append(QStringLiteral("%1.direction must not be zero").arg(field));
            else if (vector.lengthSquared() > 0.0f)
                light.direction = vector;
        }
        if (finiteVectorFromArray(lightObject.value(QStringLiteral("position")), field + QStringLiteral(".position"), &vector, errors))
            light.position = vector;
        const auto nonNegative = [lightObject, field, errors](const QString &name, double *target) {
            if (!lightObject.contains(name)) return;
            const double value = lightObject.value(name).toDouble(std::numeric_limits<double>::quiet_NaN());
            if (!std::isfinite(value) || value < 0.0) {
                if (errors) errors->append(QStringLiteral("%1.%2 must be a non-negative number").arg(field, name));
            } else *target = value;
        };
        nonNegative(QStringLiteral("cutOffAngle"), &light.cutOffAngle);
        nonNegative(QStringLiteral("constantAttenuation"), &light.constantAttenuation);
        nonNegative(QStringLiteral("linearAttenuation"), &light.linearAttenuation);
        nonNegative(QStringLiteral("quadraticAttenuation"), &light.quadraticAttenuation);
        const QJsonArray bindings = lightObject.value(QStringLiteral("bindings")).toArray();
        for (const auto &bindingValue : bindings) {
            const QJsonObject bindingObject = bindingValue.toObject();
            ca3DBindingConfig binding;
            binding.channel = stringFromObject(bindingObject, QStringLiteral("channel"));
            binding.targetName = stringFromObject(bindingObject, QStringLiteral("target"));
            binding.target = bindingTargetFromString(binding.targetName, errors);
            binding.mode = bindingModeFromString(stringFromObject(bindingObject, QStringLiteral("mode")), errors);
            binding.scale = bindingObject.value(QStringLiteral("scale")).toDouble(1.0);
            binding.offset = bindingObject.value(QStringLiteral("offset")).toDouble(0.0);
            if (bindingObject.contains(QStringLiteral("min"))) { binding.minimum = bindingObject.value(QStringLiteral("min")).toDouble(); binding.hasMinimum = true; }
            if (bindingObject.contains(QStringLiteral("max"))) { binding.maximum = bindingObject.value(QStringLiteral("max")).toDouble(); binding.hasMaximum = true; }
            if (binding.channel.isEmpty() && errors) errors->append(QStringLiteral("Binding without channel on light '%1'").arg(light.id));
            if ((binding.target < ca3DBindingConfig::LightEnabled || binding.target == ca3DBindingConfig::InvalidTarget) && errors)
                errors->append(QStringLiteral("Invalid light binding target '%1'").arg(binding.targetName));
            light.bindings.append(binding);
        }
        config->lights.append(light);
    }

    const QJsonArray objects = root.value(QStringLiteral("objects")).toArray();
    for (const auto &value : objects) {
        const QJsonObject object = value.toObject();
        ca3DObjectConfig item;
        item.id = stringFromObject(object, QStringLiteral("id"));
        item.masterObjectId = stringFromObject(object, QStringLiteral("masterObject"));
        item.mesh = object.contains(QStringLiteral("mesh"))
                        ? stringFromObject(object, QStringLiteral("mesh"))
                        : stringFromObject(object, QStringLiteral("meshFile"));
        item.meshResolved = resolveDisplayFile(item.mesh);
        item.type = stringFromObject(object, QStringLiteral("type"));
        if (item.type.isEmpty()) {
            item.type = inferMeshType(item.mesh);
        }
        item.texture = object.contains(QStringLiteral("texture"))
                           ? stringFromObject(object, QStringLiteral("texture"))
                           : stringFromObject(object, QStringLiteral("textureFile"));
        item.textureResolved = resolveDisplayFile(item.texture);
        const QJsonValue materialColorValue = object.value(QStringLiteral("materialColor"));
        if (!materialColorValue.isUndefined() && !materialColorValue.isNull()) {
            item.materialColor = colorFromValue(materialColorValue,
                                                item.materialColor,
                                                QStringLiteral("object.materialColor"),
                                                errors);
            item.hasMaterialColor = item.materialColor.isValid();
        }
        if (object.value(QStringLiteral("material")).isObject()) {
            const QJsonValue nestedMaterialColorValue = object.value(QStringLiteral("material")).toObject().value(QStringLiteral("color"));
            if (!nestedMaterialColorValue.isUndefined() && !nestedMaterialColorValue.isNull()) {
                item.materialColor = colorFromValue(nestedMaterialColorValue,
                                                    item.materialColor,
                                                    QStringLiteral("object.material.color"),
                                                    errors);
                item.hasMaterialColor = item.materialColor.isValid();
            }
        }
        item.position = vectorFromArray(object.value(QStringLiteral("position")), QStringLiteral("object.position"), errors);
        item.rotation = vectorFromArray(object.value(QStringLiteral("rotation")), QStringLiteral("object.rotation"), errors);
        item.scale = object.value(QStringLiteral("scale")).toDouble(1.0);
        item.configuredOriginPosition = vectorFromArray(object.value(QStringLiteral("configuredOriginPosition")), QStringLiteral("object.configuredOriginPosition"), errors);
        item.configuredOriginRotation = vectorFromArray(object.value(QStringLiteral("configuredOriginRotation")), QStringLiteral("object.configuredOriginRotation"), errors);

        if (item.id.isEmpty() && errors) {
            errors->append(QStringLiteral("Object without id"));
        }
        if (item.meshResolved.isEmpty()) {
            appendMissingFileError(QStringLiteral("mesh"), item.mesh, errors);
        }

        const QJsonArray axes = object.value(QStringLiteral("axes")).toArray();
        for (const auto &axisValue : axes) {
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

        const QJsonArray bindings = object.value(QStringLiteral("bindings")).toArray();
        for (const auto &bindingValue : bindings) {
            const QJsonObject bindingObject = bindingValue.toObject();
            ca3DBindingConfig binding;
            binding.channel = stringFromObject(bindingObject, QStringLiteral("channel"));
            binding.targetName = stringFromObject(bindingObject, QStringLiteral("target"));
            binding.target = bindingTargetFromString(binding.targetName, errors);
            binding.mode = bindingModeFromString(stringFromObject(bindingObject, QStringLiteral("mode")), errors);
            binding.scale = bindingObject.value(QStringLiteral("scale")).toDouble(1.0);
            binding.offset = bindingObject.value(QStringLiteral("offset")).toDouble(0.0);
            if (bindingObject.contains(QStringLiteral("min"))) {
                binding.minimum = bindingObject.value(QStringLiteral("min")).toDouble();
                binding.hasMinimum = true;
            }
            if (bindingObject.contains(QStringLiteral("max"))) {
                binding.maximum = bindingObject.value(QStringLiteral("max")).toDouble();
                binding.hasMaximum = true;
            }
            if (binding.channel.isEmpty() && errors) {
                errors->append(QStringLiteral("Binding without channel on object '%1'").arg(item.id));
            }
            if (binding.target == ca3DBindingConfig::InvalidTarget && errors) {
                errors->append(QStringLiteral("Binding without valid target on object '%1'").arg(item.id));
            }
            item.bindings.append(binding);
        }

        config->objects.append(item);
    }

    validateObjectLinks(config->objects, errors);

    const QJsonArray overlays = root.value(QStringLiteral("overlays")).toArray();
    for (const auto &value : overlays) {
        const QJsonObject object = value.toObject();
        ca3DOverlayConfig item;
        item.id = stringFromObject(object, QStringLiteral("id"));
        item.includeFile = stringFromObject(object, QStringLiteral("includeFile"));
        item.includeFileResolved = resolveDisplayFile(item.includeFile);
        item.macro = stringFromObject(object, QStringLiteral("macro"));
        item.position = vectorFromArray(object.value(QStringLiteral("position")), QStringLiteral("overlay.position"), errors);
        item.rotation = vectorFromArray(object.value(QStringLiteral("rotation")), QStringLiteral("overlay.rotation"), errors);
        item.size = sizeFromArray(object.value(QStringLiteral("size")), QStringLiteral("overlay.size"), errors);
        const QString visibility = object.contains(QStringLiteral("visibilityMode"))
                                       ? stringFromObject(object, QStringLiteral("visibilityMode"))
                                       : stringFromObject(object, QStringLiteral("visibility"));
        item.visibilityMode = visibilityModeFromString(visibility, errors);
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
    for (const auto &value : presets) {
        const QJsonObject object = value.toObject();
        ca3DCameraPresetConfig item;
        item.id = object.value(QStringLiteral("id")).toInt(0);
        item.name = stringFromObject(object, QStringLiteral("name"));
        item.position = vectorFromArray(object.value(QStringLiteral("position")), QStringLiteral("cameraPreset.position"), errors);
        item.hasViewCenter = object.contains(QStringLiteral("viewCenter"));
        item.viewCenter = vectorFromArray(object.value(QStringLiteral("viewCenter")), QStringLiteral("cameraPreset.viewCenter"), errors);
        if (object.contains(QStringLiteral("upVector"))) {
            item.upVector = vectorFromArray(object.value(QStringLiteral("upVector")), QStringLiteral("cameraPreset.upVector"), errors);
        }
        item.yaw = object.value(QStringLiteral("yaw")).toDouble(0.0);
        item.pitch = object.value(QStringLiteral("pitch")).toDouble(0.0);
        item.fov = object.value(QStringLiteral("fov")).toDouble(45.0);
        item.snapshot = stringFromObject(object, QStringLiteral("snapshot"));
        item.snapshotResolved = resolveDisplayFile(item.snapshot);

        const QJsonArray overlayIds = object.value(QStringLiteral("overlays")).toArray();
        for (const auto &overlayId : overlayIds) {
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
