/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2010 - 2021
 *
 *  Author:
 *    Anton Mezger
 *  Contact details:
 *    anton.mezger@psi.ch
 */

#include <QtGui>
#include <QtDesigner/QtDesigner>
#include "pvdialog.h"
#include <QJsonDocument>
#include <iostream>

void PVDialog::print_out(const wchar_t* output)
{
    std::wcout << output << "\n";
    std::wcout.flush();
}

PVDialog::PVDialog(QWidget *tic, QWidget *parent) : QDialog(parent)
{
    QString stylesheet("QDialog {background: rgb(154,192,205);} \
                       QLineEdit {color: black; background: rgb(255, 255, 127)};");
    QString PV("");
    QString trimmedPV("");
    QString dbndType("");
    QString syncKey("");
    QString errorMessage("");

    bool deadbandPresent = false;
    bool displayratePresent = false;
    bool arrayPresent = false;
    bool syncPresent = false;
    bool tsPresent = false;
    bool decPresent = false;

    bool decValueOK = false;
    int decValueDecoded = 1;
    bool rateValueOK = false;
    int rateValueDecoded = 1;
    bool dbndValueOK = false;
    double dbndValueDecoded = 0;
    bool sValueOK = false;
    int sValueDecoded = 0;
    bool iValueOK = false;
    int iValueDecoded = 1;
    bool eValueOK = false;
    int eValueDecoded = -1;

    int syncIndex = -1;
    QString syncValue("");

    int pos;
    QString prefix("");
    QStringList plugins;
    plugins <<""<<"epics3"<<"epics4"<<"bsread"<<"environment"<<"modbus"<<"gps"<<"archiveHTTP"<<"opcua";

    if(caLed *w = qobject_cast<caLed *>(tic)) PV = w->getPV();
    else if (caLinearGauge *w = qobject_cast<caLinearGauge*>(tic)) PV = w->getPV();
    else if (caCircularGauge *w = qobject_cast<caCircularGauge*>(tic)) PV = w->getPV();
    else if (caMeter *w = qobject_cast<caMeter*>(tic)) PV = w->getPV();
    else if(caLineEdit *w = qobject_cast<caLineEdit *>(tic)) PV = w->getPV();
    else if(caMultiLineString *w = qobject_cast<caMultiLineString *>(tic)) PV = w->getPV();
    else if (caThermo *w = qobject_cast<caThermo*>(tic)) PV = w->getPV();
    else if (caByte *w = qobject_cast<caByte*>(tic)) PV = w->getPV();
    else if (caWaveTable *w = qobject_cast<caWaveTable*>(tic)) PV = w->getPV();
    else if (caCamera *w = qobject_cast<caCamera*>(tic)) PV = w->getPV_Data();
    else if (caWaterfallPlot *w = qobject_cast<caWaterfallPlot*>(tic)) PV = w->getPV();
    else if(caLineDraw *w = qobject_cast<caLineDraw *>(tic)) PV = w->getPV();

    else if (caNumeric *w = qobject_cast<caNumeric*>(tic)) PV = w->getPV();
    else if (caApplyNumeric *w = qobject_cast<caApplyNumeric*>(tic)) PV = w->getPV();
    else if (caSlider *w = qobject_cast<caSlider*>(tic)) PV = w->getPV();
    else if (caMenu *w = qobject_cast<caMenu*>(tic)) PV = w->getPV();
    else if (caChoice *w = qobject_cast<caChoice*>(tic)) PV = w->getPV();
    else if (caTextEntry *w = qobject_cast<caTextEntry*>(tic)) PV = w->getPV();
    else if (caMessageButton *w = qobject_cast<caMessageButton*>(tic)) PV = w->getPV();
    else if (caToggleButton *w = qobject_cast<caToggleButton*>(tic)) PV = w->getPV();
    else if (caSpinbox *w = qobject_cast<caSpinbox*>(tic)) PV = w->getPV();
    else if (caByteController *w = qobject_cast<caByteController*>(tic)) PV = w->getPV();

    else return;

    entry = tic;

    trimmedPV = PV;

    pos = PV.indexOf("://");
    if(pos != -1) {
        prefix = PV.mid(0, pos);
        PV = PV.mid(pos+3);
        trimmedPV = PV;
    }

    //std::cerr << "\nPV string " << qasc(PV) << "\n";

    // separate pv and filters, then parse filters
    pos = PV.indexOf(".{");
    if(pos != -1) {
        QString JSONString = PV.mid(pos+1);
        trimmedPV = PV.mid(0, pos);
        //std::cerr << "jsonstring=" << qasc(JSONString) << "\n";

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(JSONString.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            //print_out(L"failed to parse");
            errorMessage = "could not parse entered expression, got error:" + parseError.errorString();
        } else if (!document.isObject()) {
            //print_out(L"code is not an object");
            errorMessage = "could not parse entered expression, isn't json object";
        } else {
            //print_out(L"\nMain object:");
            //print_out(main_object->Stringify().c_str());

            // Fetch the keys
            QJsonObject main_object = document.object();
            QStringList keys = main_object.keys();
            QStringList::iterator iter = keys.begin();
            while (iter != keys.end()) {
                QString key = *iter;
                QJsonValue value = main_object[key];
                //std::cerr <<  "Key: " << qasc(key) << " Value. " << qasc(value) << "\n";

                // get maxdisplayrate
                if(value.isObject() && value.toObject().contains("maxdisplayrate")) {
                    displayratePresent = true;
                    QJsonObject root = value.toObject();
                    if (root["maxdisplayrate"].isDouble()) {;
                        //std::cerr << "maxdisplayrate detected\n";
                        rateValueDecoded = root["maxdisplayrate"].toDouble();
                        rateValueOK = true;
                        //std::cerr << "decode value=" << rateValueOK << " " << rateValueDecoded << "\n";
                    } else {
                        errorMessage = "could not get ratevalue";
                    }
                }

                // get deadband value
                if(key.contains("dbnd") && value.isObject()) {
                    deadbandPresent = true;
                    QJsonObject root = value.toObject();
                    if (root["abs"].isDouble())  {
                        //std::cerr << "abs detected " << root[L"abs"]->Stringify().c_str() << "\n";
                        dbndType = "abs";
                        dbndValueDecoded = root["abs"].toDouble();
                        dbndValueOK = true;
                        //std::cerr << "decode value=" << dbndValueOK << " " << dbndValueDecoded << "\n";
                    } else if(root["rel"].isDouble()) {
                        //std::cerr << "rel detected\n";
                        dbndType = "rel";
                        dbndValueDecoded = root["rel"].toDouble();
                        dbndValueOK = true;
                        //std::cerr << "decode value=" << dbndValueOK << " " << dbndValueDecoded << "\n";
                    } else {
                        errorMessage = "could not get dbndvalue";
                    }
                }

                // get decimation, do not parse with json, here we have a simple case
                if(key.contains("dec") && value.isObject()) {
                    decPresent = true;
                    QJsonObject root = value.toObject();
                    if (root["n"].isDouble()) {
                        // std::cerr << "dec detected\n";
                        decValueDecoded = root["n"].toDouble();
                        decValueOK = true;
                        //std::cerr << "decode value=" << decValueOK << " " << decValueDecoded << "\n";
                    } else {
                        errorMessage = "could not get decvalue";
                    }
                }

                // get sync data
                if(key.contains("sync") && value.isObject()) {  // camonitor 'ACM:COUNT:1.{"sync":{"while":"blue"}}’
                    //std::cerr <<  "Key: " << qasc(QKey) << " Value. " << qasc(QValue) << "\n";
                    syncPresent = true;
                    QJsonObject root = value.toObject();
                    // iterate for next objects
                    QStringList keys = root.keys();
                    QStringList::iterator iter1 = keys.begin();
                    while (iter1 != keys.end()) {
                        QString key = *iter1;
                        //std::cerr << "Key: " << qasc(QKey) << "\n";
                        QStringList choices;
                        choices <<"before"<<"first"<<"while"<<"last"<<"after"<<"unless";
                        if((syncIndex = choices.indexOf(key)) != -1) {
                           //std::cerr << "index in QKey: " << syncIndex << "\n";
                        } else {
                           //std::cerr << "index in QKey not found " << qasc(QKey )<< "\n";
                           errorMessage = "could not find valid keyword for sync";
                        }

                        // Get the key's value.
                        QJsonValue value = root[key];
                        if (value.isString()) {
                            syncValue = value.toString();
                            //std::cerr << "Value: " << qasc(syncValue) << "\n";
                         }
                        // Next key.
                        iter1++;
                        break;
                    }
                }


                // get ts
                if(key.contains("ts")) {
                    tsPresent = true;
                }

                // get array data
                if(key.contains("arr") && value.isObject()) {
                    arrayPresent = true;
                    QJsonObject root = value.toObject();
                    QStringList keys = root.keys();
                    QStringList::iterator iter1 = keys.begin();
                    while (iter1 != keys.end()) {
                        QString key = *iter1;
                        //std::cerr << "Key: " << qasc(QKey);
                        // Get the key's value.
                        QJsonValue value = root[key];
                        QString stringValue;
                        if (value.isString()) {
                            stringValue = value.toString();
                        } else if (value.isDouble()) {
                            stringValue = QString::number(static_cast<int>(value.toDouble()));
                        }

                        if (!stringValue.isEmpty()) {
                            //std::cerr << "key: " << qasc(key) << " Value: " << qasc(stringValue) << "\n";
                            if(key.contains("s")) {
                                sValueDecoded = stringValue.toInt(&sValueOK);
                                if(!sValueOK) errorMessage = "could not get array svalue";;
                            } else if(key.contains("i")) {
                                iValueDecoded = stringValue.toInt(&iValueOK);
                                if(!iValueOK) errorMessage = "could not get array ivalue";
                            } else if(key.contains("e")) {
                                eValueDecoded = stringValue.toInt(&eValueOK);
                                if(!eValueOK) errorMessage = "could not get array evalue";
                            }
                         }
                        // Next key.
                        iter1++;
                    }
                }
                // Next key.
                iter++;
            }
        }
    }

    // define buttonbox
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(saveState()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGridLayout *mainLayout = new QGridLayout;

    autocompletionCheckBox = new QCheckBox;
    autocompletionCheckBox->setChecked(true);

    // channel
    pvLabel = new QLabel("PV:");
    pvLine = new TextEdit;
    pvLine->setAcceptRichText(false);
    pvLine->setLineWrapMode(QTextEdit::NoWrap);

    // error message
    msgLine = new QLineEdit;
    msgLine->setDisabled(true);

    QFontMetrics metrics(pvLine->font());
    int lineHeight = metrics.lineSpacing();
    pvLine->setFixedHeight(1.5*lineHeight);
    pvLine->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pvLine->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    completer = new NetworkCompleter(this);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(false);
    completer->setCompletionColumn(0);
    completer->setErrorWidget(msgLine);
    completer->setCompletionWidget(autocompletionCheckBox);
    pvLine->setCompleter(completer);

    mainLayout->addWidget(pvLabel, 0, 0);
    mainLayout->addWidget(pvLine, 0, 1, 1, 7);
    pvLine->setText(trimmedPV);

    // prefix
    prefixLabel = new QLabel("Plugin prefix:");
    remarkLabel = new QLabel("nul: epics3");
    prefixComboBox = new QComboBox;
    for(int i=0; i< plugins.size(); i++) prefixComboBox->addItem(plugins.at(i));
    mainLayout->addWidget(prefixLabel, 1, 0);;
    mainLayout->addWidget(prefixComboBox, 1, 3);
    mainLayout->addWidget(remarkLabel, 1, 4);
    if(prefix.size() > 0) {
        int indx = 0;
        if((indx = plugins.indexOf(prefix)) != -1) prefixComboBox->setCurrentIndex(indx);
    }
    prefixLabel->setToolTip(prefixToolTip);

    // deadband filter
    dbndLabel = new QLabel("Deadband Filter:");
    dbndCheckBox = new QCheckBox;
    dbndComboBox = new QComboBox;
    dbndComboBox->addItem("abs");
    dbndComboBox->addItem("rel");
    dbndDoubleValue = new QDoubleSpinBox;
    mainLayout->addWidget(dbndLabel, 2, 0);
    mainLayout->addWidget(dbndCheckBox, 2, 1);
    mainLayout->addWidget(dbndComboBox, 2, 3);
    mainLayout->addWidget(dbndDoubleValue, 2, 5);
    if(deadbandPresent) {
        dbndCheckBox->setChecked(true);
        if(dbndType.contains("abs")) dbndComboBox->setCurrentIndex(0);
        if(dbndType.contains("rel")) dbndComboBox->setCurrentIndex(1);
        if(dbndValueOK) dbndDoubleValue->setValue(dbndValueDecoded);
    }
    dbndLabel->setToolTip(dbndToolTip);

    // array filter
    arrayLabel = new QLabel("Array Filter:");
    arrayCheckBox = new QCheckBox;
    arrayLabel_s = new QLabel("s:");
    arrayLabel_i = new QLabel("i:");
    arrayLabel_e = new QLabel("e:");
    arrayIntValue_s = new QSpinBox;
    arrayIntValue_i = new QSpinBox;
    arrayIntValue_e = new QSpinBox;
    arrayIntValue_s->setMinimum(-1000);
    arrayIntValue_s->setMaximum(+1000);
    arrayIntValue_i->setMinimum(0);
    arrayIntValue_i->setMaximum(+1000);
    arrayIntValue_e->setMinimum(-1000);
    arrayIntValue_e->setMaximum(+1000);
    arrayIntValue_s->setValue(0);
    arrayIntValue_i->setValue(1);
    arrayIntValue_e->setValue(-1);
    mainLayout->addWidget(arrayLabel, 3, 0);
    mainLayout->addWidget(arrayCheckBox, 3, 1);
    mainLayout->addWidget(arrayLabel_s, 3, 2);
    mainLayout->addWidget(arrayLabel_i, 3, 4);
    mainLayout->addWidget(arrayLabel_e, 3, 6);
    mainLayout->addWidget(arrayIntValue_s, 3, 3);
    mainLayout->addWidget(arrayIntValue_i, 3, 5);
    mainLayout->addWidget(arrayIntValue_e, 3, 7);
    if(arrayPresent) {
       arrayCheckBox->setChecked(true);
       if(sValueOK) arrayIntValue_s->setValue(sValueDecoded);
       if(iValueOK) arrayIntValue_i->setValue(iValueDecoded);
       if(eValueOK) arrayIntValue_e->setValue(eValueDecoded);
    }
    arrayLabel->setToolTip(arrayToolTip);

    // sync filter
    syncLabel = new QLabel("Sync Filter:");
    syncCheckBox = new QCheckBox;
    syncComboBox = new QComboBox;
    syncComboBox->addItem("before");
    syncComboBox->addItem("first");
    syncComboBox->addItem("while");
    syncComboBox->addItem("last");
    syncComboBox->addItem("after");
    syncComboBox->addItem("unless");
    syncLine = new QLineEdit;
    mainLayout->addWidget(syncLabel, 4, 0);
    mainLayout->addWidget(syncCheckBox, 4, 1);
    mainLayout->addWidget(syncComboBox, 4, 3);
    mainLayout->addWidget(syncLine, 4, 5, 1, 3);
    if(syncPresent) {
        syncCheckBox->setChecked(true);
        if(syncIndex > -1) syncComboBox->setCurrentIndex(syncIndex);
        syncLine->setText(syncValue);
    }
    syncLabel->setToolTip(syncToolTip);

    // decimation filter
    decLabel = new QLabel("Decimation:");
    decCheckBox = new QCheckBox;
    decIntValue = new QSpinBox;
    decIntValue->setMinimum(0);
    mainLayout->addWidget(decLabel, 5, 0);
    mainLayout->addWidget(decCheckBox, 5, 1);
    mainLayout->addWidget(decIntValue, 5, 3);
    if(decPresent) {
        decCheckBox->setChecked(true);
        if(decValueOK) decIntValue->setValue(decValueDecoded);
    }
    decLabel->setToolTip(decToolTip);

    // ts filter
    tsLabel = new QLabel("ts Filter:");
    tsCheckBox = new QCheckBox;
    mainLayout->addWidget(tsLabel, 6, 0);
    mainLayout->addWidget(tsCheckBox, 6, 1);
    if(tsPresent) {
        tsCheckBox->setChecked(true);
    }
    tsLabel->setToolTip(tsToolTip);

    // displayrate
    rateLabel = new QLabel("MaxdisplayRate:");
    rateCheckBox = new QCheckBox;
    rateIntValue = new QSpinBox;
    mainLayout->addWidget(rateLabel, 7, 0);
    mainLayout->addWidget(rateCheckBox, 7, 1);
    mainLayout->addWidget(rateIntValue, 7, 3);
    rateIntValue->setMinimum(1);
    rateIntValue->setMaximum(100);
    if(displayratePresent) {
       rateCheckBox->setChecked(true);
       if(rateValueOK) rateIntValue->setValue(rateValueDecoded);
    }
    rateLabel->setToolTip(rateToolTip);

    // autocompletion checkbox
    autoLabel = new QLabel("pv autocompletion:");
    mainLayout->addWidget(autoLabel, 9, 0);
    mainLayout->addWidget(autocompletionCheckBox, 9, 1);

    // error message
    mainLayout->addWidget(msgLine, 8,0,1,8);
    msgLine->setText(errorMessage);

    // buttonbox
    mainLayout->addWidget(buttonBox, 9, 2, 2, 6);

    setLayout(mainLayout);
    setWindowTitle(tr("Edit PV"));
    setStyleSheet(stylesheet);
    int dialogWidth  = tic->width()+40;
    int dialogHeight = tic->height()+80;
    if(dialogWidth < 500) dialogWidth = 500;
    if(dialogHeight < 350) dialogHeight = 350;
    setFixedWidth(dialogWidth);
    setFixedHeight(dialogHeight);
}

QSize PVDialog::sizeHint() const
{
    return QSize(250, 250);
}

void PVDialog::saveState()
{
    if (QDesignerFormWindowInterface *formWindow = QDesignerFormWindowInterface::findFormWindow(
            entry)) {
        QString channel("");
        QString pv = pvLine->toPlainText();
        QString prefix = prefixComboBox->currentText();
        QJsonObject root;
        bool B_dbnd = dbndCheckBox->isChecked();
        bool B_rate = rateCheckBox->isChecked();
        bool B_array = arrayCheckBox->isChecked();
        bool B_sync = syncCheckBox->isChecked();
        bool B_ts = tsCheckBox->isChecked();
        bool B_dec = decCheckBox->isChecked();
        if (B_dbnd) {
            root["dbnd"] = QJsonObject{{dbndComboBox->currentText(), dbndDoubleValue->value()}};
        }
        if (B_rate) {
            root["caqtdm_monitor"] = QJsonObject{{"maxdisplayrate", rateIntValue->value()}};
        }
        if (B_dec) {
            root["dec"] = QJsonObject{{"n", decIntValue->value()}};
        }
        if (B_array) {
            root["arr"] = QJsonObject{{"s", arrayIntValue_s->value()},
                                       {"i", arrayIntValue_i->value()},
                                       {"e", arrayIntValue_e->value()}};
        }
        if (B_sync) {
            root["sync"] = QJsonObject{{syncComboBox->currentText(), syncLine->text()}};
        }
        if (B_ts) {
            root["ts"] = QJsonObject();
        }
        if (pv.size() > 0) {
            if (prefix.size() > 0) {
                channel = prefix + "://" + pv;
            } else {
                channel = pv;
            }
            if (B_dbnd || B_rate || B_array || B_sync || B_ts || B_dec) {
                QString strng = QJsonDocument(root).toJson(QJsonDocument::Compact);
                channel.append(".");
                channel.append(strng);
            }
        }
        if (caCamera *w = qobject_cast<caCamera *>(entry)) {
            Q_UNUSED(w);
            formWindow->cursor()->setProperty("channelData", channel);
        } else {
            formWindow->cursor()->setProperty("channel", channel);
        }
    }
    accept();
}
