#include "cahmiconfigtransferitem.h"

caHMIConfigTransferItem::caHMIConfigTransferItem(QObject *parent)
    : QObject{parent}
{}

QSharedPointer<caHMIConfigTransferItem> caHMIConfigTransferItem::clone() const
{
    auto clone = QSharedPointer<caHMIConfigTransferItem>::create();
    clone->setUUID(uuid());
    clone->setPID(pid());
    clone->setEnabled(enabled());
    clone->setObjectName(objectName());
    clone->setFileName(fileName());
    clone->setOutputA(outputA());
    clone->setOutputB(outputB());
    clone->setChannel(channel());
    clone->setChannelB(channelB());
    clone->setChannelC(channelC());
    clone->setChannelD(channelD());
    clone->setShortcut(shortcut());
    clone->setValue(value());
    clone->setCalculationType(calculationType());
    clone->setCaptureType(captureType());
    clone->setCaptureRange(captureRange());
    clone->setTimestamp(timestamp());

    return clone;
}

#include "moc_cahmiconfigtransferitem.cpp"