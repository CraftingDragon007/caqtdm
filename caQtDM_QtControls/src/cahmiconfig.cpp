#include "cahmiconfig.h"

caHMIConfig::caHMIConfig(QWidget *parent)
    : QWidget{parent}
{
}

void caHMIConfig::setShortcut(const QKeySequence &key){
    this->thisKey = key;
}

QKeySequence caHMIConfig::shortcut() const {
    return this->thisKey;
}

void caHMIConfig::setChannel(const QString &channel){
    this->thisChannel = channel;
}

QString caHMIConfig::channel() const {
    return this->thisChannel;
}
