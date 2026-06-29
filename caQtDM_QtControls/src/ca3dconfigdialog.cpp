/*
 *  This file is part of the caQtDM Framework.
 */

#include "ca3dconfigdialog.h"

#include "ca3dconfig.h"
#include "ca3dwidget.h"
#include "pvdialog.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>

namespace
{
QString numberString(double value)
{
    return QString::number(value, 'g', 12);
}

QJsonArray vectorArray(const QString &x, const QString &y, const QString &z)
{
    QJsonArray array;
    array.append(x.toDouble());
    array.append(y.toDouble());
    array.append(z.toDouble());
    return array;
}

QJsonArray sizeArray(const QString &width, const QString &height)
{
    QJsonArray array;
    array.append(width.toDouble());
    array.append(height.toDouble());
    return array;
}

QJsonArray rectArray(const QString &x, const QString &y, const QString &width, const QString &height)
{
    QJsonArray array;
    array.append(x.toInt());
    array.append(y.toInt());
    array.append(width.toInt());
    array.append(height.toInt());
    return array;
}
}

ca3DConfigDialog::ca3DConfigDialog(ca3DWidget *widget, QWidget *parent)
    : QDialog(parent ? parent : widget)
    , widget3D(widget)
    , previewWidget(Q_NULLPTR)
    , previewPresetCombo(Q_NULLPTR)
    , captureSnapshotButton(Q_NULLPTR)
    , pendingSnapshotPreset(0)
    , tabs(Q_NULLPTR)
    , objectsTable(Q_NULLPTR)
    , bindingsTable(Q_NULLPTR)
    , overlaysTable(Q_NULLPTR)
    , rawJsonEdit(Q_NULLPTR)
    , errorLabel(Q_NULLPTR)
    , buttonBox(Q_NULLPTR)
{
    buildUi();
    loadFromWidget();
}

void ca3DConfigDialog::buildUi()
{
    setWindowTitle(tr("Edit 3D Scene"));
    resize(1100, 700);

    QVBoxLayout *layout = new QVBoxLayout(this);
    tabs = new QTabWidget(this);

    QWidget *objectsPage = new QWidget(tabs);
    QVBoxLayout *objectsLayout = new QVBoxLayout(objectsPage);
    objectsTable = new QTableWidget(objectsPage);
    objectsTable->setColumnCount(14);
    objectsTable->setHorizontalHeaderLabels(QStringList()
                                            << tr("id") << tr("meshFile") << tr("textureFile") << tr("materialColor")
                                            << tr("pos x") << tr("pos y") << tr("pos z")
                                            << tr("rot x") << tr("rot y") << tr("rot z")
                                            << tr("origin x") << tr("origin y") << tr("origin z")
                                            << tr("scale"));
    objectsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QHBoxLayout *objectsButtons = new QHBoxLayout();
    QPushButton *addObjectButton = new QPushButton(tr("Add Object"), objectsPage);
    QPushButton *removeObjectButton = new QPushButton(tr("Remove Selected"), objectsPage);
    objectsButtons->addWidget(addObjectButton);
    objectsButtons->addWidget(removeObjectButton);
    objectsButtons->addStretch();
    objectsLayout->addWidget(objectsTable);
    objectsLayout->addLayout(objectsButtons);
    tabs->addTab(objectsPage, tr("Objects"));
    connect(addObjectButton, SIGNAL(clicked()), this, SLOT(addObjectRow()));
    connect(removeObjectButton, SIGNAL(clicked()), this, SLOT(removeObjectRow()));

    QWidget *bindingsPage = new QWidget(tabs);
    QVBoxLayout *bindingsLayout = new QVBoxLayout(bindingsPage);
    bindingsTable = new QTableWidget(bindingsPage);
    bindingsTable->setColumnCount(8);
    bindingsTable->setHorizontalHeaderLabels(QStringList()
                                             << tr("object id") << tr("channel") << tr("target") << tr("scale")
                                             << tr("offset") << tr("mode") << tr("min") << tr("max"));
    bindingsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QHBoxLayout *bindingsButtons = new QHBoxLayout();
    QPushButton *addBindingButton = new QPushButton(tr("Add Binding"), bindingsPage);
    QPushButton *removeBindingButton = new QPushButton(tr("Remove Selected"), bindingsPage);
    QPushButton *editPvButton = new QPushButton(tr("Edit PV..."), bindingsPage);
    bindingsButtons->addWidget(addBindingButton);
    bindingsButtons->addWidget(removeBindingButton);
    bindingsButtons->addWidget(editPvButton);
    bindingsButtons->addStretch();
    bindingsLayout->addWidget(bindingsTable);
    bindingsLayout->addLayout(bindingsButtons);
    tabs->addTab(bindingsPage, tr("Bindings"));
    connect(addBindingButton, SIGNAL(clicked()), this, SLOT(addBindingRow()));
    connect(removeBindingButton, SIGNAL(clicked()), this, SLOT(removeBindingRow()));
    connect(editPvButton, SIGNAL(clicked()), this, SLOT(editSelectedBindingPv()));

    QWidget *overlaysPage = new QWidget(tabs);
    QVBoxLayout *overlaysLayout = new QVBoxLayout(overlaysPage);
    overlaysTable = new QTableWidget(overlaysPage);
    overlaysTable->setObjectName(QStringLiteral("overlaysTable"));
    overlaysTable->setColumnCount(18);
    overlaysTable->setHorizontalHeaderLabels(QStringList()
                                             << tr("id") << tr("includeFile") << tr("macro")
                                             << tr("pos x") << tr("pos y") << tr("pos z")
                                             << tr("rot x") << tr("rot y") << tr("rot z")
                                             << tr("width") << tr("height") << tr("visibility")
                                             << tr("camera preset") << tr("fallback x") << tr("fallback y")
                                             << tr("fallback width") << tr("fallback height") << tr("transparent"));
    overlaysTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QHBoxLayout *overlaysButtons = new QHBoxLayout();
    QPushButton *addOverlayButton = new QPushButton(tr("Add Overlay"), overlaysPage);
    QPushButton *removeOverlayButton = new QPushButton(tr("Remove Selected"), overlaysPage);
    overlaysButtons->addWidget(addOverlayButton);
    overlaysButtons->addWidget(removeOverlayButton);
    overlaysButtons->addStretch();
    overlaysLayout->addWidget(overlaysTable);
    overlaysLayout->addLayout(overlaysButtons);
    tabs->addTab(overlaysPage, tr("Overlays"));
    connect(addOverlayButton, SIGNAL(clicked()), this, SLOT(addOverlayRow()));
    connect(removeOverlayButton, SIGNAL(clicked()), this, SLOT(removeOverlayRow()));

    QWidget *previewPage = new QWidget(tabs);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewPage);
    QHBoxLayout *previewButtons = new QHBoxLayout();
    previewPresetCombo = new QComboBox(previewPage);
    QPushButton *refreshPreviewButton = new QPushButton(tr("Refresh Preview"), previewPage);
    captureSnapshotButton = new QPushButton(tr("Capture Snapshot..."), previewPage);
    previewButtons->addWidget(new QLabel(tr("Camera preset:"), previewPage));
    previewButtons->addWidget(previewPresetCombo);
    previewButtons->addWidget(refreshPreviewButton);
    previewButtons->addWidget(captureSnapshotButton);
    previewButtons->addStretch();
    previewWidget = new ca3DWidget(previewPage);
    previewWidget->setMinimumSize(640, 360);
    previewWidget->setForce3DPreview(true);
    previewLayout->addLayout(previewButtons);
    previewLayout->addWidget(previewWidget, 1);
    tabs->addTab(previewPage, tr("Preview"));
    connect(refreshPreviewButton, SIGNAL(clicked()), this, SLOT(refreshPreview()));
    connect(captureSnapshotButton, SIGNAL(clicked()), this, SLOT(captureSnapshot()));
    connect(previewWidget, SIGNAL(snapshotCaptured(QPixmap)), this, SLOT(finishSnapshotCapture(QPixmap)));
    connect(previewWidget, SIGNAL(snapshotCaptureFailed(QString)), this, SLOT(failSnapshotCapture(QString)));

    QWidget *rawPage = new QWidget(tabs);
    QVBoxLayout *rawLayout = new QVBoxLayout(rawPage);
    rawJsonEdit = new QPlainTextEdit(rawPage);
    QPushButton *validateButton = new QPushButton(tr("Validate Raw JSON"), rawPage);
    rawLayout->addWidget(rawJsonEdit);
    rawLayout->addWidget(validateButton);
    tabs->addTab(rawPage, tr("Raw JSON"));
    connect(validateButton, SIGNAL(clicked()), this, SLOT(validateRawJson()));

    errorLabel = new QLabel(this);
    errorLabel->setWordWrap(true);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);

    layout->addWidget(tabs);
    layout->addWidget(errorLabel);
    layout->addWidget(buttonBox);

    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(applyChanges()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void ca3DConfigDialog::loadFromWidget()
{
    const QString json = widget3D ? widget3D->getSceneConfig() : QString();
    rawJsonEdit->setPlainText(json);
    populateTablesFromJson(json);
    populatePresetSelector(json);
    refreshPreview();
}

void ca3DConfigDialog::populateTablesFromJson(const QString &json)
{
    ca3DSceneConfig config;
    QStringList errors;
    if (!ca3DConfigParser::parse(json, &config, &errors)) {
        showErrors(errors);
        tabs->setCurrentWidget(rawJsonEdit->parentWidget());
        return;
    }

    showErrors(QStringList());
    objectsTable->setRowCount(0);
    bindingsTable->setRowCount(0);
    overlaysTable->setRowCount(0);

    for (const ca3DObjectConfig &object : config.objects) {
        const int row = objectsTable->rowCount();
        objectsTable->insertRow(row);
        setTableText(objectsTable, row, 0, object.id);
        setTableText(objectsTable, row, 1, object.mesh);
        setTableText(objectsTable, row, 2, object.texture);
        setTableText(objectsTable, row, 3, object.hasMaterialColor ? object.materialColor.name() : QString());
        setTableText(objectsTable, row, 4, numberString(object.position.x()));
        setTableText(objectsTable, row, 5, numberString(object.position.y()));
        setTableText(objectsTable, row, 6, numberString(object.position.z()));
        setTableText(objectsTable, row, 7, numberString(object.rotation.x()));
        setTableText(objectsTable, row, 8, numberString(object.rotation.y()));
        setTableText(objectsTable, row, 9, numberString(object.rotation.z()));
        setTableText(objectsTable, row, 10, numberString(object.configuredOriginPosition.x()));
        setTableText(objectsTable, row, 11, numberString(object.configuredOriginPosition.y()));
        setTableText(objectsTable, row, 12, numberString(object.configuredOriginPosition.z()));
        setTableText(objectsTable, row, 13, numberString(object.scale));

        for (const ca3DBindingConfig &binding : object.bindings) {
            const int bindingRow = bindingsTable->rowCount();
            bindingsTable->insertRow(bindingRow);
            setTableText(bindingsTable, bindingRow, 0, object.id);
            setTableText(bindingsTable, bindingRow, 1, binding.channel);
            setTableCombo(bindingsTable, bindingRow, 2, QStringList()
                          << QStringLiteral("translation.x") << QStringLiteral("translation.y") << QStringLiteral("translation.z")
                          << QStringLiteral("rotation.x") << QStringLiteral("rotation.y") << QStringLiteral("rotation.z"), binding.targetName);
            setTableText(bindingsTable, bindingRow, 3, numberString(binding.scale));
            setTableText(bindingsTable, bindingRow, 4, numberString(binding.offset));
            setTableCombo(bindingsTable, bindingRow, 5, QStringList() << QStringLiteral("relative") << QStringLiteral("absolute"),
                          binding.mode == ca3DBindingConfig::Absolute ? QStringLiteral("absolute") : QStringLiteral("relative"));
            setTableText(bindingsTable, bindingRow, 6, binding.hasMinimum ? numberString(binding.minimum) : QString());
            setTableText(bindingsTable, bindingRow, 7, binding.hasMaximum ? numberString(binding.maximum) : QString());
        }
    }

    for (const ca3DOverlayConfig &overlay : config.overlays) {
        const int row = overlaysTable->rowCount();
        overlaysTable->insertRow(row);
        setTableText(overlaysTable, row, 0, overlay.id);
        setTableText(overlaysTable, row, 1, overlay.includeFile);
        setTableText(overlaysTable, row, 2, overlay.macro);
        setTableText(overlaysTable, row, 3, numberString(overlay.position.x()));
        setTableText(overlaysTable, row, 4, numberString(overlay.position.y()));
        setTableText(overlaysTable, row, 5, numberString(overlay.position.z()));
        setTableText(overlaysTable, row, 6, numberString(overlay.rotation.x()));
        setTableText(overlaysTable, row, 7, numberString(overlay.rotation.y()));
        setTableText(overlaysTable, row, 8, numberString(overlay.rotation.z()));
        setTableText(overlaysTable, row, 9, numberString(overlay.size.width()));
        setTableText(overlaysTable, row, 10, numberString(overlay.size.height()));
        QString visibility = QStringLiteral("presetOnly");
        if (overlay.visibilityMode == ca3DOverlayConfig::InView) {
            visibility = QStringLiteral("inView");
        } else if (overlay.visibilityMode == ca3DOverlayConfig::AlwaysWhenInView) {
            visibility = QStringLiteral("alwaysWhenInView");
        }
        setTableCombo(overlaysTable, row, 11,
                      QStringList() << QStringLiteral("presetOnly") << QStringLiteral("inView")
                                    << QStringLiteral("alwaysWhenInView"), visibility);
        setTableText(overlaysTable, row, 12, overlay.cameraPreset > 0 ? QString::number(overlay.cameraPreset) : QString());
        if (!overlay.fallbackGeometry.isEmpty()) {
            setTableText(overlaysTable, row, 13, QString::number(overlay.fallbackGeometry.x()));
            setTableText(overlaysTable, row, 14, QString::number(overlay.fallbackGeometry.y()));
            setTableText(overlaysTable, row, 15, QString::number(overlay.fallbackGeometry.width()));
            setTableText(overlaysTable, row, 16, QString::number(overlay.fallbackGeometry.height()));
        } else {
            for (int column = 13; column <= 16; ++column) {
                setTableText(overlaysTable, row, column, QString());
            }
        }
        setTableCheck(overlaysTable, row, 17, overlay.transparentBackground);
    }
}

void ca3DConfigDialog::populatePresetSelector(const QString &json)
{
    if (!previewPresetCombo) {
        return;
    }

    const QVariant selected = previewPresetCombo->currentData();
    previewPresetCombo->clear();
    ca3DSceneConfig config;
    QStringList errors;
    if (!ca3DConfigParser::parse(json, &config, &errors) || config.cameraPresets.isEmpty()) {
        previewPresetCombo->addItem(tr("No preset"), 0);
        return;
    }

    for (const ca3DCameraPresetConfig &preset : config.cameraPresets) {
        const QString label = preset.name.isEmpty()
                              ? QString::number(preset.id)
                              : QStringLiteral("%1 - %2").arg(preset.id).arg(preset.name);
        previewPresetCombo->addItem(label, preset.id);
    }
    const int index = previewPresetCombo->findData(selected);
    if (index >= 0) {
        previewPresetCombo->setCurrentIndex(index);
    }
}

QString ca3DConfigDialog::currentEditorJson() const
{
    return tabs->currentWidget() == rawJsonEdit->parentWidget() ? rawJsonEdit->toPlainText() : jsonFromTables();
}

QString ca3DConfigDialog::jsonFromTables() const
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(rawJsonEdit->toPlainText().toUtf8(), &parseError);
    QJsonObject root = parseError.error == QJsonParseError::NoError && document.isObject()
                       ? document.object()
                       : QJsonObject();
    QJsonArray objects;
    QJsonArray overlays;
    QMap<QString, QJsonArray> bindingsByObject;

    for (int row = 0; row < bindingsTable->rowCount(); ++row) {
        QJsonObject binding;
        binding.insert(QStringLiteral("channel"), tableText(bindingsTable, row, 1));
        binding.insert(QStringLiteral("target"), tableComboText(bindingsTable, row, 2));
        binding.insert(QStringLiteral("scale"), tableText(bindingsTable, row, 3).isEmpty() ? 1.0 : tableText(bindingsTable, row, 3).toDouble());
        binding.insert(QStringLiteral("offset"), tableText(bindingsTable, row, 4).isEmpty() ? 0.0 : tableText(bindingsTable, row, 4).toDouble());
        binding.insert(QStringLiteral("mode"), tableComboText(bindingsTable, row, 5));
        if (!tableText(bindingsTable, row, 6).isEmpty()) {
            binding.insert(QStringLiteral("min"), tableText(bindingsTable, row, 6).toDouble());
        }
        if (!tableText(bindingsTable, row, 7).isEmpty()) {
            binding.insert(QStringLiteral("max"), tableText(bindingsTable, row, 7).toDouble());
        }
        bindingsByObject[tableText(bindingsTable, row, 0)].append(binding);
    }

    for (int row = 0; row < objectsTable->rowCount(); ++row) {
        QJsonObject object;
        const QString objectId = tableText(objectsTable, row, 0);
        object.insert(QStringLiteral("id"), objectId);
        object.insert(QStringLiteral("meshFile"), tableText(objectsTable, row, 1));
        if (!tableText(objectsTable, row, 2).isEmpty()) {
            object.insert(QStringLiteral("textureFile"), tableText(objectsTable, row, 2));
        }
        if (!tableText(objectsTable, row, 3).isEmpty()) {
            object.insert(QStringLiteral("materialColor"), tableText(objectsTable, row, 3));
        }
        object.insert(QStringLiteral("position"), vectorArray(tableText(objectsTable, row, 4), tableText(objectsTable, row, 5), tableText(objectsTable, row, 6)));
        object.insert(QStringLiteral("rotation"), vectorArray(tableText(objectsTable, row, 7), tableText(objectsTable, row, 8), tableText(objectsTable, row, 9)));
        object.insert(QStringLiteral("configuredOriginPosition"), vectorArray(tableText(objectsTable, row, 10), tableText(objectsTable, row, 11), tableText(objectsTable, row, 12)));
        object.insert(QStringLiteral("scale"), tableText(objectsTable, row, 13).isEmpty() ? 1.0 : tableText(objectsTable, row, 13).toDouble());
        if (bindingsByObject.contains(objectId)) {
            object.insert(QStringLiteral("bindings"), bindingsByObject.value(objectId));
        }
        objects.append(object);
    }

    root.insert(QStringLiteral("objects"), objects);
    for (int row = 0; row < overlaysTable->rowCount(); ++row) {
        QJsonObject overlay;
        overlay.insert(QStringLiteral("id"), tableText(overlaysTable, row, 0));
        overlay.insert(QStringLiteral("includeFile"), tableText(overlaysTable, row, 1));
        if (!tableText(overlaysTable, row, 2).isEmpty()) {
            overlay.insert(QStringLiteral("macro"), tableText(overlaysTable, row, 2));
        }
        overlay.insert(QStringLiteral("position"), vectorArray(tableText(overlaysTable, row, 3), tableText(overlaysTable, row, 4), tableText(overlaysTable, row, 5)));
        overlay.insert(QStringLiteral("rotation"), vectorArray(tableText(overlaysTable, row, 6), tableText(overlaysTable, row, 7), tableText(overlaysTable, row, 8)));
        overlay.insert(QStringLiteral("size"), sizeArray(tableText(overlaysTable, row, 9), tableText(overlaysTable, row, 10)));
        overlay.insert(QStringLiteral("visibilityMode"), tableComboText(overlaysTable, row, 11));
        if (!tableText(overlaysTable, row, 12).isEmpty()) {
            overlay.insert(QStringLiteral("cameraPreset"), tableText(overlaysTable, row, 12).toInt());
        }
        bool hasFallbackGeometry = false;
        for (int column = 13; column <= 16; ++column) {
            hasFallbackGeometry = hasFallbackGeometry || !tableText(overlaysTable, row, column).isEmpty();
        }
        if (hasFallbackGeometry) {
            overlay.insert(QStringLiteral("fallbackGeometry"),
                           rectArray(tableText(overlaysTable, row, 13), tableText(overlaysTable, row, 14),
                                     tableText(overlaysTable, row, 15), tableText(overlaysTable, row, 16)));
        }
        overlay.insert(QStringLiteral("transparentBackground"), tableCheck(overlaysTable, row, 17));
        overlays.append(overlay);
    }
    root.insert(QStringLiteral("overlays"), overlays);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void ca3DConfigDialog::addObjectRow()
{
    const int row = objectsTable->rowCount();
    objectsTable->insertRow(row);
    for (int column = 0; column < objectsTable->columnCount(); ++column) {
        setTableText(objectsTable, row, column, QString());
    }
    setTableText(objectsTable, row, 13, QStringLiteral("1.0"));
}

void ca3DConfigDialog::removeObjectRow()
{
    objectsTable->removeRow(objectsTable->currentRow());
}

void ca3DConfigDialog::addBindingRow()
{
    const int row = bindingsTable->rowCount();
    bindingsTable->insertRow(row);
    for (int column = 0; column < bindingsTable->columnCount(); ++column) {
        setTableText(bindingsTable, row, column, QString());
    }
    setTableCombo(bindingsTable, row, 2, QStringList()
                  << QStringLiteral("translation.x") << QStringLiteral("translation.y") << QStringLiteral("translation.z")
                  << QStringLiteral("rotation.x") << QStringLiteral("rotation.y") << QStringLiteral("rotation.z"), QStringLiteral("translation.x"));
    setTableText(bindingsTable, row, 3, QStringLiteral("1.0"));
    setTableText(bindingsTable, row, 4, QStringLiteral("0.0"));
    setTableCombo(bindingsTable, row, 5, QStringList() << QStringLiteral("relative") << QStringLiteral("absolute"), QStringLiteral("relative"));
}

void ca3DConfigDialog::removeBindingRow()
{
    bindingsTable->removeRow(bindingsTable->currentRow());
}

void ca3DConfigDialog::addOverlayRow()
{
    const int row = overlaysTable->rowCount();
    overlaysTable->insertRow(row);
    for (int column = 0; column < overlaysTable->columnCount() - 1; ++column) {
        setTableText(overlaysTable, row, column, QString());
    }
    for (int column = 3; column <= 8; ++column) {
        setTableText(overlaysTable, row, column, QStringLiteral("0"));
    }
    setTableText(overlaysTable, row, 9, QStringLiteral("1.5"));
    setTableText(overlaysTable, row, 10, QStringLiteral("1.0"));
    setTableCombo(overlaysTable, row, 11,
                  QStringList() << QStringLiteral("presetOnly") << QStringLiteral("inView")
                                << QStringLiteral("alwaysWhenInView"), QStringLiteral("presetOnly"));
    setTableCheck(overlaysTable, row, 17, true);
}

void ca3DConfigDialog::removeOverlayRow()
{
    overlaysTable->removeRow(overlaysTable->currentRow());
}

void ca3DConfigDialog::editSelectedBindingPv()
{
    const int row = bindingsTable->currentRow();
    if (row < 0) {
        showErrors(QStringList() << tr("Select a binding row before editing its PV"));
        return;
    }

    PVDialog dialog(tableText(bindingsTable, row, 1), this);
    if (dialog.exec() == QDialog::Accepted) {
        setTableText(bindingsTable, row, 1, dialog.editedChannel());
        showErrors(QStringList());
    }
}

void ca3DConfigDialog::refreshPreview()
{
    if (!previewWidget) {
        return;
    }

    const QString json = currentEditorJson();
    QStringList errors;
    if (!validateJson(json, &errors)) {
        showErrors(errors);
        return;
    }

    previewWidget->setForce3DPreview(true);
    previewWidget->setSceneConfig(json);
    populatePresetSelector(json);
    const int preset = previewPresetCombo ? previewPresetCombo->currentData().toInt() : 0;
    if (preset > 0) {
        previewWidget->setCameraPreset(preset);
    }
    showErrors(QStringList());
}

void ca3DConfigDialog::captureSnapshot()
{
    refreshPreview();
    const int preset = previewPresetCombo ? previewPresetCombo->currentData().toInt() : 0;
    const QString defaultName = preset > 0
                                ? QStringLiteral("3d_preset_%1.png").arg(preset)
                                : QStringLiteral("3d_snapshot.png");
    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          tr("Save 3D Snapshot"),
                                                          defaultName,
                                                          tr("PNG Images (*.png)"));
    if (fileName.isEmpty()) {
        return;
    }

    pendingSnapshotFileName = fileName;
    pendingSnapshotPreset = preset;
    if (captureSnapshotButton) {
        captureSnapshotButton->setEnabled(false);
    }
    showErrors(QStringList() << tr("Capturing 3D background snapshot without overlays..."));
    if (!previewWidget || !previewWidget->capture3DSnapshot(false)) {
        if (captureSnapshotButton) {
            captureSnapshotButton->setEnabled(true);
        }
        pendingSnapshotFileName.clear();
        pendingSnapshotPreset = 0;
    }
}

void ca3DConfigDialog::finishSnapshotCapture(const QPixmap &snapshot)
{
    const QString fileName = pendingSnapshotFileName;
    const int preset = pendingSnapshotPreset;
    pendingSnapshotFileName.clear();
    pendingSnapshotPreset = 0;
    if (captureSnapshotButton) {
        captureSnapshotButton->setEnabled(true);
    }

    if (snapshot.isNull() || !snapshot.save(fileName, "PNG")) {
        showErrors(QStringList() << tr("Could not save snapshot '%1'").arg(fileName));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(currentEditorJson().toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError && document.isObject() && preset > 0) {
        QJsonObject root = document.object();
        QJsonArray presets = root.value(QStringLiteral("cameraPresets")).toArray();
        for (int i = 0; i < presets.count(); ++i) {
            QJsonObject presetObject = presets.at(i).toObject();
            if (presetObject.value(QStringLiteral("id")).toInt() == preset) {
                presetObject.insert(QStringLiteral("snapshot"), fileName);
                presets.replace(i, presetObject);
                break;
            }
        }
        root.insert(QStringLiteral("cameraPresets"), presets);
        const QString json = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
        rawJsonEdit->setPlainText(json);
        populateTablesFromJson(json);
        populatePresetSelector(json);
    }

    showErrors(QStringList() << tr("Saved 3D background snapshot without overlays: %1").arg(fileName));
}

void ca3DConfigDialog::failSnapshotCapture(const QString &error)
{
    pendingSnapshotFileName.clear();
    pendingSnapshotPreset = 0;
    if (captureSnapshotButton) {
        captureSnapshotButton->setEnabled(true);
    }
    showErrors(QStringList() << error);
}

void ca3DConfigDialog::validateRawJson()
{
    QStringList errors;
    if (validateJson(rawJsonEdit->toPlainText(), &errors)) {
        showErrors(QStringList() << tr("JSON is valid"));
        populateTablesFromJson(rawJsonEdit->toPlainText());
        populatePresetSelector(rawJsonEdit->toPlainText());
    } else {
        showErrors(errors);
    }
}

void ca3DConfigDialog::applyChanges()
{
    const QString json = currentEditorJson();
    QStringList errors;
    if (!validateJson(json, &errors)) {
        showErrors(errors);
        return;
    }
    if (widget3D) {
        if (QDesignerFormWindowInterface *formWindow = QDesignerFormWindowInterface::findFormWindow(widget3D)) {
            formWindow->cursor()->setProperty("sceneConfig", json);
        } else {
            widget3D->setSceneConfig(json);
        }
    }
    rawJsonEdit->setPlainText(json);
    populateTablesFromJson(json);
    populatePresetSelector(json);
}

void ca3DConfigDialog::accept()
{
    applyChanges();
    if (errorLabel->text().isEmpty()) {
        QDialog::accept();
    }
}

bool ca3DConfigDialog::validateJson(const QString &json, QStringList *errors) const
{
    ca3DSceneConfig config;
    return ca3DConfigParser::parse(json, &config, errors);
}

QString ca3DConfigDialog::tableText(QTableWidget *table, int row, int column) const
{
    QTableWidgetItem *item = table ? table->item(row, column) : Q_NULLPTR;
    return item ? item->text().trimmed() : QString();
}

void ca3DConfigDialog::setTableText(QTableWidget *table, int row, int column, const QString &text)
{
    table->setItem(row, column, new QTableWidgetItem(text));
}

void ca3DConfigDialog::setTableCombo(QTableWidget *table, int row, int column, const QStringList &items, const QString &currentText)
{
    QComboBox *combo = new QComboBox(table);
    combo->addItems(items);
    const int index = combo->findText(currentText);
    combo->setCurrentIndex(index >= 0 ? index : 0);
    table->setCellWidget(row, column, combo);
}

QString ca3DConfigDialog::tableComboText(QTableWidget *table, int row, int column) const
{
    QComboBox *combo = table ? qobject_cast<QComboBox *>(table->cellWidget(row, column)) : Q_NULLPTR;
    return combo ? combo->currentText() : tableText(table, row, column);
}

void ca3DConfigDialog::setTableCheck(QTableWidget *table, int row, int column, bool checked)
{
    QCheckBox *checkBox = new QCheckBox(table);
    checkBox->setChecked(checked);
    checkBox->setStyleSheet(QStringLiteral("margin-left: 8px"));
    table->setCellWidget(row, column, checkBox);
}

bool ca3DConfigDialog::tableCheck(QTableWidget *table, int row, int column) const
{
    QCheckBox *checkBox = table ? qobject_cast<QCheckBox *>(table->cellWidget(row, column)) : Q_NULLPTR;
    return checkBox && checkBox->isChecked();
}

void ca3DConfigDialog::showErrors(const QStringList &errors)
{
    if (errors.isEmpty()) {
        errorLabel->clear();
    } else if (errors.count() == 1 && errors.first() == tr("JSON is valid")) {
        errorLabel->setText(errors.first());
    } else {
        errorLabel->setText(errors.join(QStringLiteral("\n")));
    }
}
