/*
 *  This file is part of the caQtDM Framework.
 */

#include "tst_ca3dconfigdialog.h"

#include "ca3dconfig.h"
#include "ca3dconfigdialog.h"
#include "ca3dwidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>

void TestCa3DConfigDialog::initTestCase()
{
    hadForceFallbackValue = qEnvironmentVariableIsSet("CAQTDM_3D_FORCE_FALLBACK");
    previousForceFallbackValue = qgetenv("CAQTDM_3D_FORCE_FALLBACK");
    qputenv("CAQTDM_3D_FORCE_FALLBACK", QByteArrayLiteral("1"));
}

void TestCa3DConfigDialog::cleanupTestCase()
{
    if (hadForceFallbackValue) {
        qputenv("CAQTDM_3D_FORCE_FALLBACK", previousForceFallbackValue);
    } else {
        qunsetenv("CAQTDM_3D_FORCE_FALLBACK");
    }
}

void TestCa3DConfigDialog::appliesStructuredOverlayChanges()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString overlayFile = tempDir.filePath(QStringLiteral("panel.ui"));
    QFile file(overlayFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QByteArray ui = QByteArrayLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<ui version=\"4.0\"><class>Panel</class>"
        "<widget class=\"QWidget\" name=\"Panel\"><property name=\"geometry\">"
        "<rect><x>0</x><y>0</y><width>320</width><height>200</height></rect>"
        "</property></widget></ui>");
    QCOMPARE(file.write(ui), qint64(ui.size()));
    file.close();

    const QString json = QString::fromLatin1(R"json({
        "backgroundColor": "#112233",
        "objects": [],
        "overlays": [{
            "id": "panel",
            "includeFile": "%1",
            "position": [1.2, 2, 3],
            "rotation": [4, 5, 6],
            "size": [7, 8],
            "visibilityMode": "presetOnly",
            "transparentBackground": true
        }],
        "cameraPresets": [{
            "id": 1,
            "position": [0, 0, 10],
            "viewCenter": [0, 0.9, 0],
            "overlays": ["panel"]
        }]
    })json").arg(overlayFile);

    ca3DWidget widget;
    widget.setSceneConfig(json);
    ca3DConfigDialog dialog(&widget);
    QTableWidget *table = dialog.findChild<QTableWidget *>(QStringLiteral("overlaysTable"));
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 3)->text(), QStringLiteral("1.2"));
    QTableWidget *presetsTable = dialog.findChild<QTableWidget *>(QStringLiteral("presetsTable"));
    QVERIFY(presetsTable);
    QCOMPARE(presetsTable->rowCount(), 1);
    QCOMPARE(presetsTable->item(0, 6)->text(), QStringLiteral("0.9"));
    QDialogButtonBox *buttonBox = dialog.findChild<QDialogButtonBox *>();
    QVERIFY(buttonBox);
    QPushButton *applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QVERIFY(applyButton);
    QVERIFY(!applyButton->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(&dialog, "validateRawJson", Qt::DirectConnection));
    QVERIFY(!applyButton->isEnabled());
    QLabel *validationLabel = dialog.findChild<QLabel *>(QStringLiteral("rawValidationLabel"));
    QVERIFY(validationLabel);
    QCOMPARE(validationLabel->text(), QStringLiteral("JSON is valid"));
    QLabel *errorLabel = dialog.findChild<QLabel *>(QStringLiteral("errorLabel"));
    QVERIFY(errorLabel);
    QVERIFY(errorLabel->isHidden());

    QPlainTextEdit *rawEdit = dialog.findChild<QPlainTextEdit *>();
    QVERIFY(rawEdit);
    rawEdit->setPlainText(QStringLiteral("{"));
    QVERIFY(QMetaObject::invokeMethod(&dialog, "validateRawJson", Qt::DirectConnection));
    QVERIFY(validationLabel->text().contains(QStringLiteral("Invalid sceneConfig JSON")));
    QVERIFY(errorLabel->isHidden());
    rawEdit->setPlainText(json);
    QVERIFY(QMetaObject::invokeMethod(&dialog, "validateRawJson", Qt::DirectConnection));
    QCOMPARE(validationLabel->text(), QStringLiteral("JSON is valid"));
    QVERIFY(errorLabel->isHidden());
    QVERIFY(QMetaObject::invokeMethod(&dialog, "applyChanges", Qt::DirectConnection));
    QVERIFY(!applyButton->isEnabled());

    table->item(0, 2)->setText(QStringLiteral("P=TEST"));
    QVERIFY(applyButton->isEnabled());
    table->item(0, 3)->setText(QStringLiteral("10.5"));
    table->item(0, 12)->setText(QStringLiteral("20"));
    table->item(0, 13)->setText(QStringLiteral("30"));
    table->item(0, 14)->setText(QStringLiteral("400"));
    table->item(0, 15)->setText(QStringLiteral("250"));
    QComboBox *visibility = qobject_cast<QComboBox *>(table->cellWidget(0, 11));
    QVERIFY(visibility);
    visibility->setCurrentText(QStringLiteral("alwaysWhenInView"));
    QCheckBox *transparent = qobject_cast<QCheckBox *>(table->cellWidget(0, 16));
    QVERIFY(transparent);
    transparent->setChecked(false);
    presetsTable->item(0, 1)->setText(QStringLiteral("Front camera"));
    presetsTable->item(0, 2)->setText(QStringLiteral("4.5"));
    presetsTable->item(0, 5)->setText(QStringLiteral("1"));
    presetsTable->item(0, 6)->setText(QStringLiteral("2"));
    presetsTable->item(0, 7)->setText(QStringLiteral("3"));
    presetsTable->item(0, 11)->setText(QStringLiteral("12.5"));
    presetsTable->item(0, 13)->setText(QStringLiteral("35"));
    QWidget *presetOverlaySelector = presetsTable->cellWidget(0, 15);
    QVERIFY(presetOverlaySelector);
    QLineEdit *presetOverlaySummary = presetOverlaySelector->findChild<QLineEdit *>(QStringLiteral("presetOverlaySelection"));
    QVERIFY(presetOverlaySummary);
    QCOMPARE(presetOverlaySummary->text(), QStringLiteral("panel"));
    presetOverlaySummary->setText(QStringLiteral("panel, secondary"));

    QVERIFY(QMetaObject::invokeMethod(&dialog, "applyChanges", Qt::DirectConnection));
    QVERIFY(!applyButton->isEnabled());

    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY2(ca3DConfigParser::parse(widget.getSceneConfig(), &config, &errors),
             qPrintable(errors.join(QLatin1Char('\n'))));
    QCOMPARE(config.overlays.count(), 1);
    const ca3DOverlayConfig overlay = config.overlays.first();
    QCOMPARE(overlay.macro, QStringLiteral("P=TEST"));
    QCOMPARE(overlay.position.x(), 10.5f);
    QCOMPARE(overlay.visibilityMode, ca3DOverlayConfig::AlwaysWhenInView);
    QCOMPARE(overlay.fallbackGeometry, QRect(20, 30, 400, 250));
    QVERIFY(!overlay.transparentBackground);

    QCOMPARE(config.cameraPresets.count(), 1);
    const ca3DCameraPresetConfig preset = config.cameraPresets.first();
    QCOMPARE(preset.name, QStringLiteral("Front camera"));
    QCOMPARE(preset.position.x(), 4.5f);
    QVERIFY(preset.hasViewCenter);
    QCOMPARE(preset.viewCenter, QVector3D(1.0f, 2.0f, 3.0f));
    QCOMPARE(preset.yaw, 12.5);
    QCOMPARE(preset.fov, 35.0);
    QCOMPARE(preset.overlays, QStringList() << QStringLiteral("panel") << QStringLiteral("secondary"));

    const QJsonObject root = QJsonDocument::fromJson(widget.getSceneConfig().toUtf8()).object();
    QCOMPARE(root.value(QStringLiteral("backgroundColor")).toString(), QStringLiteral("#112233"));
    QCOMPARE(root.value(QStringLiteral("cameraPresets")).toArray().count(), 1);
}

void TestCa3DConfigDialog::keepsNewRowsWhenValidatingRawJson()
{
    ca3DWidget widget;
    widget.setSceneConfig(QString());
    ca3DConfigDialog dialog(&widget);

    QTableWidget *presetsTable = dialog.findChild<QTableWidget *>(QStringLiteral("presetsTable"));
    QPlainTextEdit *rawEdit = dialog.findChild<QPlainTextEdit *>();
    QVERIFY(presetsTable);
    QVERIFY(rawEdit);
    QCOMPARE(presetsTable->rowCount(), 0);

    QVERIFY(QMetaObject::invokeMethod(&dialog, "addPresetRow", Qt::DirectConnection));
    QCOMPARE(presetsTable->rowCount(), 1);
    const QJsonObject root = QJsonDocument::fromJson(rawEdit->toPlainText().toUtf8()).object();
    const QJsonArray presets = root.value(QStringLiteral("cameraPresets")).toArray();
    QCOMPARE(presets.count(), 1);
    QVERIFY(presets.first().toObject().value(QStringLiteral("overlays")).isArray());
    QVERIFY(presets.first().toObject().value(QStringLiteral("overlays")).toArray().isEmpty());

    QVERIFY(QMetaObject::invokeMethod(&dialog, "validateRawJson", Qt::DirectConnection));
    QCOMPARE(presetsTable->rowCount(), 1);
}

void TestCa3DConfigDialog::allowsApplyingWithMissingOverlayFile()
{
    const QString json = QString::fromLatin1(R"json({
        "objects": [],
        "overlays": [{
            "id": "missing_overlay",
            "includeFile": "does_not_exist.ui",
            "position": [0, 0, 0],
            "rotation": [0, 0, 0],
            "size": [1.5, 1.0]
        }],
        "cameraPresets": [{
            "id": 1,
            "position": [0, 0, 10]
        }]
    })json");

    ca3DWidget widget;
    ca3DConfigDialog dialog(&widget);
    QPlainTextEdit *rawEdit = dialog.findChild<QPlainTextEdit *>();
    QLabel *validationLabel = dialog.findChild<QLabel *>(QStringLiteral("rawValidationLabel"));
    QLabel *errorLabel = dialog.findChild<QLabel *>(QStringLiteral("errorLabel"));
    QDialogButtonBox *buttonBox = dialog.findChild<QDialogButtonBox *>();
    QVERIFY(rawEdit);
    QVERIFY(validationLabel);
    QVERIFY(errorLabel);
    QVERIFY(buttonBox);

    rawEdit->setPlainText(json);
    QVERIFY(QMetaObject::invokeMethod(&dialog, "validateRawJson", Qt::DirectConnection));
    QCOMPARE(validationLabel->text(), QStringLiteral("JSON is valid"));
    QVERIFY(errorLabel->text().contains(QStringLiteral("includeFile 'does_not_exist.ui' was not found")));

    QPushButton *applyButton = buttonBox->button(QDialogButtonBox::Apply);
    QVERIFY(applyButton);
    QVERIFY(applyButton->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(&dialog, "applyChanges", Qt::DirectConnection));
    QVERIFY(!applyButton->isEnabled());
    ca3DSceneConfig config;
    QStringList errors;
    QVERIFY(!ca3DConfigParser::parse(widget.getSceneConfig(), &config, &errors));
    QVERIFY(errors.contains(QStringLiteral("includeFile 'does_not_exist.ui' was not found in CAQTDM_DISPLAY_PATH")));
    QCOMPARE(config.overlays.count(), 1);
    QCOMPARE(config.overlays.first().id, QStringLiteral("missing_overlay"));
    QCOMPARE(config.overlays.first().includeFile, QStringLiteral("does_not_exist.ui"));
    QCOMPARE(config.overlays.first().position, QVector3D(0.0f, 0.0f, 0.0f));
    QCOMPARE(config.overlays.first().rotation, QVector3D(0.0f, 0.0f, 0.0f));
    QCOMPARE(config.overlays.first().size, QSizeF(1.5, 1.0));
    QCOMPARE(config.cameraPresets.count(), 1);
    QCOMPARE(config.cameraPresets.first().id, 1);
    QCOMPARE(config.cameraPresets.first().position, QVector3D(0.0f, 0.0f, 10.0f));
    QVERIFY(errorLabel->text().contains(QStringLiteral("includeFile 'does_not_exist.ui' was not found")));
}
