/*
 *  This file is part of the caQtDM Framework.
 */

#include "ca3dwidget.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "ca3doverlaywidgetmanager.h"
#endif
#include "cainclude.h"

#include <QFrame>
#include <QApplication>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QLoggingCategory>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QQuaternion>
#include <QResizeEvent>
#include <QTimer>
#include <QVector4D>
#include <QVBoxLayout>
#include <QtMath>
#include <cmath>
#include <memory>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtDesigner/QDesignerFormWindowInterface>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QDiffuseMapMaterial>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QTextureMaterial>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QMesh>
#include <Qt3DRender/QPaintedTextureImage>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QTextureImage>
#include <QUrl>
#endif

Q_LOGGING_CATEGORY(ca3DWidgetLog, "caqtdm.widgets.ca3dwidget")

namespace
{
constexpr qreal kOverlayTextureScale = 4.0;

QQuaternion rotationFromEuler(const QVector3D &rotation)
{
    return QQuaternion::fromEulerAngles(rotation.x(), rotation.y(), rotation.z());
}

QVector3D normalizedOrFallback(const QVector3D &vector, const QVector3D &fallback)
{
    return vector.lengthSquared() > 0.0f ? vector.normalized() : fallback;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class LiveWidgetTextureImage final : public Qt3DRender::QPaintedTextureImage
{
public:
    LiveWidgetTextureImage(const QImage &image, Qt3DCore::QNode *parent)
        : Qt3DRender::QPaintedTextureImage(parent)
        , thisImage(image)
    {
        setSize(thisImage.size().expandedTo(QSize(1, 1)));
    }

    void setImage(const QImage &image)
    {
        thisImage = image;
        setSize(thisImage.size().expandedTo(QSize(1, 1)));
        update();
    }

protected:
    void paint(QPainter *painter) override
    {
        painter->setViewport(0, height(), width(), -height());
        painter->fillRect(QRect(QPoint(0, 0), size()), Qt::transparent);
        painter->drawImage(QPoint(0, 0), thisImage);
    }

private:
    QImage thisImage;
};

class OverlayInteractionFilter final : public QObject
{
public:
    OverlayInteractionFilter(QWidget *viewport,
                             QWindow *renderWindow,
                             Qt3DRender::QCamera *camera,
                             ca3DOverlayWidgetManager *overlayManager,
                             const QVector3D &center,
                             const QVector3D &right,
                             const QVector3D &up,
                             float width,
                             float height,
                             QObject *parent)
        : QObject(parent)
        , thisViewport(viewport)
        , thisRenderWindow(renderWindow)
        , thisCamera(camera)
        , thisOverlayManager(overlayManager)
        , thisCenter(center)
        , thisRight(right.normalized())
        , thisUp(up.normalized())
        , thisNormal(QVector3D::crossProduct(thisRight, thisUp).normalized())
        , thisWidth(width)
        , thisHeight(height)
        , thisOverlayFocused(false)
        , thisForwardingKeyEvent(false)
    {
        setProperty("ca3DOverlayActive", false);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (!thisCamera || !thisOverlayManager || !property("ca3DOverlayActive").toBool()) {
            return QObject::eventFilter(watched, event);
        }

        const bool isMouseEvent = event->type() == QEvent::MouseButtonPress ||
                                  event->type() == QEvent::MouseButtonRelease ||
                                  event->type() == QEvent::MouseMove;
        const bool isKeyEvent = event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease;
        const bool isShortcutOverride = event->type() == QEvent::ShortcutOverride;

        if (thisForwardingKeyEvent && isKeyEvent) {
            return QObject::eventFilter(watched, event);
        }
        if (isMouseEvent && watched != thisViewport && watched != thisRenderWindow) {
            return QObject::eventFilter(watched, event);
        }
        if (isKeyEvent && !thisOverlayFocused) {
            return QObject::eventFilter(watched, event);
        }
        if (isShortcutOverride) {
            if (thisOverlayFocused && thisOverlayManager->hasFocusedTextInput()) {
                event->accept();
                return true;
            }
            return QObject::eventFilter(watched, event);
        }

        if (isMouseEvent) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            QPointF designPosition;
            if (!designPositionFromMouse(mouseEvent->position(), &designPosition)) {
                if (event->type() == QEvent::MouseButtonPress) {
                    thisOverlayFocused = false;
                }
                return QObject::eventFilter(watched, event);
            }

            if (event->type() == QEvent::MouseButtonPress) {
                thisOverlayFocused = true;
                if (thisViewport) {
                    thisViewport->setFocus(Qt::MouseFocusReason);
                }
            }

            return thisOverlayManager->sendMouseEvent(designPosition,
                                                      event->type(),
                                                      mouseEvent->button(),
                                                      mouseEvent->buttons(),
                                                      mouseEvent->modifiers());
        }

        if (isKeyEvent) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Escape) {
                thisOverlayManager->clearOverlayFocus();
                thisOverlayFocused = false;
                return true;
            }
            thisForwardingKeyEvent = true;
            const bool handled = thisOverlayManager->sendKeyEvent(keyEvent);
            thisForwardingKeyEvent = false;
            return handled;
        }

        return QObject::eventFilter(watched, event);
    }

private:
    bool designPositionFromMouse(const QPointF &mousePosition, QPointF *designPosition) const
    {
        const QSize designSize = thisOverlayManager->sourceDesignSize();
        const QSize surfaceSize = currentSurfaceSize();
        if (designSize.isEmpty() || surfaceSize.width() <= 0 || surfaceSize.height() <= 0) {
            return false;
        }

        const QRect viewportRect(QPoint(0, 0), surfaceSize);
        const QVector3D nearPoint(static_cast<float>(mousePosition.x()),
                                  static_cast<float>(surfaceSize.height() - mousePosition.y()),
                                  0.0f);
        const QVector3D farPoint(static_cast<float>(mousePosition.x()),
                                 static_cast<float>(surfaceSize.height() - mousePosition.y()),
                                 1.0f);
        const QMatrix4x4 viewMatrix = thisCamera->viewMatrix();
        const QMatrix4x4 projectionMatrix = thisCamera->projectionMatrix();
        const QVector3D nearWorld = nearPoint.unproject(viewMatrix, projectionMatrix, viewportRect);
        const QVector3D farWorld = farPoint.unproject(viewMatrix, projectionMatrix, viewportRect);
        QVector3D rayDirection = farWorld - nearWorld;
        rayDirection = rayDirection.lengthSquared() > 0.0f ? rayDirection.normalized() : QVector3D(0.0f, 0.0f, -1.0f);
        const float denominator = QVector3D::dotProduct(rayDirection, thisNormal);
        if (std::abs(denominator) < 0.0001f) {
            return false;
        }

        const float distance = QVector3D::dotProduct(thisCenter - nearWorld, thisNormal) / denominator;
        if (distance < 0.0f) {
            return false;
        }

        const QVector3D hitPoint = nearWorld + (rayDirection * distance);
        const QVector3D local = hitPoint - thisCenter;
        const float localX = QVector3D::dotProduct(local, thisRight);
        const float localY = QVector3D::dotProduct(local, thisUp);
        if (std::abs(localX) > thisWidth * 0.5f || std::abs(localY) > thisHeight * 0.5f) {
            return false;
        }

        const qreal x = (static_cast<qreal>(localX / thisWidth) + 0.5) * designSize.width();
        const qreal y = (0.5 - static_cast<qreal>(localY / thisHeight)) * designSize.height();
        *designPosition = QPointF(x, y);
        return true;
    }

    QSize currentSurfaceSize() const
    {
        if (thisRenderWindow && !thisRenderWindow->size().isEmpty()) {
            return thisRenderWindow->size();
        }
        return thisViewport ? thisViewport->size() : QSize();
    }

    QWidget *thisViewport;
    QWindow *thisRenderWindow;
    Qt3DRender::QCamera *thisCamera;
    ca3DOverlayWidgetManager *thisOverlayManager;
    QVector3D thisCenter;
    QVector3D thisRight;
    QVector3D thisUp;
    QVector3D thisNormal;
    float thisWidth;
    float thisHeight;
    bool thisOverlayFocused;
    bool thisForwardingKeyEvent;
};

bool overlayIsInCameraView(Qt3DRender::QCamera *camera, const ca3DOverlayConfig &overlay)
{
    if (!camera) {
        return false;
    }

    const QMatrix4x4 viewProjection = camera->projectionMatrix() * camera->viewMatrix();
    const QVector4D clip = viewProjection * QVector4D(overlay.position, 1.0f);
    if (clip.w() <= 0.0f) {
        return false;
    }

    const QVector3D ndc = clip.toVector3DAffine();
    constexpr float margin = 1.05f;
    return ndc.x() >= -margin && ndc.x() <= margin
           && ndc.y() >= -margin && ndc.y() <= margin
           && ndc.z() >= -margin && ndc.z() <= margin;
}

QVector3D cameraForward(Qt3DRender::QCamera *camera)
{
    return normalizedOrFallback(camera->viewCenter() - camera->position(), QVector3D(0.0f, 0.0f, -1.0f));
}

QVector3D cameraRight(Qt3DRender::QCamera *camera)
{
    return normalizedOrFallback(QVector3D::crossProduct(cameraForward(camera), camera->upVector()), QVector3D(1.0f, 0.0f, 0.0f));
}

void moveCameraAlong(Qt3DRender::QCamera *camera, const QVector3D &direction, double distance)
{
    const QVector3D delta = direction * static_cast<float>(distance);
    camera->setPosition(camera->position() + delta);
    camera->setViewCenter(camera->viewCenter() + delta);
}

void turnCameraBy(Qt3DRender::QCamera *camera, double yawDelta, double pitchDelta)
{
    const float viewDistance = qMax((camera->viewCenter() - camera->position()).length(), 1.0f);
    QVector3D forward = cameraForward(camera);
    if (!qFuzzyIsNull(yawDelta)) {
        forward = QQuaternion::fromAxisAndAngle(camera->upVector(), static_cast<float>(yawDelta)).rotatedVector(forward);
    }
    if (!qFuzzyIsNull(pitchDelta)) {
        forward = QQuaternion::fromAxisAndAngle(cameraRight(camera), static_cast<float>(pitchDelta)).rotatedVector(forward);
    }

    camera->setViewCenter(camera->position() + normalizedOrFallback(forward, QVector3D(0.0f, 0.0f, -1.0f)) * viewDistance);
    camera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
}

QString cameraDebugState(Qt3DRender::QCamera *camera)
{
    if (!camera) {
        return QStringLiteral("camera=null");
    }

    const QVector3D position = camera->position();
    const QVector3D viewCenter = camera->viewCenter();
    const QVector3D upVector = camera->upVector();
    return QStringLiteral("position=(%1,%2,%3) viewCenter=(%4,%5,%6) upVector=(%7,%8,%9)")
        .arg(position.x()).arg(position.y()).arg(position.z())
        .arg(viewCenter.x()).arg(viewCenter.y()).arg(viewCenter.z())
        .arg(upVector.x()).arg(upVector.y()).arg(upVector.z());
}
#endif
}

ca3DWidget::ca3DWidget(QWidget *parent)
    : QWidget(parent)
    , thisStatusLabel(new QLabel(this))
    , thisViewContainer(Q_NULLPTR)
    , thisFallbackView(new QWidget(this))
    , thisFallbackSnapshotLabel(new QLabel(thisFallbackView))
    , thisCameraPreset(0)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    , thisFallbackMode(true)
#else
    , thisFallbackMode(false)
#endif
    , thisConfigValid(true)
    , thisDesignerMode(false)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    , this3DView(Q_NULLPTR)
    , thisRootEntity(Q_NULLPTR)
#endif
{
    setMinimumSize(120, 80);
    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(30, 34, 40));
    pal.setColor(QPalette::WindowText, Qt::white);
    setPalette(pal);

    thisStatusLabel->setAlignment(Qt::AlignCenter);
    thisStatusLabel->setWordWrap(true);
    thisStatusLabel->setFrameShape(QFrame::Box);
    thisStatusLabel->setFrameShadow(QFrame::Sunken);
    thisStatusLabel->setMargin(8);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(thisStatusLabel);

    thisFallbackView->hide();
    thisFallbackSnapshotLabel->setAlignment(Qt::AlignCenter);
    thisFallbackSnapshotLabel->setScaledContents(true);
    thisFallbackSnapshotLabel->setAutoFillBackground(true);
    thisFallbackSnapshotLabel->setPalette(pal);
    layout->addWidget(thisFallbackView);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QTimer::singleShot(0, this, [this]() { maybeInitialize3DView(); });
#endif

    updatePlaceholderText();
}

ca3DWidget::~ca3DWidget()
{
    clearScene();
    clearFallbackView();
}

void ca3DWidget::setSceneConfig(const QString &config)
{
    if (thisSceneConfig == config) {
        return;
    }

    thisSceneConfig = config;
    thisConfigValid = ca3DConfigParser::parse(thisSceneConfig, &thisConfig, &thisConfigErrors);
    rebuildScene();
    updatePlaceholderText();
}

QList<QWidget*> ca3DWidget::overlayRootWidgets() const
{
    QList<QWidget*> roots;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    for (ca3DOverlayWidgetManager *manager : this3DOverlayManagers) {
        if (manager && manager->contentRoot()) {
            roots.append(manager->contentRoot());
        }
    }
#endif
    return roots;
}

QString ca3DWidget::overlayMacro(QWidget *rootWidget) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    for (const ca3DOverlayConfig &overlay : thisConfig.overlays) {
        ca3DOverlayWidgetManager *manager = this3DOverlayManagersById.value(overlay.id, Q_NULLPTR);
        if (manager && manager->contentRoot() == rootWidget) {
            return overlay.macro;
        }
    }
#else
    Q_UNUSED(rootWidget);
#endif
    return QString();
}

QString ca3DWidget::overlayIncludePath(QWidget *rootWidget) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    for (const ca3DOverlayConfig &overlay : thisConfig.overlays) {
        ca3DOverlayWidgetManager *manager = this3DOverlayManagersById.value(overlay.id, Q_NULLPTR);
        if (manager && manager->contentRoot() == rootWidget) {
            const QFileInfo fileInfo(overlay.includeFileResolved.isEmpty() ? overlay.includeFile : overlay.includeFileResolved);
            if (!fileInfo.path().isEmpty()) {
                return fileInfo.path() + QStringLiteral("/");
            }
        }
    }
#else
    Q_UNUSED(rootWidget);
#endif
    return QString();
}

void ca3DWidget::setCameraPreset(int preset)
{
    qCDebug(ca3DWidgetLog) << "setCameraPreset" << preset;
    if (preset < 0) {
        qCWarning(ca3DWidgetLog) << "setCameraPreset ignored negative preset" << preset;
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool presetFound = thisConfig.cameraPresets.isEmpty();
    if (presetFound) {
        thisCameraPreset = preset;
    }
    for (const ca3DCameraPresetConfig &cameraPreset : thisConfig.cameraPresets) {
        if (cameraPreset.id == preset) {
            presetFound = true;
            thisCameraPreset = preset;
            applyCameraPresetConfig(cameraPreset);
            break;
        }
    }
    if (!presetFound) {
        qCWarning(ca3DWidgetLog) << "setCameraPreset ignored unknown preset" << preset;
        return;
    }
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "setCameraPreset applied" << preset
                            << cameraDebugState(this3DView ? this3DView->camera() : Q_NULLPTR);
#else
    thisCameraPreset = preset;
#endif
    applyFallbackPreset(thisCameraPreset);
    updatePlaceholderText();
}

void ca3DWidget::setCameraPosition(double x, double y, double z)
{
    qCDebug(ca3DWidgetLog) << "setCameraPosition" << x << y << z;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "setCameraPosition ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    const QVector3D position(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    const QVector3D delta = position - camera->position();
    camera->setPosition(position);
    camera->setViewCenter(camera->viewCenter() + delta);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "setCameraPosition applied" << cameraDebugState(camera);
#else
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
#endif
}

void ca3DWidget::moveCamera(double dx, double dy, double dz)
{
    qCDebug(ca3DWidgetLog) << "moveCamera" << dx << dy << dz;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "moveCamera ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    const QVector3D delta(static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz));
    camera->setPosition(camera->position() + delta);
    camera->setViewCenter(camera->viewCenter() + delta);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "moveCamera applied" << cameraDebugState(camera);
#else
    Q_UNUSED(dx);
    Q_UNUSED(dy);
    Q_UNUSED(dz);
#endif
}

void ca3DWidget::moveCameraForward(double distance)
{
    qCDebug(ca3DWidgetLog) << "moveCameraForward" << distance;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "moveCameraForward ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    moveCameraAlong(camera, cameraForward(camera), distance);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "moveCameraForward applied" << cameraDebugState(camera);
#else
    Q_UNUSED(distance);
#endif
}

void ca3DWidget::moveCameraBackward(double distance)
{
    qCDebug(ca3DWidgetLog) << "moveCameraBackward" << distance;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "moveCameraBackward ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    moveCameraAlong(camera, -cameraForward(camera), distance);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "moveCameraBackward applied" << cameraDebugState(camera);
#else
    Q_UNUSED(distance);
#endif
}

void ca3DWidget::moveCameraRight(double distance)
{
    qCDebug(ca3DWidgetLog) << "moveCameraRight" << distance;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "moveCameraRight ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    moveCameraAlong(camera, cameraRight(camera), distance);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "moveCameraRight applied" << cameraDebugState(camera);
#else
    Q_UNUSED(distance);
#endif
}

void ca3DWidget::moveCameraLeft(double distance)
{
    qCDebug(ca3DWidgetLog) << "moveCameraLeft" << distance;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "moveCameraLeft ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    moveCameraAlong(camera, -cameraRight(camera), distance);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "moveCameraLeft applied" << cameraDebugState(camera);
#else
    Q_UNUSED(distance);
#endif
}

void ca3DWidget::setCameraRotation(double yaw, double pitch)
{
    qCDebug(ca3DWidgetLog) << "setCameraRotation" << yaw << pitch;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "setCameraRotation ignored: 3D view not initialized";
        return;
    }

    const float yawRadians = qDegreesToRadians(static_cast<float>(yaw));
    const float pitchRadians = qDegreesToRadians(static_cast<float>(pitch));
    const float cosPitch = qCos(pitchRadians);
    const QVector3D forward(qSin(yawRadians) * cosPitch,
                            qSin(pitchRadians),
                            -qCos(yawRadians) * cosPitch);

    Qt3DRender::QCamera *camera = this3DView->camera();
    camera->setViewCenter(camera->position() + forward * 100.0f);
    camera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "setCameraRotation applied" << cameraDebugState(camera);
#else
    Q_UNUSED(yaw);
    Q_UNUSED(pitch);
#endif
}

void ca3DWidget::turnCameraUp(double angle)
{
    qCDebug(ca3DWidgetLog) << "turnCameraUp" << angle;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "turnCameraUp ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    turnCameraBy(camera, 0.0, angle);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "turnCameraUp applied" << cameraDebugState(camera);
#else
    Q_UNUSED(angle);
#endif
}

void ca3DWidget::turnCameraDown(double angle)
{
    qCDebug(ca3DWidgetLog) << "turnCameraDown" << angle;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "turnCameraDown ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    turnCameraBy(camera, 0.0, -angle);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "turnCameraDown applied" << cameraDebugState(camera);
#else
    Q_UNUSED(angle);
#endif
}

void ca3DWidget::turnCameraRight(double angle)
{
    qCDebug(ca3DWidgetLog) << "turnCameraRight" << angle;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "turnCameraRight ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    turnCameraBy(camera, -angle, 0.0);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "turnCameraRight applied" << cameraDebugState(camera);
#else
    Q_UNUSED(angle);
#endif
}

void ca3DWidget::turnCameraLeft(double angle)
{
    qCDebug(ca3DWidgetLog) << "turnCameraLeft" << angle;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "turnCameraLeft ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    turnCameraBy(camera, angle, 0.0);
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "turnCameraLeft applied" << cameraDebugState(camera);
#else
    Q_UNUSED(angle);
#endif
}

void ca3DWidget::setCameraViewCenter(double x, double y, double z)
{
    qCDebug(ca3DWidgetLog) << "setCameraViewCenter" << x << y << z;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "setCameraViewCenter ignored: 3D view not initialized";
        return;
    }

    Qt3DRender::QCamera *camera = this3DView->camera();
    camera->setViewCenter(QVector3D(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
    camera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "setCameraViewCenter applied" << cameraDebugState(camera);
#else
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
#endif
}

void ca3DWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (thisFallbackSnapshotLabel && thisFallbackView) {
        thisFallbackSnapshotLabel->setGeometry(thisFallbackView->rect());
    }
}

void ca3DWidget::setObjectAxisValue(const QString &objectId, const QString &axisId, double value)
{
    qCDebug(ca3DWidgetLog) << "setObjectAxisValue" << objectId << axisId << value;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    for (const ca3DObjectConfig &object : thisConfig.objects) {
        if (object.id != objectId) {
            continue;
        }

        for (const ca3DAxisConfig &axis : object.axes) {
            if (axis.id != axisId) {
                continue;
            }

            if (axis.type == ca3DAxisConfig::Rotation) {
                thisDynamicRotations[objectId] = axis.vector * static_cast<float>(value * axis.factor);
            } else {
                thisDynamicTranslations[objectId] = axis.vector * static_cast<float>(value * axis.factor);
            }
            applyObjectTransform(objectId);
            qCDebug(ca3DWidgetLog) << "setObjectAxisValue applied" << objectId << axisId << value;
            return;
        }
    }
    qCWarning(ca3DWidgetLog) << "setObjectAxisValue ignored unknown object/axis" << objectId << axisId;
#else
    Q_UNUSED(objectId);
    Q_UNUSED(axisId);
    Q_UNUSED(value);
#endif
}

void ca3DWidget::setObjectTranslation(const QString &objectId, double x, double y, double z)
{
    qCDebug(ca3DWidgetLog) << "setObjectTranslation" << objectId << x << y << z;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    thisDynamicTranslations[objectId] = QVector3D(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    applyObjectTransform(objectId);
    qCDebug(ca3DWidgetLog) << "setObjectTranslation applied" << objectId;
#else
    Q_UNUSED(objectId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
#endif
}

void ca3DWidget::setObjectRotation(const QString &objectId, double rx, double ry, double rz)
{
    qCDebug(ca3DWidgetLog) << "setObjectRotation" << objectId << rx << ry << rz;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    thisDynamicRotations[objectId] = QVector3D(static_cast<float>(rx), static_cast<float>(ry), static_cast<float>(rz));
    applyObjectTransform(objectId);
    qCDebug(ca3DWidgetLog) << "setObjectRotation applied" << objectId;
#else
    Q_UNUSED(objectId);
    Q_UNUSED(rx);
    Q_UNUSED(ry);
    Q_UNUSED(rz);
#endif
}

void ca3DWidget::updatePlaceholderText()
{
    const QString mode = thisFallbackMode ? QStringLiteral("2D fallback") : QStringLiteral("Qt6 3D");
    const QString configState = thisSceneConfig.trimmed().isEmpty()
                                ? QStringLiteral("no sceneConfig")
                                : (thisConfigValid ? QStringLiteral("sceneConfig ok") : QStringLiteral("sceneConfig errors: %1").arg(thisConfigErrors.count()));

    thisStatusLabel->setText(QStringLiteral("ca3DWidget\n%1\nPreset: %2\nObjects: %3  Overlays: %4  Cameras: %5\n%6")
                             .arg(mode)
                             .arg(thisCameraPreset)
                             .arg(thisConfig.objects.count())
                             .arg(thisConfig.overlays.count())
                             .arg(thisConfig.cameraPresets.count())
                             .arg(configState));
}

void ca3DWidget::rebuildScene()
{
    if (thisFallbackMode) {
        rebuildFallbackView();
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (thisDesignerMode || !this3DView) {
        thisStatusLabel->show();
        return;
    }

    clearScene();
    if (!thisConfigValid || thisConfig.isEmpty()) {
        thisStatusLabel->show();
        return;
    }

    thisStatusLabel->hide();
    thisRootEntity = new Qt3DCore::QEntity();
    this3DView->defaultFrameGraph()->setClearColor(thisConfig.backgroundColor);

    Qt3DCore::QEntity *lightEntity = new Qt3DCore::QEntity(thisRootEntity);
    Qt3DRender::QPointLight *light = new Qt3DRender::QPointLight(lightEntity);
    light->setColor(Qt::white);
    light->setIntensity(1.0f);
    Qt3DCore::QTransform *lightTransform = new Qt3DCore::QTransform(lightEntity);
    lightTransform->setTranslation(QVector3D(0.0f, 500.0f, 500.0f));
    lightEntity->addComponent(light);
    lightEntity->addComponent(lightTransform);

    for (const ca3DObjectConfig &object : thisConfig.objects) {
        if (object.meshResolved.isEmpty()) {
            qCWarning(ca3DWidgetLog) << "rebuildScene skipping object without resolved mesh" << object.id << object.mesh;
            continue;
        }
        qCDebug(ca3DWidgetLog) << "rebuildScene create object" << object.id
                                << "mesh" << object.meshResolved
                                << "texture" << object.textureResolved
                                << "position" << object.position
                                << "rotation" << object.rotation
                                << "scale" << object.scale;

        Qt3DCore::QEntity *entity = new Qt3DCore::QEntity(thisRootEntity);
        Qt3DRender::QMesh *mesh = new Qt3DRender::QMesh(entity);
        mesh->setSource(QUrl::fromLocalFile(object.meshResolved));
        entity->addComponent(mesh);

        if (!object.textureResolved.isEmpty()) {
            Qt3DRender::QTexture2D *texture = new Qt3DRender::QTexture2D(entity);
            Qt3DRender::QTextureImage *textureImage = new Qt3DRender::QTextureImage(texture);
            textureImage->setSource(QUrl::fromLocalFile(object.textureResolved));
            texture->addTextureImage(textureImage);

            Qt3DExtras::QDiffuseMapMaterial *material = new Qt3DExtras::QDiffuseMapMaterial(entity);
            material->setDiffuse(texture);
            if (object.hasMaterialColor) {
                material->setAmbient(object.materialColor);
            }
            entity->addComponent(material);
        } else {
            Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(entity);
            if (object.hasMaterialColor) {
                material->setAmbient(object.materialColor);
                material->setDiffuse(object.materialColor);
            }
            entity->addComponent(material);
        }

        Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(entity);
        entity->addComponent(transform);
        thisObjectTransforms.insert(object.id, transform);
        applyObjectTransform(object.id);
    }

    rebuild3DOverlays();

    this3DView->setRootEntity(thisRootEntity);
    if (!thisConfig.cameraPresets.isEmpty()) {
        const int requestedPreset = thisCameraPreset > 0 ? thisCameraPreset : thisConfig.cameraPresets.first().id;
        setCameraPreset(requestedPreset);
    } else {
        Qt3DRender::QCamera *camera = this3DView->camera();
        camera->setPosition(QVector3D(0.0f, 0.0f, 1000.0f));
        camera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
        camera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    }
#endif
}

void ca3DWidget::rebuildFallbackView()
{
    clearFallbackView();

    if (thisDesignerMode) {
        thisFallbackView->hide();
        thisStatusLabel->show();
        return;
    }

    if (!thisFallbackMode || !thisConfigValid || thisConfig.isEmpty()) {
        thisFallbackView->hide();
        thisStatusLabel->show();
        return;
    }

    thisStatusLabel->hide();
    thisFallbackView->show();
    thisFallbackSnapshotLabel->setGeometry(thisFallbackView->rect());
    thisFallbackSnapshotLabel->lower();

    for (const ca3DOverlayConfig &overlay : thisConfig.overlays) {
        if (overlay.includeFileResolved.isEmpty() || overlay.fallbackGeometry.isEmpty()) {
            continue;
        }

        caInclude *include = new caInclude(thisFallbackView);
        include->setObjectName(QStringLiteral("ca3DOverlay_%1").arg(overlay.id));
        include->setLoadIncludes(true);
        include->setFrameShape(caInclude::NoFrame);
        include->setAttribute(Qt::WA_TranslucentBackground, overlay.transparentBackground);
        include->setAutoFillBackground(!overlay.transparentBackground);
        include->setGeometry(overlay.fallbackGeometry);
        include->setFileName(overlay.includeFileResolved);
        include->hide();
        thisFallbackOverlayWidgets.insert(overlay.id, include);
    }

    const int preset = thisCameraPreset > 0 || thisConfig.cameraPresets.isEmpty()
                       ? thisCameraPreset
                       : thisConfig.cameraPresets.first().id;
    applyFallbackPreset(preset);
}

void ca3DWidget::clearFallbackView()
{
    for (QWidget *widget : thisFallbackOverlayWidgets) {
        if (widget) {
            widget->hide();
            widget->deleteLater();
        }
    }
    thisFallbackOverlayWidgets.clear();
    thisFallbackSnapshotPixmap = QPixmap();
    if (thisFallbackSnapshotLabel) {
        thisFallbackSnapshotLabel->clear();
    }
}

void ca3DWidget::applyFallbackPreset(int preset)
{
    if (!thisFallbackMode || !thisFallbackView || !thisFallbackView->isVisible()) {
        return;
    }

    const ca3DCameraPresetConfig *selectedPreset = Q_NULLPTR;
    for (const ca3DCameraPresetConfig &cameraPreset : thisConfig.cameraPresets) {
        if (cameraPreset.id == preset) {
            selectedPreset = &cameraPreset;
            break;
        }
    }

    if (!thisConfig.cameraPresets.isEmpty() && !selectedPreset) {
        return;
    }

    if (selectedPreset && !selectedPreset->snapshotResolved.isEmpty()) {
        thisFallbackSnapshotPixmap.load(selectedPreset->snapshotResolved);
        thisFallbackSnapshotLabel->setPixmap(thisFallbackSnapshotPixmap);
    } else {
        thisFallbackSnapshotPixmap = QPixmap();
        thisFallbackSnapshotLabel->clear();
    }

    for (QWidget *widget : thisFallbackOverlayWidgets) {
        if (widget) {
            widget->hide();
        }
    }

    for (const ca3DOverlayConfig &overlay : thisConfig.overlays) {
        QWidget *widget = thisFallbackOverlayWidgets.value(overlay.id, Q_NULLPTR);
        if (!widget) {
            continue;
        }

        bool visible = thisConfig.cameraPresets.isEmpty();
        if (selectedPreset) {
            visible = selectedPreset->overlays.contains(overlay.id)
                      || (overlay.cameraPreset > 0 && overlay.cameraPreset == selectedPreset->id);
        }

        if (visible) {
            widget->setGeometry(overlay.fallbackGeometry);
            widget->show();
            widget->raise();
        }
    }
}

bool ca3DWidget::isDesignerMode() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt Designer preview creates a normal top-level widget. Only the editable
    // form surface has a QDesignerFormWindowInterface and must avoid native
    // QWindow containers because they break drag/drop and selection handling.
    return QDesignerFormWindowInterface::findFormWindow(const_cast<ca3DWidget *>(this)) != Q_NULLPTR;
#else
    return false;
#endif
}

void ca3DWidget::clearScene()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    clear3DOverlays();
    thisObjectTransforms.clear();
    if (this3DView) {
        this3DView->setRootEntity(Q_NULLPTR);
    }
    if (thisRootEntity) {
        thisRootEntity->deleteLater();
    }
    thisRootEntity = Q_NULLPTR;
#endif
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void ca3DWidget::maybeInitialize3DView()
{
    thisDesignerMode = isDesignerMode();
    if (thisDesignerMode) {
        qCDebug(ca3DWidgetLog) << "maybeInitialize3DView using 2D fallback: designer edit mode";
        thisFallbackMode = true;
        updatePlaceholderText();
        return;
    }

    if (shouldUse2DFallback()) {
        qCWarning(ca3DWidgetLog) << "maybeInitialize3DView using 2D fallback: OpenGL unavailable or software renderer detected";
        thisFallbackMode = true;
        rebuildFallbackView();
        updatePlaceholderText();
        return;
    }

    if (!this3DView) {
        qCDebug(ca3DWidgetLog) << "maybeInitialize3DView creating Qt3D view";
        initialize3DView();
        rebuildScene();
    }
}

void ca3DWidget::initialize3DView()
{
    this3DView = new Qt3DExtras::Qt3DWindow();
    this3DView->defaultFrameGraph()->setClearColor(QColor(30, 34, 40));
    thisViewContainer = QWidget::createWindowContainer(this3DView, this);
    thisViewContainer->setMinimumSize(120, 80);
    thisViewContainer->setFocusPolicy(Qt::StrongFocus);
    layout()->addWidget(thisViewContainer);
}

bool ca3DWidget::shouldUse2DFallback() const
{
    if (qEnvironmentVariableIntValue("CAQTDM_3D_FORCE_FALLBACK") > 0) {
        qCWarning(ca3DWidgetLog) << "shouldUse2DFallback forced by CAQTDM_3D_FORCE_FALLBACK";
        return true;
    }

    QOpenGLContext context;
    if (!context.create()) {
        qCWarning(ca3DWidgetLog) << "shouldUse2DFallback no OpenGL context";
        return true;
    }

    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    if (!surface.isValid() || !context.makeCurrent(&surface)) {
        qCWarning(ca3DWidgetLog) << "shouldUse2DFallback invalid offscreen OpenGL surface";
        return true;
    }

    QOpenGLFunctions *functions = context.functions();
    const char *rendererRaw = reinterpret_cast<const char *>(functions->glGetString(GL_RENDERER));
    const QString renderer = rendererRaw ? QString::fromLatin1(rendererRaw).toLower() : QString();
    context.doneCurrent();
    qCDebug(ca3DWidgetLog) << "shouldUse2DFallback OpenGL renderer" << renderer;

    return renderer.isEmpty()
           || renderer.contains(QStringLiteral("llvmpipe"))
           || renderer.contains(QStringLiteral("softpipe"))
           || renderer.contains(QStringLiteral("software"))
           || renderer.contains(QStringLiteral("swrast"));
}

void ca3DWidget::rebuild3DOverlays()
{
    clear3DOverlays();
    if (!thisRootEntity || !this3DView || !thisViewContainer) {
        return;
    }

    for (const ca3DOverlayConfig &overlay : thisConfig.overlays) {
        if (overlay.includeFileResolved.isEmpty()) {
            continue;
        }

        ca3DOverlayWidgetManager *overlayManager = new ca3DOverlayWidgetManager(this);
        overlayManager->loadWidgetsFromUi(overlay.includeFileResolved);
        const QSize designSize = overlayManager->sourceDesignSize();
        if (designSize.isEmpty()) {
            overlayManager->deleteLater();
            continue;
        }

        const float overlayWidth = static_cast<float>(overlay.size.width() > 0.0 ? overlay.size.width() : 1.5);
        const float overlayHeight = static_cast<float>(overlay.size.height() > 0.0 ? overlay.size.height() : 1.0);
        const QQuaternion overlayRotation = rotationFromEuler(overlay.rotation);
        const QQuaternion planeLocalToUprightPlane = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 90.0f);
        const QQuaternion renderRotation = overlayRotation * planeLocalToUprightPlane;
        const QVector3D right = normalizedOrFallback(overlayRotation.rotatedVector(QVector3D(1.0f, 0.0f, 0.0f)), QVector3D(1.0f, 0.0f, 0.0f));
        const QVector3D up = normalizedOrFallback(overlayRotation.rotatedVector(QVector3D(0.0f, 1.0f, 0.0f)), QVector3D(0.0f, 1.0f, 0.0f));
        qCDebug(ca3DWidgetLog) << "rebuild3DOverlays create overlay" << overlay.id
                                << "position" << overlay.position
                                << "rotation" << overlay.rotation
                                << "size" << overlayWidth << overlayHeight
                                << "designSize" << designSize;

        Qt3DCore::QEntity *overlayEntity = new Qt3DCore::QEntity(thisRootEntity);
        Qt3DExtras::QPlaneMesh *plane = new Qt3DExtras::QPlaneMesh(overlayEntity);
        plane->setWidth(overlayWidth);
        plane->setHeight(overlayHeight);

        Qt3DRender::QTexture2D *texture = new Qt3DRender::QTexture2D(overlayEntity);
        texture->setGenerateMipMaps(false);
        texture->setMinificationFilter(Qt3DRender::QAbstractTexture::Linear);
        texture->setMagnificationFilter(Qt3DRender::QAbstractTexture::Linear);
        LiveWidgetTextureImage *textureImage = new LiveWidgetTextureImage(
            overlayManager->renderSnapshot(kOverlayTextureScale).flipped(Qt::Vertical), texture);
        texture->addTextureImage(textureImage);
        textureImage->update();
        overlayManager->takeTextureDirty();

        QTimer *liveTextureTimer = new QTimer(overlayManager);
        liveTextureTimer->setTimerType(Qt::CoarseTimer);
        const std::shared_ptr<int> textureTimerTick(new int(0));
        connect(liveTextureTimer, &QTimer::timeout, overlayManager,
                [overlayManager, textureImage, textureTimerTick]() {
                    ++(*textureTimerTick);
                    const bool refreshForCaret = overlayManager->hasFocusedTextInput() && (*textureTimerTick % 5 == 0);
                    const bool refreshIdle = *textureTimerTick % 20 == 0;
                    if (!overlayManager->takeTextureDirty() && !refreshForCaret && !refreshIdle) {
                        return;
                    }
                    textureImage->setImage(overlayManager->renderSnapshot(kOverlayTextureScale).flipped(Qt::Vertical));
                });
        liveTextureTimer->start(100);

        Qt3DExtras::QTextureMaterial *material = new Qt3DExtras::QTextureMaterial(overlayEntity);
        material->setTexture(texture);
        material->setAlphaBlendingEnabled(overlay.transparentBackground);

        Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(overlayEntity);
        transform->setTranslation(overlay.position);
        transform->setRotation(renderRotation);

        overlayEntity->addComponent(plane);
        overlayEntity->addComponent(material);
        overlayEntity->addComponent(transform);

        OverlayInteractionFilter *interactionFilter = new OverlayInteractionFilter(thisViewContainer,
                                                                                  this3DView,
                                                                                  this3DView->camera(),
                                                                                  overlayManager,
                                                                                  overlay.position,
                                                                                  right,
                                                                                  up,
                                                                                  overlayWidth,
                                                                                  overlayHeight,
                                                                                  this);
        thisViewContainer->installEventFilter(interactionFilter);
        this3DView->installEventFilter(interactionFilter);
        qApp->installEventFilter(interactionFilter);

        this3DOverlayManagers.append(overlayManager);
        this3DOverlayEventFilters.append(interactionFilter);
        this3DOverlayManagersById.insert(overlay.id, overlayManager);
        this3DOverlayEventFiltersById.insert(overlay.id, interactionFilter);
        this3DOverlayEntities.insert(overlay.id, overlayEntity);
    }

    apply3DOverlayVisibility(thisCameraPreset > 0 || thisConfig.cameraPresets.isEmpty()
                             ? thisCameraPreset
                             : thisConfig.cameraPresets.first().id);
    emit overlayWidgetsRebuilt();
}

void ca3DWidget::clear3DOverlays()
{
    for (QObject *filter : this3DOverlayEventFilters) {
        if (filter) {
            if (thisViewContainer) {
                thisViewContainer->removeEventFilter(filter);
            }
            if (this3DView) {
                this3DView->removeEventFilter(filter);
            }
            qApp->removeEventFilter(filter);
            delete filter;
        }
    }
    this3DOverlayEventFilters.clear();

    for (ca3DOverlayWidgetManager *manager : this3DOverlayManagers) {
        delete manager;
    }
    this3DOverlayManagers.clear();
    this3DOverlayManagersById.clear();
    this3DOverlayEventFiltersById.clear();
    this3DOverlayEntities.clear();
}

void ca3DWidget::apply3DOverlayVisibility(int preset)
{
    if (this3DOverlayEntities.isEmpty()) {
        return;
    }

    const ca3DCameraPresetConfig *selectedPreset = Q_NULLPTR;
    for (const ca3DCameraPresetConfig &cameraPreset : thisConfig.cameraPresets) {
        if (cameraPreset.id == preset) {
            selectedPreset = &cameraPreset;
            break;
        }
    }

    if (!thisConfig.cameraPresets.isEmpty() && !selectedPreset) {
        return;
    }

    for (const ca3DOverlayConfig &overlay : thisConfig.overlays) {
        Qt3DCore::QEntity *entity = this3DOverlayEntities.value(overlay.id, Q_NULLPTR);
        if (!entity) {
            continue;
        }

        const bool presetMatches = selectedPreset
                                   && (selectedPreset->overlays.contains(overlay.id)
                                       || (overlay.cameraPreset > 0 && overlay.cameraPreset == selectedPreset->id));
        const bool inView = overlayIsInCameraView(this3DView ? this3DView->camera() : Q_NULLPTR, overlay);

        bool visible = false;
        if (overlay.visibilityMode == ca3DOverlayConfig::AlwaysWhenInView) {
            visible = inView;
        } else if (overlay.visibilityMode == ca3DOverlayConfig::InView) {
            visible = (thisConfig.cameraPresets.isEmpty() || presetMatches) && inView;
        } else {
            visible = thisConfig.cameraPresets.isEmpty() || presetMatches;
        }
        entity->setEnabled(visible);
        qCDebug(ca3DWidgetLog) << "apply3DOverlayVisibility" << overlay.id
                                << "preset" << preset
                                << "mode" << overlay.visibilityMode
                                << "presetMatches" << presetMatches
                                << "inView" << inView
                                << "visible" << visible;

        QObject *filter = this3DOverlayEventFiltersById.value(overlay.id, Q_NULLPTR);
        if (filter) {
            filter->setProperty("ca3DOverlayActive", visible);
        }
        ca3DOverlayWidgetManager *manager = this3DOverlayManagersById.value(overlay.id, Q_NULLPTR);
        if (!visible && manager) {
            manager->clearOverlayFocus();
        }
    }
}

void ca3DWidget::applyCameraPresetConfig(const ca3DCameraPresetConfig &preset)
{
    if (!this3DView) {
        return;
    }

    const float yawRadians = qDegreesToRadians(static_cast<float>(preset.yaw));
    const float pitchRadians = qDegreesToRadians(static_cast<float>(preset.pitch));
    const float cosPitch = qCos(pitchRadians);
    const QVector3D forward(qSin(yawRadians) * cosPitch,
                            qSin(pitchRadians),
                            -qCos(yawRadians) * cosPitch);

    Qt3DRender::QCamera *camera = this3DView->camera();
    camera->setPosition(preset.position);
    camera->setViewCenter(preset.hasViewCenter ? preset.viewCenter : preset.position + forward * 100.0f);
    camera->setUpVector(normalizedOrFallback(preset.upVector, QVector3D(0.0f, 1.0f, 0.0f)));
    camera->lens()->setPerspectiveProjection(static_cast<float>(preset.fov), 16.0f / 9.0f, 0.1f, 100000.0f);
    qCDebug(ca3DWidgetLog) << "applyCameraPresetConfig" << preset.id
                            << "hasViewCenter" << preset.hasViewCenter
                            << "fov" << preset.fov
                            << cameraDebugState(camera);
}

void ca3DWidget::applyObjectTransform(const QString &objectId)
{
    Qt3DCore::QTransform *transform = thisObjectTransforms.value(objectId, Q_NULLPTR);
    if (!transform) {
        return;
    }

    for (const ca3DObjectConfig &object : thisConfig.objects) {
        if (object.id != objectId) {
            continue;
        }

    const QVector3D translation = object.position
                                  + object.configuredOriginPosition
                                  + thisDynamicTranslations.value(objectId);
    const QVector3D rotation = object.rotation
                               + object.configuredOriginRotation
                               + thisDynamicRotations.value(objectId);
    transform->setTranslation(translation);
    transform->setRotation(rotationFromEuler(rotation));
    transform->setScale(static_cast<float>(object.scale));
    return;
}
}
#endif
