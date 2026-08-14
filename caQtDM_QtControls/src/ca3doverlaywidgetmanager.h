#ifndef CA3DOVERLAYWIDGETMANAGER_H
#define CA3DOVERLAYWIDGETMANAGER_H

#include <QImage>
#include <QEvent>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRegion>
#include <QVector>
#include <QWidget>

#include <qtcontrols_global.h>

class QKeyEvent;

class QTCON_EXPORT ca3DOverlayWidgetManager : public QWidget
{
    Q_OBJECT

public:
    explicit ca3DOverlayWidgetManager(QWidget *parent = Q_NULLPTR);
    ~ca3DOverlayWidgetManager();

    void loadWidgetsFromUi(const QString &uiFilePath);
    QWidget *contentRoot() const { return thisContentRoot; }
    QSize sourceDesignSize() const { return thisSourceDesignSize; }
    QImage renderSnapshot(qreal scale) const;
    bool sendMouseEvent(const QPointF &designPosition, QEvent::Type type, Qt::MouseButton button,
                        Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    bool sendContextMenuEvent(const QPointF &designPosition, const QPoint &globalPosition,
                              Qt::KeyboardModifiers modifiers);
    bool sendKeyEvent(QKeyEvent *event);
    bool hasFocusedTextInput() const;
    bool takeTextureDirty();
    void markTextureDirty();
    void clearOverlayFocus();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *targetWidgetAt(const QPointF &designPosition) const;
    void installDirtyTracking(QWidget *widget);

    QWidget *thisLoadedWidget;
    QWidget *thisContentRoot;
    QWidget *thisFocusedOverlayWidget;
    QWidget *thisMouseCaptureWidget;
    QSize thisSourceDesignSize;
    mutable bool thisRenderingSnapshot;
    bool thisTextureDirty;
};

#endif
