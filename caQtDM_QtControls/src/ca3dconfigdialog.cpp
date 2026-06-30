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
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QJsonParseError>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>

#include <array>
#include <charconv>

namespace
{
QString numberString(double value)
{
    return QString::number(value, 'g', 12);
}

QString numberString(float value)
{
    std::array<char, 32> buffer;
    const std::to_chars_result result = std::to_chars(
        buffer.data(),
        buffer.data() + buffer.size(),
        value,
        std::chars_format::general);

    if (result.ec == std::errc()) {
        return QString::fromLatin1(buffer.data(), static_cast<int>(result.ptr - buffer.data()));
    }
    return QString::number(static_cast<double>(value), 'g', 7);
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

QString safeFileNameComponent(QString value, const QString &fallback)
{
    static const QRegularExpression invalidFileNameChars(
        QStringLiteral("[^A-Za-z0-9._-]+")
        );

    value = value.trimmed();
    value.replace(invalidFileNameChars, QStringLiteral("_"));

    return value.isEmpty() ? fallback : value;
}

QString panelFileName(ca3DWidget *widget)
{
    if (QDesignerFormWindowInterface *formWindow = QDesignerFormWindowInterface::findFormWindow(widget)) {
        return formWindow->fileName();
    }
    return widget && widget->window() ? widget->window()->windowFilePath() : QString();
}

QString relativeSnapshotPath(const QString &fileName, const QString &panelDirectory)
{
    QStringList baseDirectories;
    if (!panelDirectory.isEmpty()) {
        baseDirectories.append(panelDirectory);
    }
    baseDirectories.append(QDir::currentPath());
    const QStringList displayPaths = QString::fromLocal8Bit(qgetenv("CAQTDM_DISPLAY_PATH"))
                                     .split(QDir::listSeparator(), Qt::SkipEmptyParts);
    baseDirectories.append(displayPaths);

    const QString absoluteFileName = QFileInfo(fileName).absoluteFilePath();
    for (const QString &baseDirectory : baseDirectories) {
        const QString relative = QDir(QFileInfo(baseDirectory).absoluteFilePath()).relativeFilePath(absoluteFileName);
        if (relative != QStringLiteral("..") && !relative.startsWith(QStringLiteral("../"))) {
            return QDir::cleanPath(relative);
        }
    }
    return fileName;
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
    , presetsTable(Q_NULLPTR)
    , rawJsonEdit(Q_NULLPTR)
    , rawValidationLabel(Q_NULLPTR)
    , errorLabel(Q_NULLPTR)
    , buttonBox(Q_NULLPTR)
    , updatingUi(false)
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
    overlaysTable->setColumnCount(17);
    overlaysTable->setHorizontalHeaderLabels(QStringList()
                                             << tr("id") << tr("includeFile") << tr("macro")
                                             << tr("pos x") << tr("pos y") << tr("pos z")
                                             << tr("rot x") << tr("rot y") << tr("rot z")
                                             << tr("width") << tr("height") << tr("visibility")
                                             << tr("fallback x") << tr("fallback y")
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

    QWidget *presetsPage = new QWidget(tabs);
    QVBoxLayout *presetsLayout = new QVBoxLayout(presetsPage);
    presetsTable = new QTableWidget(presetsPage);
    presetsTable->setObjectName(QStringLiteral("presetsTable"));
    presetsTable->setColumnCount(16);
    presetsTable->setHorizontalHeaderLabels(QStringList()
                                            << tr("id") << tr("name")
                                            << tr("pos x") << tr("pos y") << tr("pos z")
                                            << tr("view x") << tr("view y") << tr("view z")
                                            << tr("up x") << tr("up y") << tr("up z")
                                            << tr("yaw") << tr("pitch") << tr("fov")
                                            << tr("snapshot") << tr("overlays (multiple)"));
    presetsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QHBoxLayout *presetsButtons = new QHBoxLayout();
    QPushButton *addPresetButton = new QPushButton(tr("Add Preset"), presetsPage);
    QPushButton *removePresetButton = new QPushButton(tr("Remove Selected"), presetsPage);
    presetsButtons->addWidget(addPresetButton);
    presetsButtons->addWidget(removePresetButton);
    presetsButtons->addStretch();
    presetsLayout->addWidget(presetsTable);
    presetsLayout->addLayout(presetsButtons);
    tabs->addTab(presetsPage, tr("Camera Presets"));
    connect(addPresetButton, SIGNAL(clicked()), this, SLOT(addPresetRow()));
    connect(removePresetButton, SIGNAL(clicked()), this, SLOT(removePresetRow()));

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
    connect(tabs, &QTabWidget::currentChanged, this, [this, previewPage](int) {
        if (tabs && tabs->currentWidget() == previewPage) {
            refreshPreview();
        }
    });

    QWidget *rawPage = new QWidget(tabs);
    QVBoxLayout *rawLayout = new QVBoxLayout(rawPage);
    rawJsonEdit = new QPlainTextEdit(rawPage);
    QPushButton *validateButton = new QPushButton(tr("Validate Raw JSON"), rawPage);
    rawValidationLabel = new QLabel(rawPage);
    rawValidationLabel->setObjectName(QStringLiteral("rawValidationLabel"));
    rawValidationLabel->setFixedHeight(validateButton->sizeHint().height());
    QHBoxLayout *validationLayout = new QHBoxLayout();
    validationLayout->addWidget(validateButton);
    validationLayout->addWidget(rawValidationLabel, 1);
    rawLayout->addWidget(rawJsonEdit);
    rawLayout->addLayout(validationLayout);
    tabs->addTab(rawPage, tr("Raw JSON"));
    connect(validateButton, SIGNAL(clicked()), this, SLOT(validateRawJson()));

    errorLabel = new QLabel(this);
    errorLabel->setObjectName(QStringLiteral("errorLabel"));
    errorLabel->setWordWrap(true);
    errorLabel->hide();
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);

    layout->addWidget(tabs);
    layout->addWidget(errorLabel);
    layout->addWidget(buttonBox);

    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(applyChanges()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(objectsTable, SIGNAL(cellChanged(int,int)), this, SLOT(markChanged()));
    connect(bindingsTable, SIGNAL(cellChanged(int,int)), this, SLOT(markChanged()));
    connect(overlaysTable, SIGNAL(cellChanged(int,int)), this, SLOT(markChanged()));
    connect(presetsTable, SIGNAL(cellChanged(int,int)), this, SLOT(markChanged()));
    connect(rawJsonEdit, SIGNAL(textChanged()), this, SLOT(markChanged()));
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void ca3DConfigDialog::loadFromWidget()
{
    updatingUi = true;
    const QString json = widget3D ? widget3D->getSceneConfig() : QString();
    rawJsonEdit->setPlainText(json);
    populateTablesFromJson(json);
    populatePresetSelector(json);
    updatingUi = false;
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void ca3DConfigDialog::populateTablesFromJson(const QString &json)
{
    QScopedValueRollback<bool> updatingGuard(updatingUi, true);
    QStringList syntaxErrors;
    if (!validateJsonSyntax(json, &syntaxErrors)) {
        showErrors(syntaxErrors);
        tabs->setCurrentWidget(rawJsonEdit->parentWidget());
        return;
    }

    ca3DSceneConfig config;
    QStringList errors;
    ca3DConfigParser::parse(json, &config, &errors);

    showErrors(errors);
    objectsTable->setRowCount(0);
    bindingsTable->setRowCount(0);
    overlaysTable->setRowCount(0);
    presetsTable->setRowCount(0);

    foreach (const ca3DObjectConfig &object, config.objects) {
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

    foreach (const ca3DOverlayConfig &overlay, config.overlays) {
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
        if (!overlay.fallbackGeometry.isEmpty()) {
            setTableText(overlaysTable, row, 12, QString::number(overlay.fallbackGeometry.x()));
            setTableText(overlaysTable, row, 13, QString::number(overlay.fallbackGeometry.y()));
            setTableText(overlaysTable, row, 14, QString::number(overlay.fallbackGeometry.width()));
            setTableText(overlaysTable, row, 15, QString::number(overlay.fallbackGeometry.height()));
        } else {
            for (int column = 12; column <= 15; ++column) {
                setTableText(overlaysTable, row, column, QString());
            }
        }
        setTableCheck(overlaysTable, row, 16, overlay.transparentBackground);
    }

    foreach (const ca3DCameraPresetConfig &preset, config.cameraPresets) {
        const int row = presetsTable->rowCount();
        presetsTable->insertRow(row);
        setTableText(presetsTable, row, 0, QString::number(preset.id));
        setTableText(presetsTable, row, 1, preset.name);
        setTableText(presetsTable, row, 2, numberString(preset.position.x()));
        setTableText(presetsTable, row, 3, numberString(preset.position.y()));
        setTableText(presetsTable, row, 4, numberString(preset.position.z()));
        setTableText(presetsTable, row, 5, preset.hasViewCenter ? numberString(preset.viewCenter.x()) : QString());
        setTableText(presetsTable, row, 6, preset.hasViewCenter ? numberString(preset.viewCenter.y()) : QString());
        setTableText(presetsTable, row, 7, preset.hasViewCenter ? numberString(preset.viewCenter.z()) : QString());
        setTableText(presetsTable, row, 8, numberString(preset.upVector.x()));
        setTableText(presetsTable, row, 9, numberString(preset.upVector.y()));
        setTableText(presetsTable, row, 10, numberString(preset.upVector.z()));
        setTableText(presetsTable, row, 11, numberString(preset.yaw));
        setTableText(presetsTable, row, 12, numberString(preset.pitch));
        setTableText(presetsTable, row, 13, numberString(preset.fov));
        setTableText(presetsTable, row, 14, preset.snapshot);
        setPresetOverlaySelector(row, preset.overlays);
    }
}

void ca3DConfigDialog::populatePresetSelector(const QString &json)
{
    if (!previewPresetCombo) {
        return;
    }

    const QVariant selected = previewPresetCombo->currentData();
    previewPresetCombo->clear();
    QStringList syntaxErrors;
    if (!validateJsonSyntax(json, &syntaxErrors)) {
        previewPresetCombo->addItem(tr("No preset"), 0);
        return;
    }

    ca3DSceneConfig config;
    QStringList errors;
    ca3DConfigParser::parse(json, &config, &errors);
    if (config.cameraPresets.isEmpty()) {
        previewPresetCombo->addItem(tr("No preset"), 0);
        return;
    }

    foreach (const ca3DCameraPresetConfig &preset, config.cameraPresets) {
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
    QJsonArray presets;
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
        bool hasFallbackGeometry = false;
        for (int column = 12; column <= 15; ++column) {
            hasFallbackGeometry = hasFallbackGeometry || !tableText(overlaysTable, row, column).isEmpty();
        }
        if (hasFallbackGeometry) {
            overlay.insert(QStringLiteral("fallbackGeometry"),
                           rectArray(tableText(overlaysTable, row, 12), tableText(overlaysTable, row, 13),
                                     tableText(overlaysTable, row, 14), tableText(overlaysTable, row, 15)));
        }
        overlay.insert(QStringLiteral("transparentBackground"), tableCheck(overlaysTable, row, 16));
        overlays.append(overlay);
    }
    root.insert(QStringLiteral("overlays"), overlays);
    for (int row = 0; row < presetsTable->rowCount(); ++row) {
        QJsonObject preset;
        preset.insert(QStringLiteral("id"), tableText(presetsTable, row, 0).toInt());
        if (!tableText(presetsTable, row, 1).isEmpty()) {
            preset.insert(QStringLiteral("name"), tableText(presetsTable, row, 1));
        }
        preset.insert(QStringLiteral("position"), vectorArray(tableText(presetsTable, row, 2), tableText(presetsTable, row, 3), tableText(presetsTable, row, 4)));
        bool hasViewCenter = false;
        for (int column = 5; column <= 7; ++column) {
            hasViewCenter = hasViewCenter || !tableText(presetsTable, row, column).isEmpty();
        }
        if (hasViewCenter) {
            preset.insert(QStringLiteral("viewCenter"), vectorArray(tableText(presetsTable, row, 5), tableText(presetsTable, row, 6), tableText(presetsTable, row, 7)));
        }
        preset.insert(QStringLiteral("upVector"), vectorArray(tableText(presetsTable, row, 8), tableText(presetsTable, row, 9), tableText(presetsTable, row, 10)));
        preset.insert(QStringLiteral("yaw"), tableText(presetsTable, row, 11).isEmpty() ? 0.0 : tableText(presetsTable, row, 11).toDouble());
        preset.insert(QStringLiteral("pitch"), tableText(presetsTable, row, 12).isEmpty() ? 0.0 : tableText(presetsTable, row, 12).toDouble());
        preset.insert(QStringLiteral("fov"), tableText(presetsTable, row, 13).isEmpty() ? 45.0 : tableText(presetsTable, row, 13).toDouble());
        if (!tableText(presetsTable, row, 14).isEmpty()) {
            preset.insert(QStringLiteral("snapshot"), tableText(presetsTable, row, 14));
        }
        QJsonArray presetOverlays;
        foreach (const QString &overlayId, presetOverlayIds(row)) {
            presetOverlays.append(overlayId);
        }
        preset.insert(QStringLiteral("overlays"), presetOverlays);
        presets.append(preset);
    }
    root.insert(QStringLiteral("cameraPresets"), presets);
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
    markChanged();
}

void ca3DConfigDialog::removeObjectRow()
{
    if (objectsTable->currentRow() >= 0) {
        objectsTable->removeRow(objectsTable->currentRow());
        markChanged();
    }
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
    markChanged();
}

void ca3DConfigDialog::removeBindingRow()
{
    if (bindingsTable->currentRow() >= 0) {
        bindingsTable->removeRow(bindingsTable->currentRow());
        markChanged();
    }
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
    setTableCheck(overlaysTable, row, 16, true);
    markChanged();
}

void ca3DConfigDialog::removeOverlayRow()
{
    if (overlaysTable->currentRow() >= 0) {
        overlaysTable->removeRow(overlaysTable->currentRow());
        markChanged();
    }
}

void ca3DConfigDialog::addPresetRow()
{
    int nextId = 1;
    for (int existingRow = 0; existingRow < presetsTable->rowCount(); ++existingRow) {
        nextId = qMax(nextId, tableText(presetsTable, existingRow, 0).toInt() + 1);
    }

    const int row = presetsTable->rowCount();
    presetsTable->insertRow(row);
    for (int column = 0; column < presetsTable->columnCount(); ++column) {
        setTableText(presetsTable, row, column, QString());
    }
    setTableText(presetsTable, row, 0, QString::number(nextId));
    for (int column = 2; column <= 4; ++column) {
        setTableText(presetsTable, row, column, QStringLiteral("0"));
    }
    setTableText(presetsTable, row, 8, QStringLiteral("0"));
    setTableText(presetsTable, row, 9, QStringLiteral("1"));
    setTableText(presetsTable, row, 10, QStringLiteral("0"));
    setTableText(presetsTable, row, 11, QStringLiteral("0"));
    setTableText(presetsTable, row, 12, QStringLiteral("0"));
    setTableText(presetsTable, row, 13, QStringLiteral("45"));
    setPresetOverlaySelector(row, QStringList());
    markChanged();
}

void ca3DConfigDialog::removePresetRow()
{
    if (presetsTable->currentRow() >= 0) {
        presetsTable->removeRow(presetsTable->currentRow());
        markChanged();
    }
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
        markChanged();
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
    const QString formFileName = panelFileName(widget3D);
    const QFileInfo formFileInfo(formFileName);
    const QString panelName = safeFileNameComponent(formFileInfo.completeBaseName(), QStringLiteral("panel"));
    const QString widgetName = safeFileNameComponent(widget3D ? widget3D->objectName() : QString(), QStringLiteral("ca3dwidget"));
    const QString defaultName = QStringLiteral("%1_%2_preset_%3.png").arg(panelName, widgetName).arg(preset);
    const QString initialDirectory = formFileInfo.absolutePath().isEmpty()
                                     ? QDir::currentPath()
                                     : formFileInfo.absolutePath();

    const QString displayPath = QString::fromLocal8Bit(qgetenv("CAQTDM_DISPLAY_PATH"));
    QMessageBox::information(this,
                             tr("Snapshot Location"),
                             tr("Save the snapshot in a directory listed in CAQTDM_DISPLAY_PATH or in the panel's runtime working directory. "
                                "This allows sceneConfig to use a portable relative path instead of an absolute path.\n\n"
                                "Current working directory: %1\nCAQTDM_DISPLAY_PATH: %2")
                             .arg(QDir::currentPath(), displayPath.isEmpty() ? tr("not set") : displayPath));
    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          tr("Save 3D Snapshot"),
                                                          QDir(initialDirectory).filePath(defaultName),
                                                          tr("PNG Images (*.png)"));
    if (fileName.isEmpty()) {
        return;
    }

    pendingSnapshotFileName = fileName;
    pendingSnapshotConfigPath = relativeSnapshotPath(fileName, formFileInfo.absolutePath());
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
        pendingSnapshotConfigPath.clear();
        pendingSnapshotPreset = 0;
    }
}

void ca3DConfigDialog::finishSnapshotCapture(const QPixmap &snapshot)
{
    const QString fileName = pendingSnapshotFileName;
    const QString configPath = pendingSnapshotConfigPath;
    const int preset = pendingSnapshotPreset;
    pendingSnapshotFileName.clear();
    pendingSnapshotConfigPath.clear();
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
                presetObject.insert(QStringLiteral("snapshot"), configPath);
                presets.replace(i, presetObject);
                break;
            }
        }
        root.insert(QStringLiteral("cameraPresets"), presets);
        const QString json = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
        rawJsonEdit->setPlainText(json);
        populateTablesFromJson(json);
        populatePresetSelector(json);
        markChanged();
    }

    showErrors(QStringList() << tr("Saved 3D background snapshot without overlays: %1").arg(fileName));
}

void ca3DConfigDialog::failSnapshotCapture(const QString &error)
{
    pendingSnapshotFileName.clear();
    pendingSnapshotConfigPath.clear();
    pendingSnapshotPreset = 0;
    if (captureSnapshotButton) {
        captureSnapshotButton->setEnabled(true);
    }
    showErrors(QStringList() << error);
}

void ca3DConfigDialog::validateRawJson()
{
    QStringList errors;
    showErrors(QStringList());
    if (validateJsonSyntax(rawJsonEdit->toPlainText(), &errors)) {
        rawValidationLabel->setText(tr("JSON is valid"));
        rawValidationLabel->setToolTip(QString());
        populateTablesFromJson(rawJsonEdit->toPlainText());
        populatePresetSelector(rawJsonEdit->toPlainText());
    } else {
        const QString errorText = errors.join(QStringLiteral("; "));
        rawValidationLabel->setText(errorText);
        rawValidationLabel->setToolTip(errorText);
    }
}

void ca3DConfigDialog::applyChanges()
{
    const QString json = currentEditorJson();
    QStringList errors;
    if (!validateJsonSyntax(json, &errors)) {
        showErrors(errors);
        return;
    }

    ca3DSceneConfig config;
    QStringList warnings;
    ca3DConfigParser::parse(json, &config, &warnings);
    showErrors(warnings);

    if (widget3D) {
        if (QDesignerFormWindowInterface *formWindow = QDesignerFormWindowInterface::findFormWindow(widget3D)) {
            formWindow->cursor()->setProperty("sceneConfig", json);
        } else {
            widget3D->setSceneConfig(json);
        }
    }
    updatingUi = true;
    rawJsonEdit->setPlainText(json);
    populateTablesFromJson(json);
    populatePresetSelector(json);
    updatingUi = false;
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void ca3DConfigDialog::markChanged()
{
    if (!updatingUi && buttonBox) {
        if (sender() != rawJsonEdit) {
            updateRawJsonFromTables();
        }
        if (rawValidationLabel) {
            rawValidationLabel->clear();
            rawValidationLabel->setToolTip(QString());
        }
        showErrors(QStringList());
        buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
    }
}

void ca3DConfigDialog::updateRawJsonFromTables()
{
    if (!rawJsonEdit) {
        return;
    }
    QScopedValueRollback<bool> updatingGuard(updatingUi, true);
    rawJsonEdit->setPlainText(jsonFromTables());
}

void ca3DConfigDialog::accept()
{
    applyChanges();
    QStringList errors;
    if (validateJsonSyntax(currentEditorJson(), &errors)) {
        QDialog::accept();
    }
}

bool ca3DConfigDialog::validateJsonSyntax(const QString &json, QStringList *errors, QJsonObject *root)
{
    if (errors) {
        errors->clear();
    }
    if (root) {
        *root = QJsonObject();
    }

    if (json.trimmed().isEmpty()) {
        if (root) {
            *root = QJsonObject();
        }
        return true;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errors) {
            const QByteArray utf8 = json.toUtf8();
            int line = 1;
            int column = 1;
            const int boundedOffset = qBound(0, parseError.offset, utf8.size());
            for (int i = 0; i < boundedOffset; ++i) {
                if (utf8.at(i) == '\n') {
                    ++line;
                    column = 1;
                } else {
                    ++column;
                }
            }
            errors->append(QStringLiteral("Invalid sceneConfig JSON at line %1, character %2: %3")
                           .arg(line)
                           .arg(column)
                           .arg(parseError.errorString()));
        }
        return false;
    }

    if (root) {
        *root = document.object();
    }
    return true;
}

bool ca3DConfigDialog::validateJson(const QString &json, QStringList *errors)
{
    if (!validateJsonSyntax(json, errors)) {
        return false;
    }
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
    connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(markChanged()));
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
    connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(markChanged()));
}

bool ca3DConfigDialog::tableCheck(QTableWidget *table, int row, int column) const
{
    QCheckBox *checkBox = table ? qobject_cast<QCheckBox *>(table->cellWidget(row, column)) : Q_NULLPTR;
    return checkBox && checkBox->isChecked();
}

void ca3DConfigDialog::setPresetOverlaySelector(int row, const QStringList &selectedOverlayIds)
{
    QWidget *selector = new QWidget(presetsTable);
    QHBoxLayout *layout = new QHBoxLayout(selector);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    QLineEdit *summary = new QLineEdit(selector);
    summary->setObjectName(QStringLiteral("presetOverlaySelection"));
    summary->setReadOnly(true);
    summary->setPlaceholderText(tr("No overlays selected"));
    summary->setText(selectedOverlayIds.join(QStringLiteral(", ")));
    QPushButton *selectButton = new QPushButton(tr("Select..."), selector);
    layout->addWidget(summary, 1);
    layout->addWidget(selectButton);
    selector->setMinimumWidth(260);
    presetsTable->setCellWidget(row, 15, selector);

    connect(summary, SIGNAL(textChanged(QString)), this, SLOT(markChanged()));
    connect(selectButton, &QPushButton::clicked, this, [this, summary]() {
        QDialog dialog(this);
        dialog.setWindowTitle(tr("Select Preset Overlays"));
        QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
        QListWidget *overlayList = new QListWidget(&dialog);

        QStringList availableOverlayIds;
        for (int overlayRow = 0; overlayRow < overlaysTable->rowCount(); ++overlayRow) {
            const QString overlayId = tableText(overlaysTable, overlayRow, 0);
            if (!overlayId.isEmpty() && !availableOverlayIds.contains(overlayId)) {
                availableOverlayIds.append(overlayId);
            }
        }
        QStringList selectedIds;
        const QStringList displayedIds = summary->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString &displayedId : displayedIds) {
            const QString selectedId = displayedId.trimmed();
            if (!selectedId.isEmpty()) {
                selectedIds.append(selectedId);
            }
            if (!selectedId.isEmpty() && !availableOverlayIds.contains(selectedId)) {
                availableOverlayIds.append(selectedId);
            }
        }
        for (const QString &overlayId : availableOverlayIds) {
            QListWidgetItem *item = new QListWidgetItem(overlayId, overlayList);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(selectedIds.contains(overlayId) ? Qt::Checked : Qt::Unchecked);
        }

        QDialogButtonBox *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        dialogLayout->addWidget(overlayList);
        dialogLayout->addWidget(dialogButtons);
        connect(dialogButtons, SIGNAL(accepted()), &dialog, SLOT(accept()));
        connect(dialogButtons, SIGNAL(rejected()), &dialog, SLOT(reject()));
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        QStringList checkedIds;
        for (int itemIndex = 0; itemIndex < overlayList->count(); ++itemIndex) {
            QListWidgetItem *item = overlayList->item(itemIndex);
            if (item->checkState() == Qt::Checked) {
                checkedIds.append(item->text());
            }
        }
        summary->setText(checkedIds.join(QStringLiteral(", ")));
    });
}

QStringList ca3DConfigDialog::presetOverlayIds(int row) const
{
    QWidget *selector = presetsTable ? presetsTable->cellWidget(row, 15) : Q_NULLPTR;
    QLineEdit *summary = selector ? selector->findChild<QLineEdit *>(QStringLiteral("presetOverlaySelection")) : Q_NULLPTR;
    QStringList result;
    if (!summary) {
        return result;
    }
    const QStringList ids = summary->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &id : ids) {
        const QString trimmedId = id.trimmed();
        if (!trimmedId.isEmpty()) {
            result.append(trimmedId);
        }
    }
    return result;
}

void ca3DConfigDialog::showErrors(const QStringList &errors)
{
    if (errors.isEmpty()) {
        errorLabel->clear();
        errorLabel->hide();
    } else if (errors.count() == 1 && errors.first() == tr("JSON is valid")) {
        errorLabel->setText(errors.first());
        errorLabel->show();
    } else {
        errorLabel->setText(errors.join(QStringLiteral("\n")));
        errorLabel->show();
    }
}
