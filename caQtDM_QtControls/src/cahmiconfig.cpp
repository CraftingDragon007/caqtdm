#include "cahmiconfig.h"
#include "qevent.h"

caHMIConfig::caHMIConfig(QWidget *parent)
    : QWidget{parent}
{
    setFocusPolicy(Qt::StrongFocus);

    this->thisUUID = QUuid::createUuid();
}

void caHMIConfig::setShortcutFromSequence(const QKeySequence &key){
    this->thisKey = key[0];
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

void caHMIConfig::handleKeyPressed(QObject *target, QKeyEvent *event){
    Q_UNUSED(target);
}

void caHMIConfig::handleMousePressed(QObject *target, QMouseEvent *event){
    Q_UNUSED(target);
}

void caHMIConfig::processEvent(QEvent *event){
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        qDebug() << "Central event(): Mouse press at" << mouseEvent->pos();
    } else if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);
        int key = keyEvent->key();
        Qt::Key qtKey = static_cast<Qt::Key>(key);
        int modifiers = keyEvent->modifiers();
        Qt::KeyboardModifiers qtModifiers = static_cast<Qt::KeyboardModifiers>(modifiers);
        qDebug() << "Key: " << key << " Modifiers: " << modifiers;
        qDebug() << "Central event(): Key press " << keyEvent->text();
        if (qtKey == this->thisKey.key() && qtModifiers == this->thisKey.keyboardModifiers()){
            qDebug() << "Correct key pressed!!!";
            int signal = 1;
            emit HMIConfigInputReceived(&signal);
        }
    }
}
