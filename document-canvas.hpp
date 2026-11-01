#pragma once

#include <QImage>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QRect>

#include "annotation.hpp"
#include "document.hpp"

// draws the document's own composed pixels, so the view can't disagree with an export
class DocumentCanvas : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Document *document READ document WRITE set_document NOTIFY documentChanged)
    Q_PROPERTY(double zoom READ zoom WRITE set_zoom NOTIFY zoomChanged)
    Q_PROPERTY(bool fits READ fits WRITE set_fits NOTIFY zoomChanged)
    Q_PROPERTY(double scrollX READ scroll_x WRITE set_scroll_x NOTIFY scrollChanged)
    Q_PROPERTY(double scrollY READ scroll_y WRITE set_scroll_y NOTIFY scrollChanged)
    Q_PROPERTY(double viewWidth READ view_width NOTIFY zoomChanged)
    Q_PROPERTY(double viewHeight READ view_height NOTIFY zoomChanged)
    Q_PROPERTY(QRect cropDraft READ crop_draft NOTIFY cropDraftChanged)

  public:
    explicit DocumentCanvas(QQuickItem *parent = nullptr);

    Document *document() const;
    void set_document(Document *document);
    double zoom() const;
    void set_zoom(double zoom);
    bool fits() const;
    void set_fits(bool fits);
    double scroll_x() const;
    void set_scroll_x(double x);
    double scroll_y() const;
    void set_scroll_y(double y);
    double view_width() const;
    double view_height() const;
    QRect crop_draft() const;

    QPointF to_source(const QPointF& view) const;
    QPointF to_view(const QPointF& source) const;

    void paint(QPainter *painter) override;

    Q_INVOKABLE void actual_size();
    Q_INVOKABLE void zoom_by(double factor);
    Q_INVOKABLE void apply_crop();
    Q_INVOKABLE void clear_crop();
    Q_INVOKABLE void nudge_selected(int x, int y);
    Q_INVOKABLE double view_x_of(double source_x) const;
    Q_INVOKABLE double view_y_of(double source_y) const;

  signals:
    void documentChanged();
    void zoomChanged();
    void scrollChanged();
    void cropDraftChanged();
    // the text tool clicked here, index is -1 for new text
    void textRequested(double x, double y, int index);

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void geometryChange(const QRectF& next, const QRectF& previous) override;

  private:
    enum class Gesture
    {
        None,
        Draw,
        MoveSelected,
        HandleSelected,
        CropNew,
        CropMove,
        CropHandle,
    };

    QPointF origin() const;
    QRect clamped_crop(const QRectF& area) const;
    void update_fit();
    void clamp_scroll();
    void image_changed();
    int crop_handle_at(const QPointF& view) const;
    int selected_handle_at(const QPointF& view) const;
    void draw_selection(QPainter *painter) const;
    void draw_crop(QPainter *painter) const;
    void refresh_scaled();

    QPointer<Document> shown;
    double scale = 1;
    bool fit_window = true;
    double scroll_left = 0;
    double scroll_top = 0;

    Gesture gesture = Gesture::None;
    Annotation draft;
    Annotation before_gesture;
    QImage draft_image;
    QPointF gesture_start;
    QRect crop_before;
    QRect crop_area;
    int active_handle = -1;
    QImage scaled;
};
