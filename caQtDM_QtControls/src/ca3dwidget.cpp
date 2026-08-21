/*
 *  This file is part of the caQtDM Framework.
 */

#include "ca3dwidget.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "ca3doverlaywidgetmanager.h"
#endif

#include <QFrame>
#include <QApplication>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
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
#include <QScreen>
#include <QSet>
#include <QTimer>
#include <QUiLoader>
#include <QVector4D>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QScrollBar>
#include <QtMath>
#include <memory>
#include <algorithm>
#include <iterator>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && !defined(MOBILE)
#include <QtDesigner/QDesignerFormWindowInterface>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QDiffuseMapMaterial>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QTextureMaterial>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/QDirectionalLight>
#include <Qt3DRender/QFrameGraphNode>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QMesh>
#include <Qt3DRender/QPaintedTextureImage>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QSpotLight>
#include <Qt3DRender/QRenderCapture>
#include <Qt3DRender/QRenderCaptureReply>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QTextureImage>
#include <Qt3DRender/QViewport>
#include <QUrl>
#endif

Q_LOGGING_CATEGORY(ca3DWidgetLog, "caqtdm.widgets.ca3dwidget")

namespace
{
constexpr qreal kOverlayMinTextureScale = 0.75;
constexpr qreal kOverlayMaxTextureScale = 2.0;
constexpr int kOverlayMaxTexturePixels = 1048576;
constexpr float kCameraMinPitchDegrees = -89.0f;
constexpr float kCameraMaxPitchDegrees = 89.0f;

QQuaternion rotationFromEuler(const QVector3D &rotation)
{
    return QQuaternion::fromEulerAngles(rotation.x(), rotation.y(), rotation.z());
}

QVector3D normalizedOrFallback(const QVector3D &vector, const QVector3D &fallback)
{
    return vector.lengthSquared() > 0.0f ? vector.normalized() : fallback;
}

void setVectorComponent(QVector3D *vector, ca3DBindingConfig::BindingTarget target, float value)
{
    if (!vector) {
        return;
    }

    switch (target) {
    case ca3DBindingConfig::TranslationX:
    case ca3DBindingConfig::RotationX:
        vector->setX(value);
        break;
    case ca3DBindingConfig::TranslationY:
    case ca3DBindingConfig::RotationY:
        vector->setY(value);
        break;
    case ca3DBindingConfig::TranslationZ:
    case ca3DBindingConfig::RotationZ:
        vector->setZ(value);
        break;
    case ca3DBindingConfig::LightEnabled:
    case ca3DBindingConfig::LightIntensity:
    case ca3DBindingConfig::LightDirectionX:
    case ca3DBindingConfig::LightDirectionY:
    case ca3DBindingConfig::LightDirectionZ:
    case ca3DBindingConfig::LightPositionX:
    case ca3DBindingConfig::LightPositionY:
    case ca3DBindingConfig::LightPositionZ:
    case ca3DBindingConfig::InvalidTarget:
        break;
    }
}

float vectorComponent(const QVector3D &vector, ca3DBindingConfig::BindingTarget target)
{
    switch (target) {
    case ca3DBindingConfig::TranslationX:
    case ca3DBindingConfig::RotationX:
        return vector.x();
    case ca3DBindingConfig::TranslationY:
    case ca3DBindingConfig::RotationY:
        return vector.y();
    case ca3DBindingConfig::TranslationZ:
    case ca3DBindingConfig::RotationZ:
        return vector.z();
    case ca3DBindingConfig::LightEnabled:
    case ca3DBindingConfig::LightIntensity:
    case ca3DBindingConfig::LightDirectionX:
    case ca3DBindingConfig::LightDirectionY:
    case ca3DBindingConfig::LightDirectionZ:
    case ca3DBindingConfig::LightPositionX:
    case ca3DBindingConfig::LightPositionY:
    case ca3DBindingConfig::LightPositionZ:
    case ca3DBindingConfig::InvalidTarget:
        return 0.0f;
    }
    return 0.0f;
}

bool bindingIsTranslation(ca3DBindingConfig::BindingTarget target)
{
    return target == ca3DBindingConfig::TranslationX
           || target == ca3DBindingConfig::TranslationY
           || target == ca3DBindingConfig::TranslationZ;
}

QPoint globalMousePosition(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->globalPosition().toPoint();
#else
    return event->globalPos();
#endif
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
                if (mouseEvent->button() == Qt::RightButton) {
                    const bool handled =
                        thisOverlayManager->sendContextMenuEvent(designPosition,
                                                                 globalMousePosition(mouseEvent),
                                                                 mouseEvent->modifiers());
                    if (handled) {
                        event->accept();
                        return true;
                    }
                }

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

bool pointIsInCameraView(const QMatrix4x4 &viewProjection, const QVector3D &point)
{
    const QVector4D clip = viewProjection * QVector4D(point, 1.0f);
    if (clip.w() <= 0.0f) {
        return false;
    }

    const QVector3D ndc = clip.toVector3DAffine();
    constexpr float margin = 1.05f;
    return ndc.x() >= -margin && ndc.x() <= margin
           && ndc.y() >= -margin && ndc.y() <= margin
           && ndc.z() >= -margin && ndc.z() <= margin;
}

bool overlayIsInCameraView(Qt3DRender::QCamera *camera, const ca3DOverlayConfig &overlay)
{
    if (!camera) {
        return false;
    }

    const float overlayWidth = static_cast<float>(overlay.size.width() > 0.0 ? overlay.size.width() : 1.5);
    const float overlayHeight = static_cast<float>(overlay.size.height() > 0.0 ? overlay.size.height() : 1.0);
    const QQuaternion overlayRotation = rotationFromEuler(overlay.rotation);
    const QVector3D right = normalizedOrFallback(overlayRotation.rotatedVector(QVector3D(1.0f, 0.0f, 0.0f)), QVector3D(1.0f, 0.0f, 0.0f)) * (overlayWidth * 0.5f);
    const QVector3D up = normalizedOrFallback(overlayRotation.rotatedVector(QVector3D(0.0f, 1.0f, 0.0f)), QVector3D(0.0f, 1.0f, 0.0f)) * (overlayHeight * 0.5f);
    const QMatrix4x4 viewProjection = camera->projectionMatrix() * camera->viewMatrix();
    const QVector3D corners[] = {
        overlay.position - right - up,
        overlay.position + right - up,
        overlay.position - right + up,
        overlay.position + right + up
    };

    for (const QVector3D &corner : corners) {
        if (pointIsInCameraView(viewProjection, corner)) {
            return true;
        }
    }
    return pointIsInCameraView(viewProjection, overlay.position);
}

QImage emptyOverlayTexture(const QSize &designSize)
{
    const QSize imageSize = designSize.expandedTo(QSize(1, 1));
    QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

qreal configuredOverlayMaxScale()
{
    bool ok = false;
    const qreal scale = QString::fromLocal8Bit(qgetenv("CAQTDM_3D_OVERLAY_MAX_SCALE")).toDouble(&ok);
    return ok && scale > 0.0 ? scale : kOverlayMaxTextureScale;
}

int configuredOverlayMaxPixels()
{
    const int pixels = qEnvironmentVariableIntValue("CAQTDM_3D_OVERLAY_MAX_PIXELS");
    return pixels > 0 ? pixels : kOverlayMaxTexturePixels;
}

void installLayeredFrameGraph(Qt3DExtras::Qt3DWindow *view,
                              Qt3DRender::QLayer *sceneLayer,
                              Qt3DRender::QLayer *overlayLayer,
                              const QColor &clearColor)
{
    if (!view || !sceneLayer || !overlayLayer) {
        return;
    }

    Qt3DRender::QRenderSurfaceSelector *surfaceSelector = new Qt3DRender::QRenderSurfaceSelector();
    surfaceSelector->setSurface(view);

    Qt3DRender::QViewport *viewport = new Qt3DRender::QViewport(surfaceSelector);
    viewport->setNormalizedRect(QRectF(0.0, 0.0, 1.0, 1.0));

    Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector(viewport);
    cameraSelector->setCamera(view->camera());

    Qt3DRender::QClearBuffers *sceneClear = new Qt3DRender::QClearBuffers(cameraSelector);
    sceneClear->setBuffers(Qt3DRender::QClearBuffers::ColorDepthBuffer);
    sceneClear->setClearColor(clearColor);

    Qt3DRender::QLayerFilter *sceneFilter = new Qt3DRender::QLayerFilter(sceneClear);
    sceneFilter->addLayer(sceneLayer);

    Qt3DRender::QClearBuffers *overlayDepthClear = new Qt3DRender::QClearBuffers(cameraSelector);
    overlayDepthClear->setBuffers(Qt3DRender::QClearBuffers::DepthBuffer);

    Qt3DRender::QLayerFilter *overlayFilter = new Qt3DRender::QLayerFilter(overlayDepthClear);
    overlayFilter->addLayer(overlayLayer);

    view->setActiveFrameGraph(surfaceSelector);
}
#endif

QSize uiRootDesignSize(const QString &uiFilePath)
{
    QFile file(uiFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QSize();
    }

    QXmlStreamReader reader(&file);
    bool inRootWidget = false;
    bool inGeometry = false;
    bool inWidth = false;
    bool inHeight = false;
    int depth = 0;
    int rootWidgetDepth = -1;
    int width = 0;
    int height = 0;

    while (!reader.atEnd()) {
        const QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            depth++;
            const auto name = reader.name();
            if (!inRootWidget && name == QLatin1String("widget")) {
                inRootWidget = true;
                rootWidgetDepth = depth;
            } else if (inRootWidget && depth == rootWidgetDepth + 1 && name == QLatin1String("property")
                       && reader.attributes().value(QLatin1String("name")) == QLatin1String("geometry")) {
                inGeometry = true;
            } else if (inGeometry && name == QLatin1String("width")) {
                inWidth = true;
            } else if (inGeometry && name == QLatin1String("height")) {
                inHeight = true;
            }
        } else if (token == QXmlStreamReader::Characters) {
            if (inWidth) {
                width = reader.text().toInt();
            } else if (inHeight) {
                height = reader.text().toInt();
            }
        } else if (token == QXmlStreamReader::EndElement) {
            const auto name = reader.name();
            if (name == QLatin1String("width")) {
                inWidth = false;
            } else if (name == QLatin1String("height")) {
                inHeight = false;
            } else if (name == QLatin1String("property") && inGeometry) {
                inGeometry = false;
                if (width > 0 && height > 0) {
                    return QSize(width, height);
                }
            } else if (name == QLatin1String("widget") && depth == rootWidgetDepth) {
                break;
            }
            depth--;
        }
    }

    return QSize();
}

QRect fallbackOverlayGeometry(QWidget *fallbackView, const ca3DOverlayConfig &overlay)
{
    if (!overlay.fallbackGeometry.isEmpty()) {
        return overlay.fallbackGeometry;
    }

    const QSize fallbackSize = fallbackView ? fallbackView->size().expandedTo(QSize(120, 80)) : QSize(640, 480);
    const QSize designSize = uiRootDesignSize(overlay.includeFileResolved);
    if (designSize.isEmpty()) {
        return QRect(QPoint(0, 0), fallbackSize);
    }

    QSize fittedSize = designSize;
    fittedSize.scale(fallbackSize, Qt::KeepAspectRatio);
    const QPoint topLeft((fallbackSize.width() - fittedSize.width()) / 2,
                         (fallbackSize.height() - fittedSize.height()) / 2);
    return QRect(topLeft, fittedSize);
}

void markFallbackOverlayOwner(QWidget *rootWidget, QObject *owner)
{
    if (!rootWidget || !owner) {
        return;
    }

    rootWidget->setProperty("ca3DOverlayOwner", QVariant::fromValue(owner));
    const QList<QWidget *> children = rootWidget->findChildren<QWidget *>();
    for (QWidget *child : children) {
        child->setProperty("ca3DOverlayOwner", QVariant::fromValue(owner));
    }
}

void applyFallbackWidgetScale(QGraphicsView *view, QWidget *rootWidget, const ca3DOverlayConfig &overlay, const QRect &geometry)
{
    if (!view || !rootWidget || geometry.isEmpty()) {
        return;
    }

    const QSize designSize = uiRootDesignSize(overlay.includeFileResolved);
    if (designSize.isEmpty()) {
        return;
    }

    const double scaleX = static_cast<double>(geometry.width()) / designSize.width();
    const double scaleY = static_cast<double>(geometry.height()) / designSize.height();

    view->setGeometry(geometry);
    view->setSceneRect(QRectF(QPointF(0.0, 0.0), QSizeF(designSize)));
    if (QGraphicsScene *scene = view->scene()) {
        const QList<QGraphicsItem *> items = scene->items();
        for (QGraphicsItem *item : items) {
            if (QGraphicsProxyWidget *proxy = qgraphicsitem_cast<QGraphicsProxyWidget *>(item)) {
                proxy->setPos(0.0, 0.0);
                proxy->setScale(qMin(scaleX, scaleY));
            }
        }
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool projectToSurface(const QMatrix4x4 &viewProjection,
                      const QVector3D &point,
                      const QSize &surfaceSize,
                      QPointF *surfacePoint)
{
    const QVector4D clip = viewProjection * QVector4D(point, 1.0f);
    if (clip.w() <= 0.0f) {
        return false;
    }

    const QVector3D ndc = clip.toVector3DAffine();
    const qreal x = (static_cast<qreal>(ndc.x()) * 0.5 + 0.5) * surfaceSize.width();
    const qreal y = (0.5 - static_cast<qreal>(ndc.y()) * 0.5) * surfaceSize.height();
    *surfacePoint = QPointF(x, y);
    return true;
}

qreal overlayTextureScale(Qt3DRender::QCamera *camera,
                          const ca3DOverlayConfig &overlay,
                          const QSize &designSize,
                          const QSize &surfaceSize)
{
    if (!camera || designSize.isEmpty() || surfaceSize.isEmpty()) {
        return 1.0;
    }

    const float overlayWidth = static_cast<float>(overlay.size.width() > 0.0 ? overlay.size.width() : 1.5);
    const float overlayHeight = static_cast<float>(overlay.size.height() > 0.0 ? overlay.size.height() : 1.0);
    const QQuaternion overlayRotation = rotationFromEuler(overlay.rotation);
    const QVector3D right = normalizedOrFallback(overlayRotation.rotatedVector(QVector3D(1.0f, 0.0f, 0.0f)), QVector3D(1.0f, 0.0f, 0.0f)) * (overlayWidth * 0.5f);
    const QVector3D up = normalizedOrFallback(overlayRotation.rotatedVector(QVector3D(0.0f, 1.0f, 0.0f)), QVector3D(0.0f, 1.0f, 0.0f)) * (overlayHeight * 0.5f);
    const QMatrix4x4 viewProjection = camera->projectionMatrix() * camera->viewMatrix();
    const QVector3D corners[] = {
        overlay.position - right - up,
        overlay.position + right - up,
        overlay.position - right + up,
        overlay.position + right + up
    };

    bool havePoint = false;
    qreal minX = 0.0;
    qreal maxX = 0.0;
    qreal minY = 0.0;
    qreal maxY = 0.0;
    for (const QVector3D &corner : corners) {
        QPointF point;
        if (!projectToSurface(viewProjection, corner, surfaceSize, &point)) {
            continue;
        }
        if (!havePoint) {
            minX = maxX = point.x();
            minY = maxY = point.y();
            havePoint = true;
        } else {
            minX = qMin(minX, point.x());
            maxX = qMax(maxX, point.x());
            minY = qMin(minY, point.y());
            maxY = qMax(maxY, point.y());
        }
    }
    if (!havePoint) {
        return 1.0;
    }

    const qreal projectedScale = qMax((maxX - minX) / designSize.width(),
                                      (maxY - minY) / designSize.height());
    qreal scale = qBound(kOverlayMinTextureScale, projectedScale, configuredOverlayMaxScale());
    const int maxPixels = configuredOverlayMaxPixels();
    const qreal scaledPixels = designSize.width() * scale * designSize.height() * scale;
    if (scaledPixels > maxPixels) {
        scale *= qSqrt(maxPixels / scaledPixels);
    }
    return qMax(0.25, scale);
}

QVector3D cameraForward(Qt3DRender::QCamera *camera)
{
    return normalizedOrFallback(camera->viewCenter() - camera->position(), QVector3D(0.0f, 0.0f, -1.0f));
}

QVector3D cameraRight(Qt3DRender::QCamera *camera)
{
    return normalizedOrFallback(QVector3D::crossProduct(cameraForward(camera), camera->upVector()), QVector3D(1.0f, 0.0f, 0.0f));
}

float cameraPitchDegrees(const QVector3D &forward)
{
    const QVector3D normalizedForward = normalizedOrFallback(forward, QVector3D(0.0f, 0.0f, -1.0f));
    return qRadiansToDegrees(qAsin(qBound(-1.0f, normalizedForward.y(), 1.0f)));
}

float cameraYawDegrees(const QVector3D &forward)
{
    const QVector3D normalizedForward = normalizedOrFallback(forward, QVector3D(0.0f, 0.0f, -1.0f));
    return qRadiansToDegrees(qAtan2(normalizedForward.x(), -normalizedForward.z()));
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
        forward = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), static_cast<float>(yawDelta)).rotatedVector(forward);
    }
    if (!qFuzzyIsNull(pitchDelta)) {
        const float currentPitch = cameraPitchDegrees(forward);
        const float targetPitch = qBound(kCameraMinPitchDegrees,
                                         currentPitch + static_cast<float>(pitchDelta),
                                         kCameraMaxPitchDegrees);
        const float allowedPitchDelta = targetPitch - currentPitch;
        const QVector3D right = normalizedOrFallback(QVector3D::crossProduct(forward, QVector3D(0.0f, 1.0f, 0.0f)),
                                                     QVector3D(1.0f, 0.0f, 0.0f));
        forward = QQuaternion::fromAxisAndAngle(right, allowedPitchDelta).rotatedVector(forward);
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
    , thisForce3DPreview(false)
    , thisSnapshotCapturePending(false)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    , this3DView(Q_NULLPTR)
    , thisRootEntity(Q_NULLPTR)
    , thisRenderCapture(Q_NULLPTR)
    , thisPendingCaptureReply(Q_NULLPTR)
#endif
{
    setMinimumSize(120, 80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(true);

    QPalette pal = palette();
    const QColor defaultBackground(30, 34, 40);
    pal.setColor(QPalette::Window, defaultBackground);
    pal.setColor(QPalette::WindowText, ca3DContrastingTextColor(defaultBackground));
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
    thisFallbackSnapshotLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    thisFallbackView->installEventFilter(this);
    layout->addWidget(thisFallbackView);

    updatePlaceholderText();
}

QSize ca3DWidget::sizeHint() const
{
    return QSize(640, 480);
}

QSize ca3DWidget::minimumSizeHint() const
{
    return QSize(120, 80);
}

ca3DWidget::~ca3DWidget()
{
    clearScene();
    clearFallbackView();
}

QVector3D ca3DWidget::currentCameraPosition() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (this3DView && this3DView->camera()) {
        return this3DView->camera()->position();
    }
#endif
    foreach (const ca3DCameraPresetConfig &preset, thisConfig.cameraPresets) {
        if (preset.id == thisCameraPreset) {
            return preset.position;
        }
    }
    return thisConfig.cameraPresets.isEmpty() ? QVector3D() : thisConfig.cameraPresets.first().position;
}

QVector3D ca3DWidget::currentCameraViewCenter() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (this3DView && this3DView->camera()) {
        return this3DView->camera()->viewCenter();
    }
#endif
    foreach (const ca3DCameraPresetConfig &preset, thisConfig.cameraPresets) {
        if (preset.id == thisCameraPreset) {
            return preset.hasViewCenter ? preset.viewCenter : preset.position;
        }
    }
    if (!thisConfig.cameraPresets.isEmpty()) {
        const ca3DCameraPresetConfig &preset = thisConfig.cameraPresets.first();
        return preset.hasViewCenter ? preset.viewCenter : preset.position;
    }
    return QVector3D();
}

QVector3D ca3DWidget::currentCameraUpVector() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (this3DView && this3DView->camera()) {
        return this3DView->camera()->upVector();
    }
#endif
    foreach (const ca3DCameraPresetConfig &preset, thisConfig.cameraPresets) {
        if (preset.id == thisCameraPreset) {
            return preset.upVector;
        }
    }
    return thisConfig.cameraPresets.isEmpty() ? QVector3D(0.0f, 1.0f, 0.0f) : thisConfig.cameraPresets.first().upVector;
}

QVector3D ca3DWidget::currentCameraRotation() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (this3DView && this3DView->camera()) {
        const QVector3D forward = this3DView->camera()->viewCenter() - this3DView->camera()->position();
        return QVector3D(cameraYawDegrees(forward), cameraPitchDegrees(forward), 0.0f);
    }
#endif
    foreach (const ca3DCameraPresetConfig &preset, thisConfig.cameraPresets) {
        if (preset.id == thisCameraPreset) {
            return QVector3D(static_cast<float>(preset.yaw), static_cast<float>(preset.pitch), 0.0f);
        }
    }
    if (!thisConfig.cameraPresets.isEmpty()) {
        const ca3DCameraPresetConfig &preset = thisConfig.cameraPresets.first();
        return QVector3D(static_cast<float>(preset.yaw), static_cast<float>(preset.pitch), 0.0f);
    }
    return QVector3D();
}

QVector3D ca3DWidget::currentObjectPosition(const QString &objectId) const
{
    foreach (const ca3DObjectConfig &object, thisConfig.objects) {
        if (object.id == objectId) {
            const QVector3D rotation = object.rotation
                                       + object.configuredOriginRotation
                                       + thisDynamicRotations.value(object.id);
            const QVector3D rotatedOrigin = rotationFromEuler(rotation).rotatedVector(object.configuredOriginPosition);
            return object.position
                   + thisDynamicTranslations.value(object.id)
                   + rotatedOrigin;
        }
    }
    return QVector3D();
}

QVector3D ca3DWidget::currentObjectRotation(const QString &objectId) const
{
    foreach (const ca3DObjectConfig &object, thisConfig.objects) {
        if (object.id == objectId) {
            return object.rotation
                   + object.configuredOriginRotation
                   + thisDynamicRotations.value(object.id);
        }
    }
    return QVector3D();
}

QVector3D ca3DWidget::effectiveObjectPosition(const QString &objectId) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    foreach (const ca3DObjectConfig &object, thisConfig.objects) {
        if (object.id == objectId) {
            QMap<QString, QMatrix4x4> motionCache;
            QSet<QString> visiting;
            const QMatrix4x4 matrix = effectiveObjectMotionMatrix(object, &motionCache, &visiting);
            return matrix.column(3).toVector3D();
        }
    }
#else
    Q_UNUSED(objectId);
#endif
    return QVector3D();
}

void ca3DWidget::setSceneConfig(const QString &config)
{
    if (thisSceneConfig == config) {
        return;
    }

    thisSceneConfig = config;
    thisConfigValid = ca3DConfigParser::parse(thisSceneConfig, &thisConfig, &thisConfigErrors);
    QPalette scenePalette = palette();
    scenePalette.setColor(QPalette::Window, thisConfig.backgroundColor);
    const QColor textColor = ca3DContrastingTextColor(thisConfig.backgroundColor);
    scenePalette.setColor(QPalette::WindowText, textColor);
    scenePalette.setColor(QPalette::Text, textColor);
    scenePalette.setColor(QPalette::ButtonText, textColor);
    setPalette(scenePalette);
    QPalette statusPalette = thisStatusLabel->palette();
    statusPalette.setColor(QPalette::Window, thisConfig.backgroundColor);
    statusPalette.setColor(QPalette::WindowText, textColor);
    statusPalette.setColor(QPalette::Text, textColor);
    thisStatusLabel->setPalette(statusPalette);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (this3DView) {
        this3DView->defaultFrameGraph()->setClearColor(thisConfig.backgroundColor);
    }
#endif
    rebuildObjectLinks();
    thisDynamicTranslations.clear();
    thisDynamicRotations.clear();
    thisDynamicLightDirections.clear();
    thisDynamicLightPositions.clear();
    thisDynamicLightIntensities.clear();
    thisDynamicLightEnabled.clear();
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
    if (thisFallbackMode) {
        for (QWidget *widget : thisFallbackOverlayRootWidgets) {
            if (widget) {
                roots.append(widget);
            }
        }
    }
    return roots;
}

QString ca3DWidget::overlayMacro(QWidget *rootWidget) const
{
    if (thisFallbackMode) {
        for (const ca3DOverlayConfig &overlay : thisConfig.overlays) {
            QWidget *widget = thisFallbackOverlayRootWidgets.value(overlay.id, Q_NULLPTR);
            if (widget == rootWidget) {
                return overlay.macro;
            }
        }
        return QString();
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    for (const ca3DOverlayConfig &overlay : thisConfig.overlays) {
        ca3DOverlayWidgetManager *manager = this3DOverlayManagersById.value(overlay.id, Q_NULLPTR);
        if (manager && manager->contentRoot() == rootWidget) {
            return overlay.macro;
        }
    }
#endif
    return QString();
}

QString ca3DWidget::overlayIncludePath(QWidget *rootWidget) const
{
    if (thisFallbackMode) {
        for (const ca3DOverlayConfig &overlay : thisConfig.overlays) {
            QWidget *widget = thisFallbackOverlayRootWidgets.value(overlay.id, Q_NULLPTR);
            if (widget == rootWidget) {
                const QFileInfo fileInfo(overlay.includeFileResolved.isEmpty() ? overlay.includeFile : overlay.includeFileResolved);
                if (!fileInfo.path().isEmpty()) {
                    return fileInfo.path() + QStringLiteral("/");
                }
            }
        }
        return QString();
    }
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

QStringList ca3DWidget::objectBindingChannels() const
{
    QStringList channels;
    for (const ca3DObjectConfig &object : thisConfig.objects) {
        for (const ca3DBindingConfig &binding : object.bindings) {
            channels.append(binding.channel);
        }
    }
    for (const ca3DLightConfig &light : thisConfig.lights) {
        for (const ca3DBindingConfig &binding : light.bindings) {
            channels.append(binding.channel);
        }
    }
    return channels;
}

void ca3DWidget::setForce3DPreview(bool enabled)
{
    thisForce3DPreview = enabled;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (enabled) {
        thisDesignerMode = false;
        thisFallbackMode = false;
        maybeInitialize3DView();
    }
#else
    Q_UNUSED(enabled);
#endif
}

QPixmap ca3DWidget::grab3DSnapshot(bool includeOverlays)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMap<QString, bool> overlayStates;
    if (!includeOverlays) {
        for (auto it = this3DOverlayEntities.begin(); it != this3DOverlayEntities.end(); ++it) {
            if (it.value()) {
                overlayStates.insert(it.key(), it.value()->isEnabled());
                it.value()->setEnabled(false);
            }
        }
    }

    qApp->processEvents();
    QPixmap pixmap;
    if (pixmap.isNull() && this3DView && this3DView->screen()) {
        pixmap = this3DView->screen()->grabWindow(this3DView->winId());
    }
    if (pixmap.isNull()) {
        pixmap = thisViewContainer ? thisViewContainer->grab() : grab();
    }

    for (auto it = overlayStates.begin(); it != overlayStates.end(); ++it) {
        Qt3DCore::QEntity *entity = this3DOverlayEntities.value(it.key(), Q_NULLPTR);
        if (entity) {
            entity->setEnabled(it.value());
        }
    }
    if (!includeOverlays) {
        apply3DOverlayVisibility(thisCameraPreset);
    }
    return pixmap;
#else
    Q_UNUSED(includeOverlays);
    return grab();
#endif
}

bool ca3DWidget::capture3DSnapshot(bool includeOverlays)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!this3DView || !this3DView->activeFrameGraph()) {
        emit snapshotCaptureFailed(tr("3D preview is not available"));
        return false;
    }
    if (thisSnapshotCapturePending) {
        emit snapshotCaptureFailed(tr("A 3D snapshot capture is already running"));
        return false;
    }

    if (!includeOverlays && this3DOverlayEntities.isEmpty()) {
        const QPixmap snapshot = grab3DSnapshot(false);
        if (snapshot.isNull()) {
            emit snapshotCaptureFailed(tr("3D snapshot capture returned an empty image"));
            return false;
        }
        emit snapshotCaptured(snapshot);
        return true;
    }

    thisSnapshotOverlayStates.clear();
    if (!includeOverlays) {
        for (auto it = this3DOverlayEntities.begin(); it != this3DOverlayEntities.end(); ++it) {
            if (it.value()) {
                thisSnapshotOverlayStates.insert(it.key(), it.value()->isEnabled());
                it.value()->setEnabled(false);
            }
        }
    }

    if (!thisRenderCapture) {
        thisRenderCapture = new Qt3DRender::QRenderCapture();
        Qt3DRender::QFrameGraphNode *activeFrameGraph = this3DView->activeFrameGraph();
        if (activeFrameGraph) {
            activeFrameGraph->setParent(thisRenderCapture);
        }
        this3DView->setActiveFrameGraph(thisRenderCapture);
    }

    const QSize captureSize = thisViewContainer ? thisViewContainer->size() : size();
    thisPendingCaptureReply = captureSize.isValid()
                                  ? thisRenderCapture->requestCapture(QRect(QPoint(0, 0), captureSize))
                                  : thisRenderCapture->requestCapture();
    if (!thisPendingCaptureReply) {
        restoreSnapshotOverlayStates();
        emit snapshotCaptureFailed(tr("Could not start 3D snapshot capture"));
        return false;
    }

    thisSnapshotCapturePending = true;
    connect(thisPendingCaptureReply, SIGNAL(completed()), this, SLOT(handleSnapshotCaptureCompleted()));
    QTimer::singleShot(3000, this, SLOT(handleSnapshotCaptureTimeout()));
    return true;
#else
    Q_UNUSED(includeOverlays);
    emit snapshotCaptured(grab());
    return true;
#endif
}

void ca3DWidget::handleSnapshotCaptureCompleted()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!thisSnapshotCapturePending || !thisPendingCaptureReply) {
        return;
    }

    Qt3DRender::QRenderCaptureReply *reply = thisPendingCaptureReply;
    thisPendingCaptureReply = Q_NULLPTR;
    thisSnapshotCapturePending = false;
    const QImage image = reply->image();
    reply->deleteLater();
    restoreSnapshotOverlayStates();

    if (image.isNull()) {
        emit snapshotCaptureFailed(tr("3D snapshot capture returned an empty image"));
        return;
    }
    emit snapshotCaptured(QPixmap::fromImage(image));
#endif
}

void ca3DWidget::handleSnapshotCaptureTimeout()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!thisSnapshotCapturePending) {
        return;
    }

    if (thisPendingCaptureReply) {
        thisPendingCaptureReply->deleteLater();
        thisPendingCaptureReply = Q_NULLPTR;
    }
    thisSnapshotCapturePending = false;
    restoreSnapshotOverlayStates();
    emit snapshotCaptureFailed(tr("3D snapshot capture timed out"));
#endif
}

void ca3DWidget::setCameraPreset(int preset)
{
    qCDebug(ca3DWidgetLog) << "setCameraPreset" << preset;
    if (preset < 0) {
        qCWarning(ca3DWidgetLog) << "setCameraPreset ignored negative preset" << preset;
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const int resolvedPreset = preset > 0 || thisConfig.cameraPresets.isEmpty()
                                   ? preset
                                   : thisConfig.cameraPresets.first().id;
    bool presetFound = thisConfig.cameraPresets.isEmpty();
    if (presetFound) {
        thisCameraPreset = resolvedPreset;
    }
    foreach (const ca3DCameraPresetConfig &cameraPreset, thisConfig.cameraPresets) {
        if (cameraPreset.id == resolvedPreset) {
            presetFound = true;
            thisCameraPreset = resolvedPreset;
            if (this3DView) {
                applyCameraPresetConfig(cameraPreset);
            } else {
                emitCameraPositionSignals(cameraPreset.position);
                emitCameraRotationSignals(cameraPreset.yaw, cameraPreset.pitch);
            }
            break;
        }
    }
    if (!presetFound) {
        qCWarning(ca3DWidgetLog) << "setCameraPreset ignored unknown preset" << preset;
        return;
    }
    apply3DOverlayVisibility(thisCameraPreset);
    qCDebug(ca3DWidgetLog) << "setCameraPreset applied" << resolvedPreset
                           << cameraDebugState(this3DView ? this3DView->camera() : Q_NULLPTR);
#else
    thisCameraPreset = preset > 0 || thisConfig.cameraPresets.isEmpty()
                           ? preset
                           : thisConfig.cameraPresets.first().id;
    foreach (const ca3DCameraPresetConfig &cameraPreset, thisConfig.cameraPresets) {
        if (cameraPreset.id == thisCameraPreset) {
            emitCameraPositionSignals(cameraPreset.position);
            emitCameraRotationSignals(cameraPreset.yaw, cameraPreset.pitch);
            break;
        }
    }
#endif
    applyFallbackPreset(thisCameraPreset);
    updatePlaceholderText();
}

void ca3DWidget::emitCameraPositionSignals(const QVector3D &position)
{
    emit cameraPositionXChanged(static_cast<double>(position.x()));
    emit cameraPositionXChanged(qRound(position.x()));
    emit cameraPositionYChanged(static_cast<double>(position.y()));
    emit cameraPositionYChanged(qRound(position.y()));
    emit cameraPositionZChanged(static_cast<double>(position.z()));
    emit cameraPositionZChanged(qRound(position.z()));
}

void ca3DWidget::emitCameraRotationSignals(double yaw, double pitch)
{
    emit cameraYawChanged(yaw);
    emit cameraYawChanged(qRound(yaw));
    emit cameraPitchChanged(pitch);
    emit cameraPitchChanged(qRound(pitch));
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
    const float clampedPitch = qBound(kCameraMinPitchDegrees,
                                      static_cast<float>(pitch),
                                      kCameraMaxPitchDegrees);
    const float pitchRadians = qDegreesToRadians(clampedPitch);
    const float cosPitch = qCos(pitchRadians);
    const QVector3D forward(qSin(yawRadians) * cosPitch,
                            qSin(pitchRadians),
                            -qCos(yawRadians) * cosPitch);

    Qt3DRender::QCamera *camera = this3DView->camera();
    const float viewDistance = qMax((camera->viewCenter() - camera->position()).length(), 1.0f);
    camera->setViewCenter(camera->position() + normalizedOrFallback(forward, QVector3D(0.0f, 0.0f, -1.0f)) * viewDistance);
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

bool ca3DWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == thisFallbackView && event->type() == QEvent::Resize) {
        thisFallbackSnapshotLabel->setGeometry(thisFallbackView->rect());
        applyFallbackPreset(thisCameraPreset);
    }
    bool watched3DView = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    watched3DView = watched == this3DView;
#endif
    if ((watched == thisFallbackView || watched == thisViewContainer || watched3DView)
            && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            emit customContextMenuRequested(mapFromGlobal(globalMousePosition(mouseEvent)));
            event->accept();
            return true;
        }
    }
    if ((watched == thisFallbackView || watched == thisViewContainer) && event->type() == QEvent::ContextMenu) {
        QContextMenuEvent *contextEvent = static_cast<QContextMenuEvent *>(event);
        emit customContextMenuRequested(mapFromGlobal(contextEvent->globalPos()));
        event->accept();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ca3DWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    update3DViewGeometry();
#endif
    if (thisFallbackSnapshotLabel && thisFallbackView) {
        thisFallbackSnapshotLabel->setGeometry(thisFallbackView->rect());
    }
    if (thisFallbackMode && thisFallbackView && thisFallbackView->isVisible()) {
        applyFallbackPreset(thisCameraPreset);
    }
}

void ca3DWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QTimer::singleShot(0, this, [this]() {
        if (isVisible()) {
            maybeInitialize3DView();
        }
    });
#else
    if (thisFallbackMode) {
        rebuildFallbackView();
    }
#endif
}

void ca3DWidget::setObjectAxisValue(const QString &objectId, const QString &axisId, double value)
{
    qCDebug(ca3DWidgetLog) << "setObjectAxisValue" << objectId << axisId << value;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    foreach (const ca3DObjectConfig &object, thisConfig.objects) {
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

void ca3DWidget::setLightBindingValue(const ca3DLightConfig &light,
                                      const ca3DBindingConfig &binding,
                                      double value)
{
    double mapped = value * binding.scale + binding.offset;
    if (binding.hasMinimum) mapped = qMax(mapped, binding.minimum);
    if (binding.hasMaximum) mapped = qMin(mapped, binding.maximum);
    if (binding.target == ca3DBindingConfig::LightEnabled) {
        thisDynamicLightEnabled[light.id] = mapped > 0.0;
    } else if (binding.target == ca3DBindingConfig::LightIntensity) {
        thisDynamicLightIntensities[light.id] = qMax(0.0, mapped);
    } else if (binding.target >= ca3DBindingConfig::LightDirectionX && binding.target <= ca3DBindingConfig::LightDirectionZ) {
        QVector3D direction = thisDynamicLightDirections.value(light.id, light.direction);
        setVectorComponent(&direction, static_cast<ca3DBindingConfig::BindingTarget>(binding.target - ca3DBindingConfig::LightDirectionX + ca3DBindingConfig::TranslationX), static_cast<float>(mapped));
        thisDynamicLightDirections[light.id] = direction;
    } else if (binding.target >= ca3DBindingConfig::LightPositionX && binding.target <= ca3DBindingConfig::LightPositionZ) {
        QVector3D position = thisDynamicLightPositions.value(light.id, light.position);
        setVectorComponent(&position, static_cast<ca3DBindingConfig::BindingTarget>(binding.target - ca3DBindingConfig::LightPositionX + ca3DBindingConfig::TranslationX), static_cast<float>(mapped));
        thisDynamicLightPositions[light.id] = position;
    }
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

void ca3DWidget::setObjectBindingValue(int bindingIndex, double value)
{
    qCDebug(ca3DWidgetLog) << "setObjectBindingValue" << bindingIndex << value;
    if (bindingIndex < 0) {
        qCWarning(ca3DWidgetLog) << "setObjectBindingValue ignored negative binding index" << bindingIndex;
        return;
    }

    int currentIndex = 0;
    foreach (const ca3DObjectConfig &object, thisConfig.objects) {
        foreach (const ca3DBindingConfig &binding, object.bindings) {
            if (currentIndex == bindingIndex) {
                setDynamicBindingComponent(object, binding, value);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                applyObjectTransform(object.id);
#endif
                qCDebug(ca3DWidgetLog) << "setObjectBindingValue applied" << bindingIndex << object.id << binding.targetName << value;
                return;
            }
            currentIndex++;
        }
    }

    foreach (const ca3DLightConfig &light, thisConfig.lights) {
        foreach (const ca3DBindingConfig &binding, light.bindings) {
            if (currentIndex == bindingIndex) {
                setLightBindingValue(light, binding, value);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                applyLight(light.id);
#endif
                qCDebug(ca3DWidgetLog) << "setObjectBindingValue applied light" << bindingIndex << light.id << binding.targetName << value;
                return;
            }
            currentIndex++;
        }
    }

    qCWarning(ca3DWidgetLog) << "setObjectBindingValue ignored unknown binding index" << bindingIndex;
}

void ca3DWidget::setDynamicBindingComponent(const ca3DObjectConfig &object, const ca3DBindingConfig &binding, double value)
{
    if (binding.target == ca3DBindingConfig::InvalidTarget) {
        return;
    }

    double mapped = value * binding.scale + binding.offset;
    if (binding.hasMinimum) {
        mapped = qMax(mapped, binding.minimum);
    }
    if (binding.hasMaximum) {
        mapped = qMin(mapped, binding.maximum);
    }

    const bool isTranslation = bindingIsTranslation(binding.target);
    QVector3D dynamicVector = isTranslation
                                  ? thisDynamicTranslations.value(object.id)
                                  : thisDynamicRotations.value(object.id);
    if (binding.mode == ca3DBindingConfig::Absolute) {
        QVector3D baseVector;
        if (isTranslation) {
            const QVector3D rotation = object.rotation
                                       + object.configuredOriginRotation
                                       + thisDynamicRotations.value(object.id);
            baseVector = object.position
                         + rotationFromEuler(rotation).rotatedVector(object.configuredOriginPosition);
        } else {
            baseVector = object.rotation + object.configuredOriginRotation;
        }
        mapped -= vectorComponent(baseVector, binding.target);
    }

    setVectorComponent(&dynamicVector, binding.target, static_cast<float>(mapped));
    if (isTranslation) {
        thisDynamicTranslations[object.id] = dynamicVector;
    } else {
        thisDynamicRotations[object.id] = dynamicVector;
    }
}

void ca3DWidget::updatePlaceholderText()
{
    const QString mode = thisDesignerMode
                             ? QStringLiteral("Designer")
                             : (thisFallbackMode ? QStringLiteral("2D fallback") : QStringLiteral("Qt 3D"));
    QString configState;
    if (thisSceneConfig.trimmed().isEmpty()) {
        configState = QStringLiteral("no sceneConfig");
    } else if (thisConfigValid) {
        configState = QStringLiteral("sceneConfig ok");
    } else {
        configState = QStringLiteral("sceneConfig errors:\n%1").arg(thisConfigErrors.join(QStringLiteral("\n")));
    }

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
    Qt3DRender::QLayer *sceneLayer = new Qt3DRender::QLayer(thisRootEntity);
    sceneLayer->setObjectName(QStringLiteral("ca3DSceneLayer"));
    Qt3DRender::QLayer *overlayLayer = new Qt3DRender::QLayer(thisRootEntity);
    overlayLayer->setObjectName(QStringLiteral("ca3DOverlayLayer"));
    this3DView->defaultFrameGraph()->setClearColor(thisConfig.backgroundColor);
    installLayeredFrameGraph(this3DView, sceneLayer, overlayLayer, thisConfig.backgroundColor);

    const auto materialAmbientColor = [this](const QColor &objectColor) {
        const QColor ambient = thisConfig.ambientLight.color;
        const double intensity = thisConfig.ambientLight.intensity;
        const auto component = [intensity](int sceneComponent, int objectComponent) {
            const double value = static_cast<double>(sceneComponent) * intensity * objectComponent / 255.0;
            return qRound(qBound(0.0, value, 255.0));
        };
        return QColor(component(ambient.red(), objectColor.red()),
                      component(ambient.green(), objectColor.green()),
                      component(ambient.blue(), objectColor.blue()));
    };

    if (thisConfig.lightingEnabled) {
        for (const ca3DLightConfig &lightConfig : thisConfig.lights) {
            Qt3DCore::QEntity *lightEntity = new Qt3DCore::QEntity(thisRootEntity);
            QObject *lightObject = Q_NULLPTR;
            if (lightConfig.type == ca3DLightConfig::Directional) {
                Qt3DRender::QDirectionalLight *light = new Qt3DRender::QDirectionalLight(lightEntity);
                lightObject = light;
                light->setColor(lightConfig.color);
                light->setIntensity(static_cast<float>(lightConfig.intensity));
                light->setWorldDirection(lightConfig.direction);
                lightEntity->addComponent(light);
            } else if (lightConfig.type == ca3DLightConfig::Point) {
                Qt3DRender::QPointLight *light = new Qt3DRender::QPointLight(lightEntity);
                lightObject = light;
                light->setColor(lightConfig.color);
                light->setIntensity(static_cast<float>(lightConfig.intensity));
                lightEntity->addComponent(light);
            } else {
                Qt3DRender::QSpotLight *light = new Qt3DRender::QSpotLight(lightEntity);
                lightObject = light;
                light->setColor(lightConfig.color);
                light->setIntensity(static_cast<float>(lightConfig.intensity));
                light->setLocalDirection(lightConfig.direction);
                light->setCutOffAngle(static_cast<float>(lightConfig.cutOffAngle));
                light->setConstantAttenuation(static_cast<float>(lightConfig.constantAttenuation));
                light->setLinearAttenuation(static_cast<float>(lightConfig.linearAttenuation));
                light->setQuadraticAttenuation(static_cast<float>(lightConfig.quadraticAttenuation));
                lightEntity->addComponent(light);
            }
            lightEntity->addComponent(sceneLayer);
            this3DLightObjects.insert(lightConfig.id, lightObject);
            if (lightConfig.type != ca3DLightConfig::Directional) {
                Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(lightEntity);
                transform->setTranslation(lightConfig.position);
                lightEntity->addComponent(transform);
                this3DLightTransforms.insert(lightConfig.id, transform);
            }
            applyLight(lightConfig.id);
        }
    }

    foreach (const ca3DObjectConfig &object, thisConfig.objects) {
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
            material->setAmbient(materialAmbientColor(object.hasMaterialColor ? object.materialColor : QColor(Qt::white)));
            entity->addComponent(material);
        } else {
            Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(entity);
            const QColor materialColor = object.hasMaterialColor ? object.materialColor : QColor(Qt::white);
            material->setAmbient(materialAmbientColor(materialColor));
            if (object.hasMaterialColor) {
                material->setDiffuse(object.materialColor);
            }
            entity->addComponent(material);
        }

        Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(entity);
        transform->setObjectName(QStringLiteral("ca3DObjectTransform_%1").arg(object.id));
        entity->addComponent(transform);
        entity->addComponent(sceneLayer);
        thisObjectTransforms.insert(object.id, transform);
    }
    applyAllObjectTransforms();

    rebuild3DOverlays(overlayLayer);

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
    QPalette fallbackPalette = thisFallbackView->palette();
    fallbackPalette.setColor(QPalette::Window, thisConfig.backgroundColor);
    const QColor textColor = ca3DContrastingTextColor(thisConfig.backgroundColor);
    fallbackPalette.setColor(QPalette::WindowText, textColor);
    fallbackPalette.setColor(QPalette::Text, textColor);
    fallbackPalette.setColor(QPalette::ButtonText, textColor);
    thisFallbackView->setAutoFillBackground(true);
    thisFallbackView->setPalette(fallbackPalette);
    thisFallbackSnapshotLabel->setPalette(fallbackPalette);
    thisFallbackView->show();
    thisFallbackSnapshotLabel->setGeometry(thisFallbackView->rect());
    thisFallbackSnapshotLabel->lower();

    foreach (const ca3DOverlayConfig &overlay, thisConfig.overlays) {
        if (overlay.includeFileResolved.isEmpty()) {
            continue;
        }

        QFile file(overlay.includeFileResolved);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        QUiLoader loader;
        QWidget *fallbackRoot = loader.load(&file, Q_NULLPTR);
        file.close();
        if (!fallbackRoot) {
            continue;
        }

        fallbackRoot->setObjectName(QStringLiteral("ca3DOverlay_%1").arg(overlay.id));
        fallbackRoot->setWindowFlags(Qt::Widget);
        fallbackRoot->setAttribute(Qt::WA_TranslucentBackground, overlay.transparentBackground);
        fallbackRoot->setAutoFillBackground(!overlay.transparentBackground);
        fallbackRoot->ensurePolished();
        markFallbackOverlayOwner(fallbackRoot, this);

        QGraphicsView *fallbackView = new QGraphicsView(thisFallbackView);
        fallbackView->setObjectName(QStringLiteral("ca3DOverlayView_%1").arg(overlay.id));
        fallbackView->setFrameShape(QFrame::NoFrame);
        fallbackView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        fallbackView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        fallbackView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        fallbackView->setBackgroundBrush(Qt::transparent);
        fallbackView->setStyleSheet(QStringLiteral("background: transparent"));
        fallbackView->viewport()->setAutoFillBackground(false);

        QGraphicsScene *scene = new QGraphicsScene(fallbackView);
        fallbackView->setScene(scene);
        scene->addWidget(fallbackRoot);
        const QRect geometry = fallbackOverlayGeometry(thisFallbackView, overlay);
        applyFallbackWidgetScale(fallbackView, fallbackRoot, overlay, geometry);
        fallbackView->hide();
        thisFallbackOverlayWidgets.insert(overlay.id, fallbackView);
        thisFallbackOverlayRootWidgets.insert(overlay.id, fallbackRoot);
    }

    const int preset = thisCameraPreset > 0 || thisConfig.cameraPresets.isEmpty()
                           ? thisCameraPreset
                           : thisConfig.cameraPresets.first().id;
    applyFallbackPreset(preset);
    emit overlayWidgetsRebuilt();
}

void ca3DWidget::clearFallbackView()
{
    foreach (QWidget *widget, thisFallbackOverlayWidgets) {
        if (widget) {
            widget->hide();
            widget->deleteLater();
        }
    }
    thisFallbackOverlayWidgets.clear();
    thisFallbackOverlayRootWidgets.clear();
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

    const int resolvedPreset = preset > 0 || thisConfig.cameraPresets.isEmpty()
                                   ? preset
                                   : thisConfig.cameraPresets.first().id;
    const auto presetIt = std::find_if(
        thisConfig.cameraPresets.cbegin(),
        thisConfig.cameraPresets.cend(),
        [&resolvedPreset](const ca3DCameraPresetConfig &cameraPreset) {
            return cameraPreset.id == resolvedPreset;
        });

    const ca3DCameraPresetConfig *selectedPreset =
        presetIt != thisConfig.cameraPresets.cend()
            ? &*presetIt
            : nullptr;

    if (!thisConfig.cameraPresets.isEmpty() && !selectedPreset) {
        return;
    }

    if (selectedPreset && !selectedPreset->snapshotResolved.isEmpty()) {
        if (thisFallbackSnapshotPixmap.load(selectedPreset->snapshotResolved)) {
            thisFallbackSnapshotLabel->setGeometry(thisFallbackView->rect());
            thisFallbackSnapshotLabel->setPixmap(thisFallbackSnapshotPixmap);
            thisFallbackSnapshotLabel->show();
            thisFallbackSnapshotLabel->lower();
        } else {
            qCWarning(ca3DWidgetLog) << "applyFallbackPreset could not load snapshot" << selectedPreset->snapshotResolved;
            thisFallbackSnapshotPixmap = QPixmap();
            thisFallbackSnapshotLabel->clear();
        }
    } else {
        thisFallbackSnapshotPixmap = QPixmap();
        if (thisFallbackSnapshotLabel)
            thisFallbackSnapshotLabel->clear();
    }

    foreach (QWidget *widget, thisFallbackOverlayWidgets) {
        if (widget) {
            widget->hide();
        }
    }

    foreach (const ca3DOverlayConfig &overlay, thisConfig.overlays) {
        QWidget *widget = thisFallbackOverlayWidgets.value(overlay.id, Q_NULLPTR);
        if (!widget) {
            continue;
        }

        bool visible = thisConfig.cameraPresets.isEmpty();
        if (selectedPreset) {
            visible = selectedPreset->overlays.contains(overlay.id);
        }

        if (visible) {
            const QRect geometry = fallbackOverlayGeometry(thisFallbackView, overlay);
            widget->setGeometry(geometry);
            if (QGraphicsView *view = qobject_cast<QGraphicsView *>(widget)) {
                applyFallbackWidgetScale(view, thisFallbackOverlayRootWidgets.value(overlay.id, Q_NULLPTR), overlay, geometry);
            }
            widget->show();
            widget->raise();
        }
    }
}

bool ca3DWidget::isDesignerMode() const
{
    if (thisForce3DPreview) {
        return false;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && !defined(MOBILE)
    // Qt Designer preview creates a normal top-level widget. Only the editable
    // form surface has a QDesignerFormWindowInterface and must avoid native
    // QWindow containers because they break drag/drop and selection handling.
    return QDesignerFormWindowInterface::findFormWindow(const_cast<ca3DWidget *>(this)) != Q_NULLPTR;
#else
    return false;
#endif
}

void ca3DWidget::applyLight(const QString &lightId)
{
    const int index = std::distance(thisConfig.lights.begin(), std::find_if(thisConfig.lights.begin(), thisConfig.lights.end(),
        [&lightId](const ca3DLightConfig &light) { return light.id == lightId; }));
    if (index < 0 || index >= thisConfig.lights.size()) return;
    const ca3DLightConfig &config = thisConfig.lights.at(index);
    QObject *object = this3DLightObjects.value(lightId, Q_NULLPTR);
    if (!object) return;
    const bool enabled = thisConfig.lightingEnabled && thisDynamicLightEnabled.value(lightId, config.enabled);
    object->setProperty("enabled", enabled);
    const double intensity = thisDynamicLightIntensities.value(lightId, config.intensity);
    const QVector3D direction = thisDynamicLightDirections.value(lightId, config.direction);
    const QVector3D position = thisDynamicLightPositions.value(lightId, config.position);
    if (Qt3DRender::QDirectionalLight *light = qobject_cast<Qt3DRender::QDirectionalLight *>(object)) {
        light->setIntensity(static_cast<float>(intensity));
        light->setWorldDirection(direction);
    } else if (Qt3DRender::QSpotLight *light = qobject_cast<Qt3DRender::QSpotLight *>(object)) {
        light->setIntensity(static_cast<float>(intensity));
        light->setLocalDirection(direction);
    } else if (Qt3DRender::QPointLight *light = qobject_cast<Qt3DRender::QPointLight *>(object)) {
        light->setIntensity(static_cast<float>(intensity));
    }
    if (Qt3DCore::QTransform *transform = this3DLightTransforms.value(lightId, Q_NULLPTR))
        transform->setTranslation(position);
}

void ca3DWidget::clearScene()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    clear3DOverlays();
    thisObjectTransforms.clear();
    this3DLightObjects.clear();
    this3DLightTransforms.clear();
    if (this3DView) {
        this3DView->setRootEntity(Q_NULLPTR);
    }
    if (thisRootEntity) {
        thisRootEntity->deleteLater();
    }
    thisRootEntity = Q_NULLPTR;
#endif
}

void ca3DWidget::rebuildObjectLinks()
{
    thisObjectIndexById.clear();
    thisObjectChildrenById.clear();

    for (int index = 0; index < thisConfig.objects.size(); ++index) {
        const ca3DObjectConfig &object = thisConfig.objects.at(index);
        if (!object.id.isEmpty()) {
            thisObjectIndexById.insert(object.id, index);
        }
    }

    foreach (const ca3DObjectConfig &object, thisConfig.objects) {
        if (!object.id.isEmpty() && thisObjectIndexById.contains(object.masterObjectId)) {
            thisObjectChildrenById[object.masterObjectId].append(object.id);
        }
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void ca3DWidget::restoreSnapshotOverlayStates()
{
    for (auto it = thisSnapshotOverlayStates.begin(); it != thisSnapshotOverlayStates.end(); ++it) {
        Qt3DCore::QEntity *entity = this3DOverlayEntities.value(it.key(), Q_NULLPTR);
        if (entity) {
            entity->setEnabled(it.value());
        }
    }
    if (!thisSnapshotOverlayStates.isEmpty()) {
        apply3DOverlayVisibility(thisCameraPreset);
    }
    thisSnapshotOverlayStates.clear();
}

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
    thisStatusLabel->hide();
    thisFallbackView->hide();

    this3DView = new Qt3DExtras::Qt3DWindow();
    this3DView->defaultFrameGraph()->setClearColor(thisConfig.backgroundColor);
    Qt3DRender::QCamera *camera = this3DView->camera();
    connect(camera, &Qt3DRender::QCamera::positionChanged, this, [this](const QVector3D &position) {
        emitCameraPositionSignals(position);
    });
    connect(camera, &Qt3DRender::QCamera::viewCenterChanged, this, [this, camera](const QVector3D &) {
        const QVector3D forward = camera->viewCenter() - camera->position();
        emitCameraRotationSignals(cameraYawDegrees(forward), cameraPitchDegrees(forward));
    });
    thisViewContainer = QWidget::createWindowContainer(this3DView, this);
    thisViewContainer->setMinimumSize(QSize(120, 80));
    thisViewContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    thisViewContainer->setFocusPolicy(Qt::StrongFocus);
    thisViewContainer->installEventFilter(this);
    this3DView->installEventFilter(this);
    if (QVBoxLayout *boxLayout = qobject_cast<QVBoxLayout *>(layout())) {
        boxLayout->addWidget(thisViewContainer, 1);
    } else {
        layout()->addWidget(thisViewContainer);
    }
    update3DViewGeometry();
    QTimer::singleShot(0, this, &ca3DWidget::update3DViewGeometry);
}

void ca3DWidget::update3DViewGeometry()
{
    if (!this3DView || !thisViewContainer) {
        return;
    }

    const QSize viewSize = thisViewContainer->size();

    Qt3DRender::QCamera *camera = this3DView->camera();
    if (camera && viewSize.width() > 0 && viewSize.height() > 0) {
        camera->lens()->setAspectRatio(static_cast<float>(viewSize.width()) / static_cast<float>(viewSize.height()));
        apply3DOverlayVisibility(thisCameraPreset);
    }
}

bool ca3DWidget::shouldUse2DFallback() const
{
    if (qEnvironmentVariableIntValue("CAQTDM_3D_FORCE_FALLBACK") > 0) {
        qCWarning(ca3DWidgetLog) << "shouldUse2DFallback forced by CAQTDM_3D_FORCE_FALLBACK";
        return true;
    }

    if (thisForce3DPreview) {
        return false;
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

void ca3DWidget::rebuild3DOverlays(Qt3DRender::QLayer *overlayLayer)
{
    clear3DOverlays();
    if (!thisRootEntity || !this3DView || !thisViewContainer || !overlayLayer) {
        return;
    }

    foreach (const ca3DOverlayConfig &overlay, thisConfig.overlays) {
        if (overlay.includeFileResolved.isEmpty()) {
            continue;
        }

        ca3DOverlayWidgetManager *overlayManager = new ca3DOverlayWidgetManager(this);
        overlayManager->loadWidgetsFromUi(overlay.includeFileResolved);
        markFallbackOverlayOwner(overlayManager, this);
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
        overlayManager->setProperty("ca3DOverlayActive", false);
        LiveWidgetTextureImage *textureImage = new LiveWidgetTextureImage(
            emptyOverlayTexture(designSize).flipped(Qt::Vertical), texture);
        texture->addTextureImage(textureImage);
        textureImage->update();

        QTimer *liveTextureTimer = new QTimer(overlayManager);
        liveTextureTimer->setTimerType(Qt::CoarseTimer);
        const std::shared_ptr<int> textureTimerTick(new int(0));
        connect(liveTextureTimer, &QTimer::timeout, overlayManager,
                [overlayManager, textureImage, textureTimerTick, renderWindow = this3DView, overlay, designSize]() {
                    if (!overlayManager->property("ca3DOverlayActive").toBool()) {
                        return;
                    }

                    ++(*textureTimerTick);
                    const bool refreshForCaret = overlayManager->hasFocusedTextInput() && (*textureTimerTick % 5 == 0);
                    if (!overlayManager->takeTextureDirty() && !refreshForCaret) {
                        return;
                    }
                    const QSize surfaceSize = renderWindow ? renderWindow->size() : QSize();
                    const qreal scale = overlayTextureScale(renderWindow ? renderWindow->camera() : Q_NULLPTR,
                                                            overlay,
                                                            designSize,
                                                            surfaceSize);
                    textureImage->setImage(overlayManager->renderSnapshot(scale).flipped(Qt::Vertical));
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
        overlayEntity->addComponent(overlayLayer);

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
    foreach (QObject *filter, this3DOverlayEventFilters) {
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

    foreach (ca3DOverlayWidgetManager *manager, this3DOverlayManagers) {
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
    foreach (const ca3DCameraPresetConfig &cameraPreset, thisConfig.cameraPresets) {
        if (cameraPreset.id == preset) {
            selectedPreset = &cameraPreset;
            break;
        }
    }

    if (!thisConfig.cameraPresets.isEmpty() && !selectedPreset) {
        return;
    }

    foreach (const ca3DOverlayConfig &overlay, thisConfig.overlays) {
        Qt3DCore::QEntity *entity = this3DOverlayEntities.value(overlay.id, Q_NULLPTR);
        if (!entity) {
            continue;
        }

        const bool presetMatches = selectedPreset && selectedPreset->overlays.contains(overlay.id);
        const bool inView = overlayIsInCameraView(this3DView ? this3DView->camera() : Q_NULLPTR, overlay);

        bool visible = false;
        if (overlay.visibilityMode == ca3DOverlayConfig::AlwaysWhenInView) {
            visible = inView;
        } else if (overlay.visibilityMode == ca3DOverlayConfig::InView) {
            visible = (thisConfig.cameraPresets.isEmpty() || presetMatches) && inView;
        } else {
            visible = (thisConfig.cameraPresets.isEmpty() || presetMatches) && inView;
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
        if (manager) {
            manager->setProperty("ca3DOverlayActive", visible);
            if (visible) {
                manager->markTextureDirty();
            } else if (!visible) {
                manager->clearOverlayFocus();
            }
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
    const QSize viewSize = thisViewContainer ? thisViewContainer->size().expandedTo(QSize(120, 80)) : QSize(640, 480);
    const float aspectRatio = viewSize.height() > 0
                                  ? static_cast<float>(viewSize.width()) / static_cast<float>(viewSize.height())
                                  : 16.0f / 9.0f;
    camera->setPosition(preset.position);
    camera->setViewCenter(preset.hasViewCenter ? preset.viewCenter : preset.position + forward * 100.0f);
    camera->setUpVector(normalizedOrFallback(preset.upVector, QVector3D(0.0f, 1.0f, 0.0f)));
    camera->lens()->setPerspectiveProjection(static_cast<float>(preset.fov), aspectRatio, 0.1f, 100000.0f);
    qCDebug(ca3DWidgetLog) << "applyCameraPresetConfig" << preset.id
                           << "hasViewCenter" << preset.hasViewCenter
                           << "fov" << preset.fov
                           << cameraDebugState(camera);
}

void ca3DWidget::applyObjectTransform(const QString &objectId)
{
    if (!thisObjectIndexById.contains(objectId)) {
        applyAllObjectTransforms();
        return;
    }

    QMap<QString, QMatrix4x4> motionCache;
    QSet<QString> affected;
    QList<QString> pending;
    pending.append(objectId);
    while (!pending.isEmpty()) {
        const QString currentId = pending.takeLast();
        if (affected.contains(currentId)) {
            continue;
        }
        affected.insert(currentId);

        const ca3DObjectConfig &object = thisConfig.objects.at(thisObjectIndexById.value(currentId));
        Qt3DCore::QTransform *transform = thisObjectTransforms.value(currentId, Q_NULLPTR);
        if (transform) {
            QSet<QString> visiting;
            QMatrix4x4 renderMatrix = effectiveObjectMotionMatrix(object, &motionCache, &visiting);
            renderMatrix.scale(static_cast<float>(object.scale));
            transform->setMatrix(renderMatrix);
        }

        pending.append(thisObjectChildrenById.value(currentId));
    }
}

void ca3DWidget::applyAllObjectTransforms()
{
    QMap<QString, QMatrix4x4> motionCache;
    foreach (const ca3DObjectConfig &object, thisConfig.objects) {
        Qt3DCore::QTransform *transform = thisObjectTransforms.value(object.id, Q_NULLPTR);
        if (!transform) {
            continue;
        }

        QSet<QString> visiting;
        QMatrix4x4 renderMatrix = effectiveObjectMotionMatrix(object, &motionCache, &visiting);
        renderMatrix.scale(static_cast<float>(object.scale));
        transform->setMatrix(renderMatrix);
    }
}

QMatrix4x4 ca3DWidget::objectMotionMatrix(const ca3DObjectConfig &object, bool includeDynamic) const
{
    const QVector3D translation = object.position
                                  + (includeDynamic ? thisDynamicTranslations.value(object.id) : QVector3D());
    const QVector3D rotation = object.rotation
                               + object.configuredOriginRotation
                               + (includeDynamic ? thisDynamicRotations.value(object.id) : QVector3D());
    QMatrix4x4 matrix;
    matrix.translate(translation);
    matrix.rotate(rotationFromEuler(rotation));
    matrix.translate(object.configuredOriginPosition);
    return matrix;
}

QMatrix4x4 ca3DWidget::effectiveObjectMotionMatrix(const ca3DObjectConfig &object,
                                                   QMap<QString, QMatrix4x4> *cache,
                                                   QSet<QString> *visiting) const
{
    if (cache && cache->contains(object.id)) {
        return cache->value(object.id);
    }
    if (!visiting || visiting->contains(object.id)) {
        qCWarning(ca3DWidgetLog) << "effectiveObjectMotionMatrix ignored cyclic object link" << object.id;
        return objectMotionMatrix(object, true);
    }

    QMatrix4x4 effective = objectMotionMatrix(object, true);
    if (!object.masterObjectId.isEmpty()) {
        visiting->insert(object.id);
        const int masterIndex = thisObjectIndexById.value(object.masterObjectId, -1);
        if (masterIndex >= 0) {
            const ca3DObjectConfig *masterObject = &thisConfig.objects.at(masterIndex);
            bool invertible = false;
            const QMatrix4x4 masterZeroInverse = objectMotionMatrix(*masterObject, false).inverted(&invertible);
            if (invertible) {
                effective = effectiveObjectMotionMatrix(*masterObject, cache, visiting) * masterZeroInverse * effective;
            } else {
                qCWarning(ca3DWidgetLog) << "effectiveObjectMotionMatrix ignored non-invertible master transform"
                                         << object.id << object.masterObjectId;
            }
        } else {
            qCWarning(ca3DWidgetLog) << "effectiveObjectMotionMatrix ignored unknown masterObject"
                                     << object.id << object.masterObjectId;
        }
        visiting->remove(object.id);
    }

    if (cache) {
        cache->insert(object.id, effective);
    }
    return effective;
}
#endif
