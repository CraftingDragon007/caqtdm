#include "cahmiconfig.h"
#include "qapplication.h"

#include <QPainter>

caHMIConfig::caHMIConfig(QWidget *parent)
    : QWidget{parent}
{
    setFocusPolicy(Qt::StrongFocus);

    this->thisUUID = QUuid::createUuid();
}

void caHMIConfig::setOutputA(const QString &outputA) {
    this->thisOutputA = outputA;
}
QString caHMIConfig::outputA() const {
    return this->thisOutputA;
}

void caHMIConfig::setOutputB(const QString &outputB) {
    this->thisOutputB = outputB;
}
QString caHMIConfig::outputB() const {
    return this->thisOutputB;
}

void caHMIConfig::setChannel(const QString &channel) {
    this->thisChannelA = channel;
}
QString caHMIConfig::channel() const {
    return this->thisChannelA;
}

void caHMIConfig::setChannelB(const QString &channel) {
    this->thisChannelB = channel;
}
QString caHMIConfig::channelB() const {
    return this->thisChannelB;
}

void caHMIConfig::setChannelC(const QString &channel) {
    this->thisChannelC = channel;
}
QString caHMIConfig::channelC() const {
    return this->thisChannelC;
}

void caHMIConfig::setChannelD(const QString &channel) {
    this->thisChannelD = channel;
}
QString caHMIConfig::channelD() const {
    return this->thisChannelD;
}

void caHMIConfig::setShortcut(const QKeySequence &shortcut) {
    this->thisShortcut = shortcut;
}

QKeySequence caHMIConfig::shortcut() const {
    return this->thisShortcut;
}

void caHMIConfig::setValue(const QString &value) {
    this->thisValue = value;
}

QString caHMIConfig::value() const {
    return this->thisValue.toString();
}

void caHMIConfig::setCalculationType(const calcType &calclationType) {
    this->thisCalculationType = calclationType;
}

caHMIConfig::calcType caHMIConfig::calculationType() const {
    return this->thisCalculationType;
}

void caHMIConfig::setCaptureType(const capType &captureType) {
    this->thisCaptureType = captureType;
}

caHMIConfig::capType caHMIConfig::captureType() const {
    return this->thisCaptureType;
}

void caHMIConfig::setCaptureRange(const capRange &captureRange){
    this->thisCaptureRange = captureRange;
}

caHMIConfig::capRange caHMIConfig::captureRange() const {
    return this->thisCaptureRange;
}

void caHMIConfig::setMouseRectSize(const QSize &rectSize){
    this->thisMouseRectSize = rectSize;
}

QSize caHMIConfig::mouseRectSize() const {
    return this->thisMouseRectSize;
}


QString caHMIConfig::uuid() const {
    return this->thisUUID.toString();
}

bool caHMIConfig::isDesignerMode() {
    return qApp->property("APP_SOURCE") == QString("DESIGNER");
}

void caHMIConfig::paintEvent(QPaintEvent *ev) {
    QWidget::paintEvent(ev);
    if (this->isDesignerMode()) {
        QPainter p(this);
        p.setPen(Qt::white);
        p.fillRect(rect(), Qt::blue);
        QString text;
        QKeySequence key = QKeySequence(this->thisShortcut);
        if (!key.isEmpty()) {
            text = key.toString();
        }
        if (text.simplified().isEmpty()){
            text = this->objectName();
        }
        p.drawText(rect(), Qt::AlignCenter, text);
    }
}
