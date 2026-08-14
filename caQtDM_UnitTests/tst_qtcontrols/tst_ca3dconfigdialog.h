/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef TST_CA3DCONFIGDIALOG_H
#define TST_CA3DCONFIGDIALOG_H

#include <QByteArray>
#include <QObject>

class TestCa3DConfigDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void appliesStructuredOverlayChanges();
    void roundTripsObjectMasterLinks();
    void keepsNewRowsWhenValidatingRawJson();
    void allowsApplyingWithMissingOverlayFile();

private:
    QByteArray previousForceFallbackValue;
    bool hadForceFallbackValue = false;
};

#endif
