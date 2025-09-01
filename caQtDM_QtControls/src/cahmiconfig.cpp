#include "cahmiconfig.h"
#include "qevent.h"
#include <iostream>
#include <ostream>

caHMIConfig::caHMIConfig(QWidget *parent)
    : QWidget{parent}
{
    setFocusPolicy(Qt::StrongFocus);

    globalEventFilter = new HMIApplicationEventFilter(this);

    // check if qApp isn't a nullptr (as it would be in qtdesigner)
    if (qApp){
        qApp->installEventFilter(globalEventFilter);
        connect(globalEventFilter, &HMIApplicationEventFilter::keyPressed, this, &caHMIConfig::handleKeyPressed);
        connect(globalEventFilter, &HMIApplicationEventFilter::mousePressed, this, &caHMIConfig::handleMousePressed);
    }
}

void caHMIConfig::setShortcut(const QKeySequence &key){
    this->thisKey = key[0];
}

QKeySequence caHMIConfig::shortcut() const {
    return * new QKeySequence(this->thisKey);
}

void caHMIConfig::setChannel(const QString &channel){
    this->thisChannel = channel;
}

QString caHMIConfig::channel() const {
    return this->thisChannel;
}
/*
void caHMIConfig::keyPressEvent(QKeyEvent *event) {
    /*printf("keyPressEvent() called \n");
    std::flush(std::cout);
    if (event) {
        //->processEvent(event);
    }
}

bool caHMIConfig::event(QEvent *event){
    /*printf("event() called \n");
    std::flush(std::cout);
    //return this->processEvent(event);
    return false;
}*/

void caHMIConfig::handleKeyPressed(QObject *target, QKeyEvent *event){
    processEvent(event);
}

void caHMIConfig::handleMousePressed(QObject *target, QMouseEvent *event){

}

bool caHMIConfig::processEvent(QEvent *event){
    if (event->type() != QEvent::Leave && event->type() != QEvent::Enter && event->type() != QEvent::MouseMove && event->type() != QEvent::WindowActivate && event->type() != QEvent::Paint&& event->type() != QEvent::WindowDeactivate && event->type() != QEvent::ToolTip){
        //printf("events \n");
        //std::flush(std::cout);
    }
    qDebug() << event->type();
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
        //QKeyCombination* combination = new QKeyCombination(qtModifiers, qtKey); -> this segfaults

        bool prev = false;
        if (previousInput){
            prev = true;
        }

        /*if (!prev || (combination->key() != this->previousInput->key())) {*/
            /*previousInput = combination;*/
            qDebug() << "Central event(): Key press " << keyEvent->text();
            if (/*combination->key()*/ qtKey == this->thisKey.key() && /*combination->keyboardModifiers()*/ qtModifiers == this->thisKey.keyboardModifiers()){
                printf("Correct key pressed!!!");
                std::flush(std::cout);
                int combinedKeyData = static_cast<int>(keyEvent->key()) | static_cast<int>(keyEvent->modifiers().toInt());
                int signal = 1;
                emit HMIConfigInputReceived(&signal);
                return true;
            }
        /*}*/
    }
    return QWidget::event(event);
}
/*
bool caHMIConfig::eventFilter(QObject *obj, QEvent *event){
    printf("eventFilter() called\n");
    std::flush(std::cout);
    return this->processEvent(event);
}*/
