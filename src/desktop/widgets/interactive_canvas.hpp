#pragma once

#include <QColor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QStringList>
#include <QWheelEvent>
#include <QWidget>

namespace photo26::desktop {

class InteractiveCanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit InteractiveCanvasWidget(QWidget* parent = nullptr);
    ~InteractiveCanvasWidget() override = default;

    // Image state
    void setPreviewImage(const QImage& processed);
    void setOriginalImage(const QImage& original);
    const QImage& previewImage() const noexcept { return previewImage_; }
    const QImage& originalImage() const noexcept { return originalImage_; }
    void clearImages();
    bool hasImage() const noexcept { return !previewImage_.isNull(); }

    // Checkerboard background for alpha channel preview
    void setCheckerboardEnabled(bool enabled);
    bool isCheckerboardEnabled() const noexcept { return checkerboardEnabled_; }

    // Zoom & Pan controls
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void fitToWindow();
    void setZoomFactor(double factor);
    double zoomFactor() const noexcept { return zoomFactor_; }

    // Split Before / After comparison mode
    void setSplitComparisonEnabled(bool enabled);
    bool isSplitComparisonEnabled() const noexcept { return splitComparisonEnabled_; }
    void setSplitPosition(double ratio0to1);
    double splitPosition() const noexcept { return splitPosition_; }

    // Interactive Crop mode
    void setCropOverlayEnabled(bool enabled);
    bool isCropOverlayEnabled() const noexcept { return cropOverlayEnabled_; }
    void setCropRectImageCoords(const QRect& rect);
    QRect cropRectImageCoords() const noexcept { return cropRectImg_; }
    void setCropAspectRatio(double ratioOrZeroForFree);
    double cropAspectRatio() const noexcept { return cropAspectRatio_; }
    void setShowRuleOfThirds(bool show);
    bool showRuleOfThirds() const noexcept { return showRuleOfThirds_; }

    // Eyedropper color picker mode
    void setColorPickerEnabled(bool enabled);
    bool isColorPickerEnabled() const noexcept { return colorPickerEnabled_; }

signals:
    void cropRectChanged(const QRect& newCropRectInImageCoords);
    void colorPicked(const QColor& color);
    void filesDropped(const QStringList& filePaths);
    void zoomFactorChanged(double newZoom);
    void splitPositionChanged(double ratio0to1);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    enum class CropDragHandle {
        None,
        MoveInterior,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left
    };

    QRectF computeImageSceneRect() const;
    QPointF widgetToImageCoords(const QPointF& widgetPt) const;
    QRectF imageToWidgetRect(const QRect& imgRect) const;
    CropDragHandle hitTestCropHandle(const QPointF& widgetPt) const;
    void updateCursorForPosition(const QPointF& widgetPt);
    void constrainCropRect(QRect& rect, CropDragHandle handle) const;
    void drawCheckerboard(QPainter& p, const QRectF& targetRect) const;
    void drawSplitOverlay(QPainter& p, const QRectF& targetRect) const;
    void drawCropOverlay(QPainter& p, const QRectF& targetRect) const;
    void drawEmptyPlaceholder(QPainter& p) const;

    QImage previewImage_;
    QImage originalImage_;

    bool checkerboardEnabled_ = true;
    double zoomFactor_ = 1.0;
    bool autoFitPending_ = true;
    QPointF panOffset_{0.0, 0.0};

    // Pan interaction
    bool isPanning_ = false;
    bool spacePressed_ = false;
    QPointF lastMousePos_{0.0, 0.0};

    // Split comparison state
    bool splitComparisonEnabled_ = false;
    double splitPosition_ = 0.5;
    bool isDraggingSplit_ = false;

    // Crop overlay state
    bool cropOverlayEnabled_ = false;
    QRect cropRectImg_;
    double cropAspectRatio_ = 0.0;
    bool showRuleOfThirds_ = true;
    CropDragHandle activeCropHandle_ = CropDragHandle::None;
    QPointF cropDragStartWidgetPt_;
    QRect cropRectAtDragStart_;

    // Eyedropper state
    bool colorPickerEnabled_ = false;
};

} // namespace photo26::desktop
