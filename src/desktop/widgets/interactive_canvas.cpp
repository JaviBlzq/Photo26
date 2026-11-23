#include "interactive_canvas.hpp"

#include <QCursor>
#include <QFont>
#include <QFontMetrics>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QUrl>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace photo26::desktop {

namespace {
constexpr double kMinZoom = 0.05;
constexpr double kMaxZoom = 16.0;
constexpr double kZoomStep = 1.2;
constexpr double kHandleHitRadius = 9.0;
constexpr int kMinCropSizePx = 8;
} // namespace

InteractiveCanvasWidget::InteractiveCanvasWidget(QWidget* parent)
    : QWidget(parent) {
    setMouseTracking(true);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
}

void InteractiveCanvasWidget::setPreviewImage(const QImage& processed) {
    const bool sizeChanged = (previewImage_.size() != processed.size());
    previewImage_ = processed;

    if (!previewImage_.isNull()) {
        if (cropRectImg_.isNull() || sizeChanged) {
            cropRectImg_ = QRect(0, 0, previewImage_.width(), previewImage_.height());
            if (cropAspectRatio_ > 0.0) {
                constrainCropRect(cropRectImg_, CropDragHandle::BottomRight);
            }
        } else {
            cropRectImg_ = cropRectImg_.intersected(
                QRect(0, 0, previewImage_.width(), previewImage_.height()));
        }

        if (autoFitPending_ || sizeChanged) {
            fitToWindow();
            autoFitPending_ = false;
        }
    }
    update();
}

void InteractiveCanvasWidget::setOriginalImage(const QImage& original) {
    originalImage_ = original;
    update();
}

void InteractiveCanvasWidget::clearImages() {
    previewImage_ = QImage();
    originalImage_ = QImage();
    cropRectImg_ = QRect();
    autoFitPending_ = true;
    panOffset_ = QPointF(0.0, 0.0);
    update();
}

void InteractiveCanvasWidget::setCheckerboardEnabled(bool enabled) {
    if (checkerboardEnabled_ == enabled) {
        return;
    }
    checkerboardEnabled_ = enabled;
    update();
}

void InteractiveCanvasWidget::zoomIn() {
    setZoomFactor(zoomFactor_ * kZoomStep);
}

void InteractiveCanvasWidget::zoomOut() {
    setZoomFactor(zoomFactor_ / kZoomStep);
}

void InteractiveCanvasWidget::zoomReset() {
    autoFitPending_ = false;
    panOffset_ = QPointF(0.0, 0.0);
    setZoomFactor(1.0);
}

void InteractiveCanvasWidget::fitToWindow() {
    if (previewImage_.isNull() || width() <= 16 || height() <= 16) {
        zoomFactor_ = 1.0;
        panOffset_ = QPointF(0.0, 0.0);
        update();
        return;
    }

    const double availW = std::max(32.0, static_cast<double>(width() - 36));
    const double availH = std::max(32.0, static_cast<double>(height() - 36));
    const double scaleX = availW / static_cast<double>(previewImage_.width());
    const double scaleY = availH / static_cast<double>(previewImage_.height());

    panOffset_ = QPointF(0.0, 0.0);
    zoomFactor_ = std::clamp(std::min(scaleX, scaleY), kMinZoom, kMaxZoom);
    emit zoomFactorChanged(zoomFactor_);
    update();
}

void InteractiveCanvasWidget::setZoomFactor(double factor) {
    const double clamped = std::clamp(factor, kMinZoom, kMaxZoom);
    if ( std::abs(zoomFactor_ - clamped) < 1e-5 ) {
        return;
    }
    zoomFactor_ = clamped;
    autoFitPending_ = false;
    emit zoomFactorChanged(zoomFactor_);
    update();
}

void InteractiveCanvasWidget::setSplitComparisonEnabled(bool enabled) {
    if (splitComparisonEnabled_ == enabled) {
        return;
    }
    splitComparisonEnabled_ = enabled;
    isDraggingSplit_ = false;
    update();
}

void InteractiveCanvasWidget::setSplitPosition(double ratio0to1) {
    const double clamped = std::clamp(ratio0to1, 0.02, 0.98);
    if (std::abs(splitPosition_ - clamped) < 1e-4) {
        return;
    }
    splitPosition_ = clamped;
    emit splitPositionChanged(splitPosition_);
    update();
}

void InteractiveCanvasWidget::setCropOverlayEnabled(bool enabled) {
    if (cropOverlayEnabled_ == enabled) {
        return;
    }
    cropOverlayEnabled_ = enabled;
    activeCropHandle_ = CropDragHandle::None;
    if (cropOverlayEnabled_ && !previewImage_.isNull() && cropRectImg_.isNull()) {
        cropRectImg_ = QRect(0, 0, previewImage_.width(), previewImage_.height());
        if (cropAspectRatio_ > 0.0) {
            constrainCropRect(cropRectImg_, CropDragHandle::BottomRight);
        }
    }
    update();
}

void InteractiveCanvasWidget::setCropRectImageCoords(const QRect& rect) {
    if (previewImage_.isNull()) {
        cropRectImg_ = rect;
        return;
    }
    QRect bounded = rect.normalized().intersected(
        QRect(0, 0, previewImage_.width(), previewImage_.height()));
    if (bounded.width() < kMinCropSizePx) {
        bounded.setWidth(std::min(kMinCropSizePx, previewImage_.width()));
    }
    if (bounded.height() < kMinCropSizePx) {
        bounded.setHeight(std::min(kMinCropSizePx, previewImage_.height()));
    }
    if (cropRectImg_ != bounded) {
        cropRectImg_ = bounded;
        update();
    }
}

void InteractiveCanvasWidget::setCropAspectRatio(double ratioOrZeroForFree) {
    cropAspectRatio_ = std::max(0.0, ratioOrZeroForFree);
    if (cropAspectRatio_ > 0.0 && !previewImage_.isNull() && !cropRectImg_.isNull()) {
        constrainCropRect(cropRectImg_, CropDragHandle::BottomRight);
        emit cropRectChanged(cropRectImg_);
        update();
    }
}

void InteractiveCanvasWidget::setShowRuleOfThirds(bool show) {
    if (showRuleOfThirds_ == show) {
        return;
    }
    showRuleOfThirds_ = show;
    update();
}

void InteractiveCanvasWidget::setColorPickerEnabled(bool enabled) {
    if (colorPickerEnabled_ == enabled) {
        return;
    }
    colorPickerEnabled_ = enabled;
    setCursor(colorPickerEnabled_ ? Qt::CrossCursor : Qt::ArrowCursor);
    update();
}

QRectF InteractiveCanvasWidget::computeImageSceneRect() const {
    if (previewImage_.isNull()) {
        return QRectF();
    }
    const double dispW = previewImage_.width() * zoomFactor_;
    const double dispH = previewImage_.height() * zoomFactor_;
    const double cx = width() * 0.5 + panOffset_.x();
    const double cy = height() * 0.5 + panOffset_.y();
    return QRectF(cx - dispW * 0.5, cy - dispH * 0.5, dispW, dispH);
}

QPointF InteractiveCanvasWidget::widgetToImageCoords(const QPointF& widgetPt) const {
    const QRectF sceneRect = computeImageSceneRect();
    if (sceneRect.isEmpty() || zoomFactor_ <= 0.0) {
        return QPointF(0.0, 0.0);
    }
    const double ix = (widgetPt.x() - sceneRect.left()) / zoomFactor_;
    const double iy = (widgetPt.y() - sceneRect.top()) / zoomFactor_;
    return QPointF(ix, iy);
}

QRectF InteractiveCanvasWidget::imageToWidgetRect(const QRect& imgRect) const {
    const QRectF sceneRect = computeImageSceneRect();
    if (sceneRect.isEmpty()) {
        return QRectF();
    }
    return QRectF(
        sceneRect.left() + imgRect.left() * zoomFactor_,
        sceneRect.top() + imgRect.top() * zoomFactor_,
        imgRect.width() * zoomFactor_,
        imgRect.height() * zoomFactor_);
}

InteractiveCanvasWidget::CropDragHandle InteractiveCanvasWidget::hitTestCropHandle(
    const QPointF& widgetPt) const {
    if (!cropOverlayEnabled_ || previewImage_.isNull() || cropRectImg_.isNull()) {
        return CropDragHandle::None;
    }

    const QRectF r = imageToWidgetRect(cropRectImg_);
    const QPointF tl = r.topLeft();
    const QPointF tr = r.topRight();
    const QPointF bl = r.bottomLeft();
    const QPointF br = r.bottomRight();
    const QPointF tm(r.center().x(), r.top());
    const QPointF bm(r.center().x(), r.bottom());
    const QPointF lm(r.left(), r.center().y());
    const QPointF rm(r.right(), r.center().y());

    auto nearPt = [&](const QPointF& target) {
        return QLineF(widgetPt, target).length() <= kHandleHitRadius;
    };

    if (nearPt(tl)) return CropDragHandle::TopLeft;
    if (nearPt(tr)) return CropDragHandle::TopRight;
    if (nearPt(bl)) return CropDragHandle::BottomLeft;
    if (nearPt(br)) return CropDragHandle::BottomRight;
    if (nearPt(tm)) return CropDragHandle::Top;
    if (nearPt(bm)) return CropDragHandle::Bottom;
    if (nearPt(lm)) return CropDragHandle::Left;
    if (nearPt(rm)) return CropDragHandle::Right;

    if (r.contains(widgetPt)) {
        return CropDragHandle::MoveInterior;
    }
    return CropDragHandle::None;
}

void InteractiveCanvasWidget::updateCursorForPosition(const QPointF& widgetPt) {
    if (colorPickerEnabled_) {
        setCursor(Qt::CrossCursor);
        return;
    }
    if (isPanning_ || spacePressed_) {
        setCursor(isPanning_ ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
        return;
    }

    if (splitComparisonEnabled_ && !previewImage_.isNull()) {
        const QRectF sceneRect = computeImageSceneRect();
        const double splitX = sceneRect.left() + sceneRect.width() * splitPosition_;
        if (std::abs(widgetPt.x() - splitX) <= 10.0 && sceneRect.contains(widgetPt)) {
            setCursor(Qt::SplitHCursor);
            return;
        }
    }

    if (cropOverlayEnabled_ && !previewImage_.isNull()) {
        switch (hitTestCropHandle(widgetPt)) {
            case CropDragHandle::TopLeft:
            case CropDragHandle::BottomRight:
                setCursor(Qt::SizeFDiagCursor);
                return;
            case CropDragHandle::TopRight:
            case CropDragHandle::BottomLeft:
                setCursor(Qt::SizeBDiagCursor);
                return;
            case CropDragHandle::Top:
            case CropDragHandle::Bottom:
                setCursor(Qt::SizeVerCursor);
                return;
            case CropDragHandle::Left:
            case CropDragHandle::Right:
                setCursor(Qt::SizeHorCursor);
                return;
            case CropDragHandle::MoveInterior:
                setCursor(Qt::SizeAllCursor);
                return;
            case CropDragHandle::None:
                break;
        }
    }

    setCursor(Qt::ArrowCursor);
}

void InteractiveCanvasWidget::constrainCropRect(QRect& rect, CropDragHandle handle) const {
    if (previewImage_.isNull()) {
        return;
    }
    const int imgW = previewImage_.width();
    const int imgH = previewImage_.height();

    rect = rect.normalized();
    if (rect.width() < kMinCropSizePx) rect.setWidth(std::min(kMinCropSizePx, imgW));
    if (rect.height() < kMinCropSizePx) rect.setHeight(std::min(kMinCropSizePx, imgH));

    if (handle == CropDragHandle::MoveInterior) {
        const int w = std::min(rect.width(), imgW);
        const int h = std::min(rect.height(), imgH);
        const int x = std::clamp(rect.left(), 0, std::max(0, imgW - w));
        const int y = std::clamp(rect.top(), 0, std::max(0, imgH - h));
        rect = QRect(x, y, w, h);
        return;
    }

    rect = rect.intersected(QRect(0, 0, imgW, imgH));
    if (rect.width() < kMinCropSizePx) rect.setWidth(std::min(kMinCropSizePx, imgW));
    if (rect.height() < kMinCropSizePx) rect.setHeight(std::min(kMinCropSizePx, imgH));

    if (cropAspectRatio_ > 0.0) {
        double w = static_cast<double>(rect.width());
        double h = w / cropAspectRatio_;
        if (h > imgH) {
            h = static_cast<double>(imgH);
            w = h * cropAspectRatio_;
        }
        if (w > imgW) {
            w = static_cast<double>(imgW);
            h = w / cropAspectRatio_;
        }
        const int finalW = std::max(1, qRound(w));
        const int finalH = std::max(1, qRound(h));
        const int finalX = std::clamp(rect.left(), 0, std::max(0, imgW - finalW));
        const int finalY = std::clamp(rect.top(), 0, std::max(0, imgH - finalH));
        rect = QRect(finalX, finalY, finalW, finalH);
    }
}

void InteractiveCanvasWidget::drawCheckerboard(QPainter& p, const QRectF& targetRect) const {
    const int tileSize = 12;
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QColor lightTile = dark ? QColor(46, 49, 54) : QColor(246, 247, 249);
    const QColor darkTile = dark ? QColor(34, 37, 41) : QColor(228, 231, 236);

    p.save();
    p.setClipRect(targetRect);

    const int startX = static_cast<int>( std::floor(targetRect.left()) );
    const int startY = static_cast<int>( std::floor(targetRect.top()) );
    const int endX = static_cast<int>( std::ceil(targetRect.right()) );
    const int endY = static_cast<int>( std::ceil(targetRect.bottom()) );

    int row = 0;
    for (int y = startY; y < endY; y += tileSize, ++row) {
        int col = 0;
        for (int x = startX; x < endX; x += tileSize, ++col) {
            p.fillRect(
                QRect(x, y, tileSize, tileSize),
                ((row + col) % 2 == 0) ? lightTile : darkTile);
        }
    }
    p.restore();
}

void InteractiveCanvasWidget::drawSplitOverlay(QPainter& p, const QRectF& targetRect) const {
    const double splitX = targetRect.left() + targetRect.width() * splitPosition_;

    p.save();
    // Draw original image on the left half if available
    if (!originalImage_.isNull()) {
        p.save();
        const QRectF leftClip(
            targetRect.left(),
            targetRect.top(),
            splitX - targetRect.left(),
            targetRect.height());
        p.setClipRect(leftClip);
        p.drawImage(targetRect, originalImage_);
        p.restore();
    }

    // Vertical split line
    p.setPen(QPen(QColor(255, 255, 255, 235), 2.0));
    p.drawLine(QPointF(splitX, targetRect.top()), QPointF(splitX, targetRect.bottom()));

    // Handle circle in the middle
    const QPointF handleCenter(splitX, targetRect.center().y());
    p.setBrush(QColor(45, 95, 138, 240));
    p.setPen(QPen(Qt::white, 2.0));
    p.drawEllipse(handleCenter, 13.0, 13.0);

    // Left/Right chevrons inside the handle
    QPainterPath arrows;
    arrows.moveTo(handleCenter.x() - 3.0, handleCenter.y() - 4.0);
    arrows.lineTo(handleCenter.x() - 7.0, handleCenter.y());
    arrows.lineTo(handleCenter.x() - 3.0, handleCenter.y() + 4.0);
    arrows.moveTo(handleCenter.x() + 3.0, handleCenter.y() - 4.0);
    arrows.lineTo(handleCenter.x() + 7.0, handleCenter.y());
    arrows.lineTo(handleCenter.x() + 3.0, handleCenter.y() + 4.0);
    p.drawPath(arrows);

    // ANTES / DESPUÉS badges
    QFont badgeFont = font();
    badgeFont.setBold(true);
    badgeFont.setPointSize(9);
    p.setFont(badgeFont);

    const QRectF beforeBadge(targetRect.left() + 12.0, targetRect.top() + 12.0, 74.0, 24.0);
    const QRectF afterBadge(targetRect.right() - 94.0, targetRect.top() + 12.0, 82.0, 24.0);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 35, 41, 200));
    if (beforeBadge.right() < splitX - 6.0) {
        p.drawRoundedRect(beforeBadge, 4.0, 4.0);
        p.setPen(Qt::white);
        p.drawText(beforeBadge, Qt::AlignCenter, QStringLiteral("ANTES"));
    }
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(45, 95, 138, 220));
    if (afterBadge.left() > splitX + 6.0) {
        p.drawRoundedRect(afterBadge, 4.0, 4.0);
        p.setPen(Qt::white);
        p.drawText(afterBadge, Qt::AlignCenter, QStringLiteral("DESPUÉS"));
    }

    p.restore();
}

void InteractiveCanvasWidget::drawCropOverlay(QPainter& p, const QRectF& targetRect) const {
    if (cropRectImg_.isNull()) {
        return;
    }

    const QRectF cropW = imageToWidgetRect(cropRectImg_);
    p.save();

    // Darken exterior around crop rect
    QPainterPath outerPath;
    outerPath.addRect(targetRect);
    QPainterPath innerPath;
    innerPath.addRect(cropW);
    const QPainterPath maskPath = outerPath.subtracted(innerPath);
    p.fillPath(maskPath, QColor(8, 11, 18, 165));

    // Rule of thirds lines
    if (showRuleOfThirds_) {
        p.setPen(QPen(QColor(255, 255, 255, 130), 1.0, Qt::DashLine));
        const double thirdW = cropW.width() / 3.0;
        const double thirdH = cropW.height() / 3.0;
        for (int i = 1; i <= 2; ++i) {
            p.drawLine(
                QPointF(cropW.left() + thirdW * i, cropW.top()),
                QPointF(cropW.left() + thirdW * i, cropW.bottom()));
            p.drawLine(
                QPointF(cropW.left(), cropW.top() + thirdH * i),
                QPointF(cropW.right(), cropW.top() + thirdH * i));
        }
    }

    // Crisp crop border
    p.setPen(QPen(QColor(45, 95, 138), 2.0));
    p.setBrush(Qt::NoBrush);
    p.drawRect(cropW);

    // 8 resize handles
    const QList<QPointF> handles = {
        cropW.topLeft(),
        QPointF(cropW.center().x(), cropW.top()),
        cropW.topRight(),
        QPointF(cropW.right(), cropW.center().y()),
        cropW.bottomRight(),
        QPointF(cropW.center().x(), cropW.bottom()),
        cropW.bottomLeft(),
        QPointF(cropW.left(), cropW.center().y())};

    p.setPen(QPen(QColor(32, 66, 98), 1.5));
    p.setBrush(Qt::white);
    for (const QPointF& pt : handles) {
        p.drawRoundedRect(QRectF(pt.x() - 4.5, pt.y() - 4.5, 9.0, 9.0), 2.0, 2.0);
    }

    // Dimensions pill badge
    const QString dimText = QStringLiteral("%1 × %2 px")
                                .arg(cropRectImg_.width())
                                .arg(cropRectImg_.height());
    QFont f = font();
    f.setBold(true);
    f.setPointSize(9);
    p.setFont(f);
    const QFontMetrics fm(f);
    const int textW = fm.horizontalAdvance(dimText) + 16;
    const QRectF pillRect(
        cropW.left() + 6.0,
        std::max(targetRect.top() + 4.0, cropW.top() - 26.0),
        textW,
        22.0);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(15, 23, 42, 215));
    p.drawRoundedRect(pillRect, 5.0, 5.0);
    p.setPen(Qt::white);
    p.drawText(pillRect, Qt::AlignCenter, dimText);

    p.restore();
}

void InteractiveCanvasWidget::drawEmptyPlaceholder(QPainter& p) const {
    p.save();
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QRectF box = rect().adjusted(28, 28, -28, -28);
    p.setPen(QPen(dark ? QColor(88, 95, 106) : QColor(186, 193, 204), 1.5, Qt::DashLine));
    p.setBrush(dark ? QColor(34, 37, 42, 160) : QColor(255, 255, 255, 200));
    p.drawRoundedRect(box, 8.0, 8.0);

    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(dark ? QColor(210, 214, 220) : QColor(56, 63, 72));
    p.drawText(
        box.adjusted(16, -16, -16, -16),
        Qt::AlignCenter,
        QStringLiteral("Arrastra imágenes aquí o usa «Añadir imágenes…»"));

    QFont subFont = font();
    subFont.setPointSize(9);
    p.setFont(subFont);
    p.setPen(dark ? QColor(145, 152, 162) : QColor(108, 117, 129));
    p.drawText(
        box.adjusted(16, 22, -16, 22),
        Qt::AlignCenter,
        QStringLiteral("Rueda del ratón: Zoom  •  Espacio + Arrastrar: Mover lienzo"));
    p.restore();
}

void InteractiveCanvasWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, zoomFactor_ < 2.0);

    const bool dark = palette().color(QPalette::Window).lightness() < 128;

    // Neutral canvas backdrop
    p.fillRect(rect(), dark ? QColor(24, 26, 29) : QColor(229, 232, 236));

    if (previewImage_.isNull()) {
        drawEmptyPlaceholder(p);
        return;
    }

    const QRectF sceneRect = computeImageSceneRect();
    if (checkerboardEnabled_) {
        drawCheckerboard(p, sceneRect);
    } else {
        p.fillRect(sceneRect, dark ? QColor(34, 37, 42) : QColor(255, 255, 255));
    }

    p.drawImage(sceneRect, previewImage_);

    if (splitComparisonEnabled_) {
        drawSplitOverlay(p, sceneRect);
    }

    if (cropOverlayEnabled_) {
        drawCropOverlay(p, sceneRect);
    }

    // Subtle border around image scene
    p.setPen(QPen(dark ? QColor(255, 255, 255, 40) : QColor(0, 0, 0, 45), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRect(sceneRect);
}

void InteractiveCanvasWidget::wheelEvent(QWheelEvent* event) {
    if (previewImage_.isNull()) {
        event->ignore();
        return;
    }

    const int delta = event->angleDelta().y();
    if (delta == 0) {
        return;
    }

    const double oldZoom = zoomFactor_;
    const double factor = (delta > 0) ? kZoomStep : (1.0 / kZoomStep);
    const double newZoom = std::clamp(oldZoom * factor, kMinZoom, kMaxZoom);

    if (std::abs(newZoom - oldZoom) > 1e-6) {
        // Keep point under mouse anchored during zoom
        const QPointF mousePos = event->position();
        const QPointF center(width() * 0.5, height() * 0.5);
        const QPointF rel = mousePos - center - panOffset_;
        panOffset_ += rel * (1.0 - (newZoom / oldZoom));
        zoomFactor_ = newZoom;
        autoFitPending_ = false;
        emit zoomFactorChanged(zoomFactor_);
        update();
    }
    event->accept();
}

void InteractiveCanvasWidget::mousePressEvent(QMouseEvent* event) {
    setFocus();
    lastMousePos_ = event->position();

    if (previewImage_.isNull()) {
        return;
    }

    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && spacePressed_)) {
        isPanning_ = true;
        updateCursorForPosition(event->position());
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (colorPickerEnabled_) {
            const QPointF imgPt = widgetToImageCoords(event->position());
            const int ix = qFloor(imgPt.x());
            const int iy = qFloor(imgPt.y());
            if (ix >= 0 && ix < previewImage_.width() &&
                iy >= 0 && iy < previewImage_.height()) {
                emit colorPicked(previewImage_.pixelColor(ix, iy));
            }
            event->accept();
            return;
        }

        if (splitComparisonEnabled_) {
            const QRectF sceneRect = computeImageSceneRect();
            const double splitX = sceneRect.left() + sceneRect.width() * splitPosition_;
            if (std::abs(event->position().x() - splitX) <= 12.0 &&
                sceneRect.contains(event->position())) {
                isDraggingSplit_ = true;
                event->accept();
                return;
            }
        }

        if (cropOverlayEnabled_) {
            const CropDragHandle handle = hitTestCropHandle(event->position());
            if (handle != CropDragHandle::None) {
                activeCropHandle_ = handle;
                cropDragStartWidgetPt_ = event->position();
                cropRectAtDragStart_ = cropRectImg_;
                event->accept();
                return;
            }
        }

        isPanning_ = true;
        updateCursorForPosition(event->position());
        event->accept();
    }
}

void InteractiveCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    const QPointF pos = event->position();

    if (isPanning_) {
        const QPointF delta = pos - lastMousePos_;
        panOffset_ += delta;
        lastMousePos_ = pos;
        update();
        event->accept();
        return;
    }

    if (isDraggingSplit_ && !previewImage_.isNull()) {
        const QRectF sceneRect = computeImageSceneRect();
        if (sceneRect.width() > 1.0) {
            const double ratio = (pos.x() - sceneRect.left()) / sceneRect.width();
            setSplitPosition(ratio);
        }
        event->accept();
        return;
    }

    if (activeCropHandle_ != CropDragHandle::None && !previewImage_.isNull() && zoomFactor_ > 0.0) {
        const QPointF deltaImg = (pos - cropDragStartWidgetPt_) / zoomFactor_;
        const int dx = qRound(deltaImg.x());
        const int dy = qRound(deltaImg.y());

        QRect next = cropRectAtDragStart_;
        switch (activeCropHandle_) {
            case CropDragHandle::MoveInterior:
                next.translate(dx, dy);
                break;
            case CropDragHandle::TopLeft:
                next.setLeft(next.left() + dx);
                next.setTop(next.top() + dy);
                break;
            case CropDragHandle::Top:
                next.setTop(next.top() + dy);
                break;
            case CropDragHandle::TopRight:
                next.setRight(next.right() + dx);
                next.setTop(next.top() + dy);
                break;
            case CropDragHandle::Right:
                next.setRight(next.right() + dx);
                break;
            case CropDragHandle::BottomRight:
                next.setRight(next.right() + dx);
                next.setBottom(next.bottom() + dy);
                break;
            case CropDragHandle::Bottom:
                next.setBottom(next.bottom() + dy);
                break;
            case CropDragHandle::BottomLeft:
                next.setLeft(next.left() + dx);
                next.setBottom(next.bottom() + dy);
                break;
            case CropDragHandle::Left:
                next.setLeft(next.left() + dx);
                break;
            case CropDragHandle::None:
                break;
        }

        constrainCropRect(next, activeCropHandle_);
        if (next != cropRectImg_) {
            cropRectImg_ = next;
            emit cropRectChanged(cropRectImg_);
            update();
        }
        event->accept();
        return;
    }

    lastMousePos_ = pos;
    updateCursorForPosition(pos);
}

void InteractiveCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    isPanning_ = false;
    isDraggingSplit_ = false;
    activeCropHandle_ = CropDragHandle::None;
    updateCursorForPosition(event->position());
    event->accept();
}

void InteractiveCanvasWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        spacePressed_ = true;
        updateCursorForPosition(lastMousePos_);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void InteractiveCanvasWidget::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        spacePressed_ = false;
        updateCursorForPosition(lastMousePos_);
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void InteractiveCanvasWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (autoFitPending_ && !previewImage_.isNull()) {
        fitToWindow();
        autoFitPending_ = false;
    }
}

void InteractiveCanvasWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData() && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void InteractiveCanvasWidget::dropEvent(QDropEvent* event) {
    if (!event->mimeData() || !event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    QStringList paths;
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl& url : urls) {
        if (url.isLocalFile()) {
            paths.append(url.toLocalFile());
        }
    }

    if (!paths.isEmpty()) {
        emit filesDropped(paths);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

} // namespace photo26::desktop
