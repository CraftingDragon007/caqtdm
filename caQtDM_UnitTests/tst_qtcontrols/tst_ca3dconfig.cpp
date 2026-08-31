/*
 *  This file is part of the caQtDM Framework.
 */

#include "tst_ca3dconfig.h"

#include "ca3dconfig.h"
#include "ca3doverlaywidgetmanager.h"
#include "ca3dwidget.h"

#include <QColor>
#include <QCoreApplication>
#include <QFile>
#include <QGraphicsView>
#include <QLabel>
#include <QLineEdit>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QPixmap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QVector3D>

namespace
{
void writeFile(const QString &path, const QByteArray &content = QByteArray("x"))
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(content), qint64(content.size()));
}

QString jsonWithFiles(const QString &mesh,
                      const QString &texture,
                      const QString &overlay,
                      const QString &snapshot)
{
    return QString::fromLatin1(R"json(
{
  "backgroundColor": [1, 2, 3],
  "objects": [
    {
      "id": "pump",
      "masterObject": "",
      "meshFile": "%1",
      "textureFile": "%2",
      "materialColor": "#123456",
      "position": [1, 2, 3],
      "rotation": [4, 5, 6],
      "scale": 2.5,
      "configuredOriginPosition": [7, 8, 9],
      "configuredOriginRotation": [10, 11, 12],
      "axes": [
        {
          "id": "main_axis",
          "type": "rotation",
          "axis": [0, 1, 0],
          "factor": 3.5
        }
      ],
      "bindings": [
        {
          "channel": "TEST:X",
          "target": "translation.x",
          "mode": "absolute",
          "scale": 4,
          "offset": -2,
          "min": -10,
          "max": 10
        }
      ]
    }
  ],
  "overlays": [
    {
      "id": "panel",
      "includeFile": "%3",
      "macro": "P=TEST",
      "position": [0.1, 0.2, 0.3],
      "rotation": [1, 2, 3],
      "size": [5.5, 6.5],
      "visibility": "alwaysWhenInView",
      "fallbackGeometry": [10, 20, 300, 200],
      "transparentBackground": false
    }
  ],
  "cameraPresets": [
    {
      "id": 2,
      "name": "front",
      "position": [9, 8, 7],
      "viewCenter": [6, 5, 4],
      "upVector": [0, 1, 0],
      "fov": 35,
      "snapshot": "%4",
      "overlays": ["panel"]
    }
  ]
}
)json")
        .arg(mesh, texture, overlay, snapshot);
}

QByteArray widgetUi(const QString &className,
                    const QString &objectName,
                    const QSize &size,
                    const QByteArray &extraChildren = QByteArray())
{
    return QString::fromLatin1(R"ui(<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>%1</class>
 <widget class="%1" name="%2">
  <property name="geometry">
   <rect>
    <x>0</x>
    <y>0</y>
    <width>%3</width>
    <height>%4</height>
   </rect>
  </property>
%5
 </widget>
</ui>
)ui")
        .arg(className,
             objectName,
             QString::number(size.width()),
             QString::number(size.height()),
             QString::fromLatin1(extraChildren))
        .toUtf8();
}

QByteArray labelChild(const QString &name, const QString &text, const QRect &geometry)
{
    return QString::fromLatin1(R"ui(
  <widget class="QLabel" name="%1">
   <property name="geometry">
    <rect>
     <x>%2</x>
     <y>%3</y>
     <width>%4</width>
     <height>%5</height>
    </rect>
   </property>
   <property name="text">
    <string>%6</string>
   </property>
  </widget>
)ui")
        .arg(name,
             QString::number(geometry.x()),
             QString::number(geometry.y()),
             QString::number(geometry.width()),
             QString::number(geometry.height()),
             text)
        .toUtf8();
}

QByteArray lineEditChild(const QString &name, const QRect &geometry)
{
    return QString::fromLatin1(R"ui(
  <widget class="QLineEdit" name="%1">
   <property name="geometry">
    <rect>
     <x>%2</x>
     <y>%3</y>
     <width>%4</width>
     <height>%5</height>
    </rect>
   </property>
  </widget>
)ui")
        .arg(name,
             QString::number(geometry.x()),
             QString::number(geometry.y()),
             QString::number(geometry.width()),
             QString::number(geometry.height()))
        .toUtf8();
}

QVector3D matrixTranslation(const QMatrix4x4 &matrix)
{
    return matrix.column(3).toVector3D();
}

void compareVector(const QVector3D &actual, const QVector3D &expected)
{
    QVERIFY2(qAbs(actual.x() - expected.x()) < 0.001f,
             qPrintable(QStringLiteral("x: actual %1 expected %2").arg(actual.x()).arg(expected.x())));
    QVERIFY2(qAbs(actual.y() - expected.y()) < 0.001f,
             qPrintable(QStringLiteral("y: actual %1 expected %2").arg(actual.y()).arg(expected.y())));
    QVERIFY2(qAbs(actual.z() - expected.z()) < 0.001f,
             qPrintable(QStringLiteral("z: actual %1 expected %2").arg(actual.z()).arg(expected.z())));
}

class TestableCa3DWidget : public ca3DWidget
{
public:
    QMatrix4x4 effectiveObjectMotion(const ca3DObjectConfig &object) const
    {
        QMap<QString, QMatrix4x4> cache;
        QSet<QString> visiting;
        return effectiveObjectMotionMatrix(object, &cache, &visiting);
    }
};
}

void TestCa3DConfig::parsesSceneConfigUtilityFields()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString mesh = tempDir.filePath(QStringLiteral("pump.stl"));
    const QString texture = tempDir.filePath(QStringLiteral("pump.png"));
    const QString overlay = tempDir.filePath(QStringLiteral("panel.ui"));
    const QString snapshot = tempDir.filePath(QStringLiteral("front.png"));
    writeFile(mesh);
    writeFile(texture);
    writeFile(overlay);
    writeFile(snapshot);

    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(jsonWithFiles(mesh, texture, overlay, snapshot), &config, &errors),
             qPrintable(errors.join(QLatin1Char('\n'))));
    QVERIFY(errors.isEmpty());

    QCOMPARE(config.backgroundColor, QColor(1, 2, 3));
    QCOMPARE(config.objects.count(), 1);
    const ca3DObjectConfig object = config.objects.first();
    QCOMPARE(object.id, QStringLiteral("pump"));
    QCOMPARE(object.masterObjectId, QString());
    QCOMPARE(object.meshResolved, mesh);
    QCOMPARE(object.textureResolved, texture);
    QCOMPARE(object.type, QStringLiteral("stl"));
    QCOMPARE(object.materialColor, QColor(QStringLiteral("#123456")));
    QVERIFY(object.hasMaterialColor);
    QCOMPARE(object.position, QVector3D(1.0f, 2.0f, 3.0f));
    QCOMPARE(object.rotation, QVector3D(4.0f, 5.0f, 6.0f));
    QCOMPARE(object.configuredOriginPosition, QVector3D(7.0f, 8.0f, 9.0f));
    QCOMPARE(object.configuredOriginRotation, QVector3D(10.0f, 11.0f, 12.0f));
    QCOMPARE(object.scale, 2.5);

    QCOMPARE(object.axes.count(), 1);
    QCOMPARE(object.axes.first().id, QStringLiteral("main_axis"));
    QCOMPARE(object.axes.first().type, ca3DAxisConfig::Rotation);
    QCOMPARE(object.axes.first().vector, QVector3D(0.0f, 1.0f, 0.0f));
    QCOMPARE(object.axes.first().factor, 3.5);

    QCOMPARE(object.bindings.count(), 1);
    const ca3DBindingConfig binding = object.bindings.first();
    QCOMPARE(binding.channel, QStringLiteral("TEST:X"));
    QCOMPARE(binding.target, ca3DBindingConfig::TranslationX);
    QCOMPARE(binding.mode, ca3DBindingConfig::Absolute);
    QCOMPARE(binding.scale, 4.0);
    QCOMPARE(binding.offset, -2.0);
    QVERIFY(binding.hasMinimum);
    QVERIFY(binding.hasMaximum);
    QCOMPARE(binding.minimum, -10.0);
    QCOMPARE(binding.maximum, 10.0);

    QCOMPARE(config.overlays.count(), 1);
    const ca3DOverlayConfig parsedOverlay = config.overlays.first();
    QCOMPARE(parsedOverlay.id, QStringLiteral("panel"));
    QCOMPARE(parsedOverlay.includeFileResolved, overlay);
    QCOMPARE(parsedOverlay.macro, QStringLiteral("P=TEST"));
    QCOMPARE(parsedOverlay.position, QVector3D(0.1f, 0.2f, 0.3f));
    QCOMPARE(parsedOverlay.rotation, QVector3D(1.0f, 2.0f, 3.0f));
    QCOMPARE(parsedOverlay.size.width(), 5.5);
    QCOMPARE(parsedOverlay.size.height(), 6.5);
    QCOMPARE(parsedOverlay.visibilityMode, ca3DOverlayConfig::AlwaysWhenInView);
    QCOMPARE(parsedOverlay.fallbackGeometry, QRect(10, 20, 300, 200));
    QCOMPARE(parsedOverlay.transparentBackground, false);

    QCOMPARE(config.cameraPresets.count(), 1);
    const ca3DCameraPresetConfig preset = config.cameraPresets.first();
    QCOMPARE(preset.id, 2);
    QCOMPARE(preset.name, QStringLiteral("front"));
    QCOMPARE(preset.position, QVector3D(9.0f, 8.0f, 7.0f));
    QCOMPARE(preset.viewCenter, QVector3D(6.0f, 5.0f, 4.0f));
    QVERIFY(preset.hasViewCenter);
    QCOMPARE(preset.upVector, QVector3D(0.0f, 1.0f, 0.0f));
    QCOMPARE(preset.fov, 35.0);
    QCOMPARE(preset.snapshotResolved, snapshot);
    QCOMPARE(preset.overlays, QStringList() << QStringLiteral("panel"));
}

void TestCa3DConfig::parsesLightConfiguration()
{
    QCOMPARE(ca3DContrastingTextColor(QColor(Qt::white)), QColor(Qt::black));
    QCOMPARE(ca3DContrastingTextColor(QColor(30, 34, 40)), QColor(Qt::white));

    const QString json = QStringLiteral(R"json({
        "backgroundColor": "#102030",
        "lighting": {
            "enabled": false,
            "ambient": {
                "color": "#203040",
                "intensity": 0.25
            },
            "lights": [
              {
                "id": "directional",
                "type": "directional",
                "enabled": true,
                "color": "#405060",
                "intensity": 3.5,
                "direction": [-1, 0, 1]
              },
              {
                "id": "point",
                "type": "point",
                "enabled": false,
                "color": [10, 20, 30],
                "intensity": 2.5,
                "position": [1, 2, 3]
              },
              {
                "id": "fill",
                "type": "spot",
                "enabled": true,
                "color": "#a0b0c0",
                "intensity": 0.75,
                "direction": [1, 0.5, -1],
                "position": [4, 5, 6],
                "cutOffAngle": 20
              }
            ]
        }
    })json");

    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(json, &config, &errors), qPrintable(errors.join(QLatin1Char('\n'))));
    QCOMPARE(config.lightingEnabled, false);
    QCOMPARE(config.ambientLight.color, QColor(32, 48, 64));
    QCOMPARE(config.ambientLight.intensity, 0.25);
    QCOMPARE(config.lights.count(), 3);
    QCOMPARE(config.lights.at(0).color, QColor(64, 80, 96));
    QCOMPARE(config.lights.at(0).intensity, 3.5);
    QCOMPARE(config.lights.at(0).direction, QVector3D(-1.0f, 0.0f, 1.0f));
    QCOMPARE(config.lights.at(1).enabled, false);
    QCOMPARE(config.lights.at(1).color, QColor(10, 20, 30));
    QCOMPARE(config.lights.at(1).intensity, 2.5);
    QCOMPARE(config.lights.at(1).position, QVector3D(1.0f, 2.0f, 3.0f));
    QCOMPARE(config.lights.at(2).type, ca3DLightConfig::Spot);
    QCOMPARE(config.lights.at(2).cutOffAngle, 20.0);

    ca3DSceneConfig defaults;
    errors.clear();
    QVERIFY2(ca3DConfigParser::parse(QStringLiteral("{}"), &defaults, &errors),
             qPrintable(errors.join(QLatin1Char('\n'))));
    QCOMPARE(defaults.lights.count(), 3);
    QCOMPARE(defaults.lightingEnabled, true);
    QCOMPARE(defaults.ambientLight.intensity, 0.20);
    QCOMPARE(defaults.lights.at(0).enabled, true);
    QCOMPARE(defaults.lights.at(2).enabled, false);
    QCOMPARE(defaults.lights.at(1).intensity, 1.0);
    QCOMPARE(defaults.lights.at(1).position, QVector3D(0.0f, 500.0f, 500.0f));
}

void TestCa3DConfig::parsesObjectMasterLinks()
{
    const QString json = QStringLiteral(R"json({
        "objects": [
            { "id": "base" },
            { "id": "arm", "masterObject": "base", "position": [10, 0, 0] },
            { "id": "sensor", "masterObject": "arm", "position": [12, 0, 0] }
        ]
    })json");

    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(json, &config, &errors), qPrintable(errors.join(QLatin1Char('\n'))));
    QCOMPARE(config.objects.count(), 3);
    QCOMPARE(config.objects.at(0).masterObjectId, QString());
    QCOMPARE(config.objects.at(1).masterObjectId, QStringLiteral("base"));
    QCOMPARE(config.objects.at(2).masterObjectId, QStringLiteral("arm"));
}

void TestCa3DConfig::parsesCameraPresetsWithoutOverlays()
{
    const QString json = QStringLiteral(R"json({
        "cameraPresets": [
            { "id": 1, "position": [0, 0, 10] },
            { "id": 2, "position": [0, 0, 20], "overlays": [] }
        ]
    })json");

    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(json, &config, &errors), qPrintable(errors.join(QLatin1Char('\n'))));
    QCOMPARE(config.cameraPresets.count(), 2);
    QVERIFY(config.cameraPresets.at(0).overlays.isEmpty());
    QVERIFY(config.cameraPresets.at(1).overlays.isEmpty());
}

void TestCa3DConfig::rejectsInvalidUtilityFields()
{
    const QString json = QStringLiteral(R"json(
{
  "backgroundColor": "not-a-color",
  "objects": [
    {
      "id": "",
      "meshFile": "",
      "position": [1, 2],
      "bindings": [
        {
          "channel": "",
          "target": "translation.bad",
          "mode": "sideways"
        }
      ]
    }
  ],
  "overlays": [
    {
      "id": "",
      "visibilityMode": "sometimes",
      "fallbackGeometry": [1, 2, 3]
    }
  ],
  "cameraPresets": [
    {
      "id": 0,
      "position": [1, 2]
    }
  ]
}
)json");

    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY(!ca3DConfigParser::parse(json, &config, &errors));

    QVERIFY(errors.contains(QStringLiteral("backgroundColor must be a valid color string or [r,g,b] array")));
    QVERIFY(errors.contains(QStringLiteral("object.position must contain exactly 3 numbers")));
    QVERIFY(errors.contains(QStringLiteral("Object without id")));
    QVERIFY(errors.contains(QStringLiteral("Unknown binding target 'translation.bad'")));
    QVERIFY(errors.contains(QStringLiteral("Unknown object binding mode 'sideways'")));
    QVERIFY(errors.contains(QStringLiteral("Binding without channel on object ''")));
    QVERIFY(errors.contains(QStringLiteral("Binding without valid target on object ''")));
    QVERIFY(errors.contains(QStringLiteral("Overlay without id")));
    QVERIFY(errors.contains(QStringLiteral("Unknown overlay visibilityMode 'sometimes'")));
    QVERIFY(errors.contains(QStringLiteral("overlay.fallbackGeometry must contain exactly 4 numbers")));
    QVERIFY(errors.contains(QStringLiteral("cameraPreset.position must contain exactly 3 numbers")));
    QVERIFY(errors.contains(QStringLiteral("Camera preset without positive id")));
}

void TestCa3DConfig::rejectsInvalidLightConfiguration()
{
    const QString json = QStringLiteral(R"json({
        "lighting": {
            "enabled": "yes",
            "lights": [{
                "id": "invalid",
                "type": "point",
                "color": "not-a-color",
                "intensity": -1,
                "position": [1, 2]
            }]
        }
    })json");

    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY(!ca3DConfigParser::parse(json, &config, &errors));
    QVERIFY(errors.contains(QStringLiteral("lighting.lights[0].color must be a valid color string or [r,g,b] array")));
    QVERIFY(errors.contains(QStringLiteral("lighting.lights[0].intensity must not be negative")));
    QVERIFY(errors.contains(QStringLiteral("lighting.lights[0].position must contain exactly 3 numbers")));
    QVERIFY(errors.contains(QStringLiteral("lighting.enabled must be boolean")));
}

void TestCa3DConfig::rejectsInvalidObjectMasterLinks()
{
    const QString unknownMasterJson = QStringLiteral(R"json({
        "objects": [
            { "id": "child", "masterObject": "missing" }
        ]
    })json");
    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY(!ca3DConfigParser::parse(unknownMasterJson, &config, &errors));
    QVERIFY(errors.contains(QStringLiteral("Object 'child' references unknown masterObject 'missing'")));

    const QString selfLinkJson = QStringLiteral(R"json({
        "objects": [
            { "id": "arm", "masterObject": "arm" }
        ]
    })json");
    QVERIFY(!ca3DConfigParser::parse(selfLinkJson, &config, &errors));
    QVERIFY(errors.contains(QStringLiteral("Object 'arm' cannot use itself as masterObject")));

    const QString duplicateJson = QStringLiteral(R"json({
        "objects": [
            { "id": "arm" },
            { "id": "arm" }
        ]
    })json");
    QVERIFY(!ca3DConfigParser::parse(duplicateJson, &config, &errors));
    QVERIFY(errors.contains(QStringLiteral("Duplicate object id 'arm'")));

    const QString cycleJson = QStringLiteral(R"json({
        "objects": [
            { "id": "a", "masterObject": "b" },
            { "id": "b", "masterObject": "c" },
            { "id": "c", "masterObject": "a" }
        ]
    })json");
    QVERIFY(!ca3DConfigParser::parse(cycleJson, &config, &errors));
    QVERIFY(errors.contains(QStringLiteral("Object link cycle detected at 'a'"))
            || errors.contains(QStringLiteral("Object link cycle detected at 'b'"))
            || errors.contains(QStringLiteral("Object link cycle detected at 'c'")));
}

void TestCa3DConfig::resolvesFilesFromDisplayPath()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString meshName = QStringLiteral("relative.stl");
    const QString meshPath = tempDir.filePath(meshName);
    writeFile(meshPath);

    const QByteArray oldDisplayPath = qgetenv("CAQTDM_DISPLAY_PATH");
    qputenv("CAQTDM_DISPLAY_PATH", QFile::encodeName(tempDir.path()));

    ca3DSceneConfig config;
    QStringList errors;
    const QString json = QString::fromLatin1(R"json(
{
  "objects": [
    {
      "id": "relative",
      "meshFile": "%1"
    }
  ]
}
)json").arg(meshName);

    QVERIFY2(ca3DConfigParser::parse(json, &config, &errors), qPrintable(errors.join(QLatin1Char('\n'))));
    QCOMPARE(config.objects.first().meshResolved, meshPath);

    if (oldDisplayPath.isNull()) {
        qunsetenv("CAQTDM_DISPLAY_PATH");
    } else {
        qputenv("CAQTDM_DISPLAY_PATH", oldDisplayPath);
    }
}

void TestCa3DWidget::initTestCase()
{
    thisLibraryPaths = QCoreApplication::libraryPaths();
    QCoreApplication::setLibraryPaths(QStringList());
}

void TestCa3DWidget::cleanupTestCase()
{
    QCoreApplication::setLibraryPaths(thisLibraryPaths);
}

void TestCa3DWidget::ca3DWidgetBuildsForcedFallbackOverlays()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString overlayPath = tempDir.filePath(QStringLiteral("panel.ui"));
    const QString snapshotPath = tempDir.filePath(QStringLiteral("front.png"));
    writeFile(overlayPath, widgetUi(QStringLiteral("QWidget"),
                                    QStringLiteral("overlayRoot"),
                                    QSize(320, 200),
                                    labelChild(QStringLiteral("statusLabel"),
                                               QStringLiteral("Overlay"),
                                               QRect(5, 5, 80, 30))));

    QPixmap snapshot(160, 90);
    snapshot.fill(Qt::red);
    QVERIFY(snapshot.save(snapshotPath, "PNG"));

    const QByteArray oldForceFallback = qgetenv("CAQTDM_3D_FORCE_FALLBACK");
    qputenv("CAQTDM_3D_FORCE_FALLBACK", QByteArrayLiteral("1"));

    ca3DWidget widget;
    widget.resize(640, 400);
    const QString json = QString::fromLatin1(R"json(
{
  "backgroundColor": "#ffffff",
  "objects": [
    {
      "id": "motor",
      "bindings": [
        {
          "channel": "MOTOR:X",
          "target": "translation.x"
        },
        {
          "channel": "MOTOR:RZ",
          "target": "rotation.z"
        }
      ]
    }
  ],
  "overlays": [
    {
      "id": "panel",
      "includeFile": "%1",
      "macro": "P=MOTOR",
      "fallbackGeometry": [10, 20, 320, 200],
      "transparentBackground": true
    }
  ],
  "cameraPresets": [
    {
      "id": 1,
      "position": [1, 2, 3],
      "yaw": 15,
      "pitch": -10,
      "snapshot": "%2",
      "overlays": ["panel"]
    },
    {
      "id": 2,
      "overlays": []
    }
  ]
}
)json")
                             .arg(overlayPath, snapshotPath);

    widget.setSceneConfig(json);
    widget.show();
    QTest::qWait(50);

    QVERIFY(widget.getFallbackMode());
    QCOMPARE(widget.objectBindingChannels(), QStringList() << QStringLiteral("MOTOR:X") << QStringLiteral("MOTOR:RZ"));
    QCOMPARE(widget.overlayRootWidgets().count(), 1);
    QWidget *overlayRoot = widget.overlayRootWidgets().first();
    QVERIFY(overlayRoot);
    QCOMPARE(widget.overlayMacro(overlayRoot), QStringLiteral("P=MOTOR"));
    QCOMPARE(widget.overlayIncludePath(overlayRoot), tempDir.path() + QLatin1Char('/'));

    QSignalSpy xSpy(&widget, qOverload<double>(&ca3DWidget::cameraPositionXChanged));
    QSignalSpy ySpy(&widget, qOverload<double>(&ca3DWidget::cameraPositionYChanged));
    QSignalSpy zSpy(&widget, qOverload<double>(&ca3DWidget::cameraPositionZChanged));
    QSignalSpy yawSpy(&widget, qOverload<double>(&ca3DWidget::cameraYawChanged));
    QSignalSpy pitchSpy(&widget, qOverload<double>(&ca3DWidget::cameraPitchChanged));
    QSignalSpy xIntSpy(&widget, qOverload<int>(&ca3DWidget::cameraPositionXChanged));
    QSignalSpy yawIntSpy(&widget, qOverload<int>(&ca3DWidget::cameraYawChanged));
    widget.setCameraPreset(1);
    QTest::qWait(20);
    QCOMPARE(xSpy.count(), 1);
    QCOMPARE(ySpy.count(), 1);
    QCOMPARE(zSpy.count(), 1);
    QCOMPARE(yawSpy.count(), 1);
    QCOMPARE(pitchSpy.count(), 1);
    QCOMPARE(xIntSpy.count(), 1);
    QCOMPARE(yawIntSpy.count(), 1);
    QCOMPARE(xSpy.takeFirst().first().toDouble(), 1.0);
    QCOMPARE(ySpy.takeFirst().first().toDouble(), 2.0);
    QCOMPARE(zSpy.takeFirst().first().toDouble(), 3.0);
    QCOMPARE(yawSpy.takeFirst().first().toDouble(), 15.0);
    QCOMPARE(pitchSpy.takeFirst().first().toDouble(), -10.0);
    QCOMPARE(xIntSpy.takeFirst().first().toInt(), 1);
    QCOMPARE(yawIntSpy.takeFirst().first().toInt(), 15);
    QList<QGraphicsView *> overlayViews = widget.findChildren<QGraphicsView *>(QStringLiteral("ca3DOverlayView_panel"));
    QCOMPARE(overlayViews.count(), 1);
    QVERIFY(overlayViews.first()->isVisible());
    QCOMPARE(overlayViews.first()->geometry(), QRect(10, 20, 320, 200));

    widget.setCameraPreset(2);
    QTest::qWait(20);
    QVERIFY(!overlayViews.first()->isVisible());

    if (oldForceFallback.isNull()) {
        qunsetenv("CAQTDM_3D_FORCE_FALLBACK");
    } else {
        qputenv("CAQTDM_3D_FORCE_FALLBACK", oldForceFallback);
    }
}

void TestCa3DWidget::linkedObjectFollowsMasterTranslation()
{
    const QString json = QStringLiteral(R"json({
        "objects": [
            { "id": "base", "position": [10, 0, 0] },
            { "id": "child", "masterObject": "base", "position": [15, 0, 0] }
        ]
    })json");
    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(json, &config, &errors), qPrintable(errors.join(QLatin1Char('\n'))));

    TestableCa3DWidget widget;
    widget.setSceneConfig(json);
    widget.setObjectTranslation(QStringLiteral("base"), 5.0, 0.0, 0.0);

    compareVector(matrixTranslation(widget.effectiveObjectMotion(config.objects.at(1))),
                  QVector3D(20.0f, 0.0f, 0.0f));
}

void TestCa3DWidget::linkedObjectFollowsMasterRotation()
{
    const QString json = QStringLiteral(R"json({
        "objects": [
            { "id": "base", "position": [0, 0, 0] },
            { "id": "child", "masterObject": "base", "position": [1, 0, 0] }
        ]
    })json");
    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(json, &config, &errors), qPrintable(errors.join(QLatin1Char('\n'))));

    TestableCa3DWidget widget;
    widget.setSceneConfig(json);
    widget.setObjectRotation(QStringLiteral("base"), 0.0, 0.0, 90.0);

    compareVector(matrixTranslation(widget.effectiveObjectMotion(config.objects.at(1))),
                  QVector3D(0.0f, 1.0f, 0.0f));
}

void TestCa3DWidget::linkedObjectAppliesOwnDynamicMotionInMasterSpace()
{
    const QString json = QStringLiteral(R"json({
        "objects": [
            { "id": "base", "position": [0, 0, 0] },
            { "id": "child", "masterObject": "base", "position": [1, 0, 0] }
        ]
    })json");
    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(json, &config, &errors), qPrintable(errors.join(QLatin1Char('\n'))));

    TestableCa3DWidget widget;
    widget.setSceneConfig(json);
    widget.setObjectRotation(QStringLiteral("base"), 0.0, 0.0, 90.0);
    widget.setObjectTranslation(QStringLiteral("child"), 1.0, 0.0, 0.0);

    compareVector(matrixTranslation(widget.effectiveObjectMotion(config.objects.at(1))),
                  QVector3D(0.0f, 2.0f, 0.0f));
}

void TestCa3DWidget::linkedObjectDoesNotInheritMasterScale()
{
    const QString json = QStringLiteral(R"json({
        "objects": [
            { "id": "base", "position": [0, 0, 0], "scale": 5.0 },
            { "id": "child", "masterObject": "base", "position": [1, 0, 0] }
        ]
    })json");
    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(json, &config, &errors), qPrintable(errors.join(QLatin1Char('\n'))));

    TestableCa3DWidget widget;
    widget.setSceneConfig(json);
    widget.setObjectTranslation(QStringLiteral("base"), 1.0, 0.0, 0.0);

    compareVector(matrixTranslation(widget.effectiveObjectMotion(config.objects.at(1))),
                  QVector3D(2.0f, 0.0f, 0.0f));
}

void TestCa3DWidget::configuredOriginPositionRotatesWithObject()
{
    const QString json = QStringLiteral(R"json({
        "objects": [
            {
                "id": "object",
                "configuredOriginPosition": [1, 0, 0]
            }
        ]
    })json");
    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(json, &config, &errors), qPrintable(errors.join(QLatin1Char('\n'))));

    TestableCa3DWidget widget;
    widget.setSceneConfig(json);
    widget.setObjectRotation(QStringLiteral("object"), 0.0, 0.0, 90.0);

    compareVector(matrixTranslation(widget.effectiveObjectMotion(config.objects.first())),
                  QVector3D(0.0f, 1.0f, 0.0f));
}

void TestCa3DOverlayWidgetManager::initTestCase()
{
    thisLibraryPaths = QCoreApplication::libraryPaths();
    QCoreApplication::setLibraryPaths(QStringList());
}

void TestCa3DOverlayWidgetManager::cleanupTestCase()
{
    QCoreApplication::setLibraryPaths(thisLibraryPaths);
}

void TestCa3DOverlayWidgetManager::ca3DOverlayWidgetManagerLoadsRendersAndTracksDirtyState()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString uiPath = tempDir.filePath(QStringLiteral("overlay.ui"));
    writeFile(uiPath, widgetUi(QStringLiteral("QWidget"),
                               QStringLiteral("overlay"),
                               QSize(120, 80),
                               labelChild(QStringLiteral("label"),
                                          QStringLiteral("Ready"),
                                          QRect(0, 0, 100, 30))));

    ca3DOverlayWidgetManager manager;
    manager.loadWidgetsFromUi(uiPath);

    QVERIFY(manager.contentRoot());
    QCOMPARE(manager.sourceDesignSize(), QSize(120, 80));
    QVERIFY(manager.takeTextureDirty());
    QVERIFY(!manager.takeTextureDirty());

    const QImage snapshot = manager.renderSnapshot(2.0);
    QCOMPARE(snapshot.size(), QSize(240, 160));
    QCOMPARE(snapshot.format(), QImage::Format_ARGB32_Premultiplied);

    manager.contentRoot()->setProperty("changed", true);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(manager.takeTextureDirty());
}

void TestCa3DOverlayWidgetManager::ca3DOverlayWidgetManagerForwardsInput()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString uiPath = tempDir.filePath(QStringLiteral("input.ui"));
    writeFile(uiPath, widgetUi(QStringLiteral("QWidget"),
                               QStringLiteral("overlay"),
                               QSize(180, 60),
                               lineEditChild(QStringLiteral("lineEdit"), QRect(10, 10, 120, 25))));

    ca3DOverlayWidgetManager manager;
    manager.loadWidgetsFromUi(uiPath);
    QVERIFY(manager.contentRoot());

    QLineEdit *lineEdit = manager.contentRoot()->findChild<QLineEdit *>(QStringLiteral("lineEdit"));
    QVERIFY(lineEdit);

    QVERIFY(manager.sendMouseEvent(QPointF(20, 20),
                                   QEvent::MouseButtonPress,
                                   Qt::LeftButton,
                                   Qt::LeftButton,
                                   Qt::NoModifier));
    QVERIFY(manager.sendMouseEvent(QPointF(20, 20),
                                   QEvent::MouseButtonRelease,
                                   Qt::LeftButton,
                                   Qt::NoButton,
                                   Qt::NoModifier));
    QVERIFY(manager.hasFocusedTextInput());

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, QStringLiteral("a"));
    QVERIFY(manager.sendKeyEvent(&keyPress));
    QCOMPARE(lineEdit->text(), QStringLiteral("a"));

    manager.clearOverlayFocus();
    QVERIFY(!manager.hasFocusedTextInput());
    QKeyEvent keyPressWithoutFocus(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, QStringLiteral("b"));
    QVERIFY(!manager.sendKeyEvent(&keyPressWithoutFocus));
    QCOMPARE(lineEdit->text(), QStringLiteral("a"));
}
