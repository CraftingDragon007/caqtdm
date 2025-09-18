#include "cahmiconfig.h"
#include "qapplication.h"

#include <QPainter>

caHMIConfig::caHMIConfig(QWidget *parent)
    : QWidget{parent}
{
    setFocusPolicy(Qt::StrongFocus);

    this->thisUUID = QUuid::createUuid();
}

void caHMIConfig::setShortcutFromSequence(const QKeySequence &key){
    qDebug() << "setShortcut called with << '" << key << "' (isDesignerMode:" << isDesignerMode() << ")";
    this->thisKey = key[0];
    update();
}

QKeySequence caHMIConfig::shortcutAsSequence() const {
    return * new QKeySequence(this->thisKey);
}

QKeyCombination caHMIConfig::shortcut() const {
    return this->thisKey;
}

void caHMIConfig::setChannel(const QString &channel){
    this->thisChannel = channel;
}

QString caHMIConfig::channel() const {
    return this->thisChannel;
}

void caHMIConfig::setValue(const QString &value){
    this->thisValue = value;
}

QString caHMIConfig::value() const {
    return this->thisValue.toString();
}

void caHMIConfig::setCalculationType(const calcType &calclationType){
    this->thisCalculationType = calclationType;
}

caHMIConfig::calcType caHMIConfig::calculationType() const {
    return this->thisCalculationType;
}

void caHMIConfig::setCaptureType(const capType &captureType){
    this->thisCaptureType = captureType;
}

caHMIConfig::capType caHMIConfig::captureType() const {
    return this->thisCaptureType;
}

QString caHMIConfig::uuid() const {
    return this->thisUUID.toString();
}

bool caHMIConfig::isDesignerMode(){
    return qApp->property("APP_SOURCE") == QString("DESIGNER");
}

void caHMIConfig::paintEvent(QPaintEvent *ev){
    QWidget::paintEvent(ev);
    if (this->isDesignerMode()){
        QPainter p(this);
        p.setPen(Qt::white);
        p.fillRect(rect(), Qt::blue);
        p.drawText(rect(), Qt::AlignCenter, QKeySequence(this->thisKey).toString());
    }
}
