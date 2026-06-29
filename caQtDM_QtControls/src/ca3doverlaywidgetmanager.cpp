#include "ca3doverlaywidgetmanager.h"

#include <QApplication>
#include <QChildEvent>
#include <QFile>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextEdit>
#include <QUiLoader>

namespace {
QWidget *textInputAncestor(QWidget *widget) {
    for (QWidget *current = widget; current; current = current->parentWidget()) {
        if (qobject_cast<QLineEdit *>(current) ||
            qobject_cast<QPlainTextEdit *>(current) ||
            qobject_cast<QTextEdit *>(current)) {
            return current;
        }
    }
    return Q_NULLPTR;
}

QWidget *scrollBarAncestor(QWidget *widget) {
    for (QWidget *current = widget; current; current = current->parentWidget()) {
        if (qobject_cast<QScrollBar *>(current)) {
            return current;
        }
    }
    return Q_NULLPTR;
}
} // namespace

ca3DOverlayWidgetManager::ca3DOverlayWidgetManager(QWidget *parent)
    : QWidget(parent), thisLoadedWidget(Q_NULLPTR), thisContentRoot(Q_NULLPTR),
    thisFocusedOverlayWidget(Q_NULLPTR), thisMouseCaptureWidget(Q_NULLPTR),
    thisRenderingSnapshot(false), thisTextureDirty(true) {
    setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

ca3DOverlayWidgetManager::~ca3DOverlayWidgetManager() {
    delete thisLoadedWidget;
}

void ca3DOverlayWidgetManager::loadWidgetsFromUi(const QString &uiFilePath) {
    delete thisLoadedWidget;
    thisLoadedWidget = Q_NULLPTR;
    thisContentRoot = Q_NULLPTR;
    thisFocusedOverlayWidget = Q_NULLPTR;
    thisMouseCaptureWidget = Q_NULLPTR;
    thisSourceDesignSize = QSize();
    thisTextureDirty = true;

    QFile file(uiFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QUiLoader loader;
    thisLoadedWidget = loader.load(&file);
    file.close();
    if (!thisLoadedWidget) {
        return;
    }

    thisLoadedWidget->setParent(this);

    thisContentRoot =
        thisLoadedWidget->findChild<QWidget *>(QStringLiteral("centralWidget"));
    if (!thisContentRoot) {
        thisContentRoot =
            thisLoadedWidget->findChild<QWidget *>(QStringLiteral("centralwidget"));
    }
    if (!thisContentRoot) {
        thisContentRoot = thisLoadedWidget;
    }

    thisLoadedWidget->ensurePolished();
    thisContentRoot->ensurePolished();

    // Use widget size from .ui file
    thisSourceDesignSize = thisLoadedWidget->size();
    if (thisSourceDesignSize.isEmpty()) {
        thisSourceDesignSize = QSize(320, 240);
    }

    thisLoadedWidget->resize(thisSourceDesignSize);
    thisContentRoot->setGeometry(QRect(QPoint(0, 0), thisSourceDesignSize));
    thisLoadedWidget->setAttribute(Qt::WA_DontShowOnScreen);
    thisLoadedWidget->show();
    installDirtyTracking(thisLoadedWidget);
    QApplication::processEvents();
}

QImage ca3DOverlayWidgetManager::renderSnapshot(qreal scale) const {
    const QSize imageSize =
        (thisSourceDesignSize * scale).expandedTo(QSize(1, 1));
    QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    if (!thisContentRoot) {
        return image;
    }

    QPainter targetPainter(&image);
    targetPainter.setRenderHint(QPainter::Antialiasing, true);
    targetPainter.setRenderHint(QPainter::TextAntialiasing, true);
    targetPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    targetPainter.scale(scale, scale);
    thisRenderingSnapshot = true;
    thisContentRoot->render(&targetPainter, QPoint(),
                            QRect(QPoint(0, 0), thisSourceDesignSize),
                            QWidget::DrawChildren);
    thisRenderingSnapshot = false;
    return image;
}

bool ca3DOverlayWidgetManager::eventFilter(QObject *watched, QEvent *event) {
    if (!event) {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::ChildAdded) {
        const auto *childEvent = static_cast<QChildEvent *>(event);
        QObject *child = childEvent->child();
        if (child && child->isWidgetType()) {
            auto *childWidget = static_cast<QWidget *>(child);
            installDirtyTracking(childWidget);
            markTextureDirty();
        }
    } else if (!thisRenderingSnapshot) {
        switch (event->type()) {
        case QEvent::UpdateRequest:
        case QEvent::LayoutRequest:
        case QEvent::Resize:
        case QEvent::Move:
        case QEvent::Show:
        case QEvent::Hide:
        case QEvent::EnabledChange:
        case QEvent::FontChange:
        case QEvent::PaletteChange:
        case QEvent::StyleChange:
        case QEvent::DynamicPropertyChange:
        case QEvent::ContentsRectChange:
            markTextureDirty();
            break;
        default:
            break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

bool ca3DOverlayWidgetManager::sendMouseEvent(const QPointF &designPosition,
                                              QEvent::Type type,
                                              Qt::MouseButton button,
                                              Qt::MouseButtons buttons,
                                              Qt::KeyboardModifiers modifiers) {
    if (!thisContentRoot ||
        !QRectF(QPointF(0.0, 0.0), QSizeF(thisSourceDesignSize))
             .contains(designPosition)) {
        return false;
    }

    QWidget *target = Q_NULLPTR;
    if (thisMouseCaptureWidget && type != QEvent::MouseButtonPress) {
        target = thisMouseCaptureWidget;
    } else {
        target = thisContentRoot->childAt(designPosition.toPoint());
        if (!target) {
            target = thisContentRoot;
        }

        if (QWidget *scrollBar = scrollBarAncestor(target)) {
            target = scrollBar;
        } else if (QWidget *textInput = textInputAncestor(target)) {
            target = textInput;
        }
    }

    if (!target) {
        return false;
    }

    if (type == QEvent::MouseButtonPress) {
        thisMouseCaptureWidget = target;
    } else if (type == QEvent::MouseButtonRelease && thisMouseCaptureWidget) {
        target = thisMouseCaptureWidget;
        thisMouseCaptureWidget = Q_NULLPTR;
    }

    const QPointF localPosition =
        target->mapFrom(thisContentRoot, designPosition.toPoint());
    QMouseEvent forwardedEvent(type, localPosition, designPosition,
                               target->mapToGlobal(localPosition.toPoint()),
                               button, buttons, modifiers);
    const bool handled = QApplication::sendEvent(target, &forwardedEvent);
    if (handled || forwardedEvent.isAccepted()) {
        markTextureDirty();
    }

    if (type == QEvent::MouseButtonPress && !scrollBarAncestor(target)) {
        if (thisFocusedOverlayWidget && thisFocusedOverlayWidget != target) {
            QFocusEvent focusOutEvent(QEvent::FocusOut, Qt::MouseFocusReason);
            QApplication::sendEvent(thisFocusedOverlayWidget, &focusOutEvent);
        }
        thisFocusedOverlayWidget = target;
        QFocusEvent focusInEvent(QEvent::FocusIn, Qt::MouseFocusReason);
        QApplication::sendEvent(thisFocusedOverlayWidget, &focusInEvent);
        target->setFocus(Qt::MouseFocusReason);
        markTextureDirty();
    }

    return handled || forwardedEvent.isAccepted();
}

bool ca3DOverlayWidgetManager::sendKeyEvent(QKeyEvent *event) {
    if (!event || !thisFocusedOverlayWidget) {
        return false;
    }

    QKeyEvent forwardedEvent(event->type(), event->key(), event->modifiers(),
                             event->text(), event->isAutoRepeat(),
                             event->count());
    const bool handled =
        QApplication::sendEvent(thisFocusedOverlayWidget, &forwardedEvent);
    if (handled || forwardedEvent.isAccepted()) {
        markTextureDirty();
    }
    return handled || forwardedEvent.isAccepted();
}

bool ca3DOverlayWidgetManager::hasFocusedTextInput() const {
    return textInputAncestor(thisFocusedOverlayWidget) != Q_NULLPTR;
}

bool ca3DOverlayWidgetManager::takeTextureDirty() {
    const bool dirty = thisTextureDirty;
    thisTextureDirty = false;
    return dirty;
}

void ca3DOverlayWidgetManager::markTextureDirty() { thisTextureDirty = true; }

void ca3DOverlayWidgetManager::installDirtyTracking(QWidget *widget) {
    if (!widget) {
        return;
    }

    widget->installEventFilter(this);
    const QList<QWidget *> children = widget->findChildren<QWidget *>();
    for (QWidget *child : children) {
        if (child) {
            child->installEventFilter(this);
        }
    }
}

void ca3DOverlayWidgetManager::clearOverlayFocus() {
    if (!thisFocusedOverlayWidget) {
        return;
    }

    QFocusEvent focusOutEvent(QEvent::FocusOut, Qt::OtherFocusReason);
    QApplication::sendEvent(thisFocusedOverlayWidget, &focusOutEvent);
    thisFocusedOverlayWidget->clearFocus();
    thisFocusedOverlayWidget = Q_NULLPTR;
    markTextureDirty();
}
