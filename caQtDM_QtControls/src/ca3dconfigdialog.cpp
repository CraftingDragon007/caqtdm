/*
 *  This file is part of the caQtDM Framework.
 */

#include "ca3dconfigdialog.h"

#include "ca3dconfig.h"
#include "ca3dwidget.h"
#include "pvdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
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
}

ca3DConfigDialog::ca3DConfigDialog(ca3DWidget *widget, QWidget *parent)
    : QDialog(parent ? parent : widget)
    , widget3D(widget)
    , tabs(Q_NULLPTR)
    , objectsTable(Q_NULLPTR)
    , bindingsTable(Q_NULLPTR)
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
}

QString ca3DConfigDialog::jsonFromTables() const
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(rawJsonEdit->toPlainText().toUtf8(), &parseError);
    QJsonObject root = parseError.error == QJsonParseError::NoError && document.isObject()
                       ? document.object()
                       : QJsonObject();
    QJsonArray objects;
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

void ca3DConfigDialog::validateRawJson()
{
    QStringList errors;
    if (validateJson(rawJsonEdit->toPlainText(), &errors)) {
        showErrors(QStringList() << tr("JSON is valid"));
        populateTablesFromJson(rawJsonEdit->toPlainText());
    } else {
        showErrors(errors);
    }
}

void ca3DConfigDialog::applyChanges()
{
    const QString json = tabs->currentWidget() == rawJsonEdit->parentWidget() ? rawJsonEdit->toPlainText() : jsonFromTables();
    QStringList errors;
    if (!validateJson(json, &errors)) {
        showErrors(errors);
        return;
    }
    if (widget3D) {
        widget3D->setSceneConfig(json);
    }
    rawJsonEdit->setPlainText(json);
    populateTablesFromJson(json);
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
