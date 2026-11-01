#include "document-canvas.hpp"

#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QQuickWindow>

#include <algorithm>
#include <cmath>

static const double SMALLEST_ZOOM = 0.05;
static const double LARGEST_ZOOM = 16;
static const double HANDLE_RADIUS = 4;
static const double HANDLE_REACH = 7;
static const double HIT_SLACK_POINTS = 4;
static const double SMALLEST_SHAPE = 2;
static const double SCROLL_STEP = 0.5;

DocumentCanvas::DocumentCanvas(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(false);
    setAntialiasing(false);
    setFillColor(Qt::transparent);
    setFlag(ItemIsFocusScope, false);
}

Document *DocumentCanvas::document() const
{
    return shown;
}

void DocumentCanvas::set_document(Document *document)
{
    if (document == shown)
    {
        return;
    }

    if (shown)
    {
        disconnect(shown, nullptr, this, nullptr);
    }
    shown = document;
    if (shown)
    {
        connect(shown, &Document::imageChanged, this, &DocumentCanvas::image_changed);
        connect(shown, &Document::historyChanged, this, &DocumentCanvas::image_changed);
        connect(shown, &Document::selectedChanged, this, [this] { update(); });
        connect(shown, &Document::toolChanged, this, &DocumentCanvas::clear_crop);
    }
    emit documentChanged();
    image_changed();
}

double DocumentCanvas::zoom() const
{
    return scale;
}

void DocumentCanvas::set_zoom(double zoom)
{
    const double next = std::clamp(zoom, SMALLEST_ZOOM, LARGEST_ZOOM);
    fit_window = false;
    if (next != scale)
    {
        scale = next;
        clamp_scroll();
        scaled = QImage();
        update();
    }
    emit zoomChanged();
}

bool DocumentCanvas::fits() const
{
    return fit_window;
}

void DocumentCanvas::set_fits(bool fits)
{
    fit_window = fits;
    update_fit();
    emit zoomChanged();
}

double DocumentCanvas::scroll_x() const
{
    return scroll_left;
}

void DocumentCanvas::set_scroll_x(double x)
{
    scroll_left = x;
    clamp_scroll();
    update();
}

double DocumentCanvas::scroll_y() const
{
    return scroll_top;
}

void DocumentCanvas::set_scroll_y(double y)
{
    scroll_top = y;
    clamp_scroll();
    update();
}

double DocumentCanvas::view_width() const
{
    return shown ? shown->image_size().width() * scale : 0;
}

double DocumentCanvas::view_height() const
{
    return shown ? shown->image_size().height() * scale : 0;
}

QRect DocumentCanvas::crop_draft() const
{
    return crop_area;
}

QPointF DocumentCanvas::origin() const
{
    // whole device pixels, a half pixel offset would blur every line of the picture
    const double dpr = window() ? window()->devicePixelRatio() : 1.0;
    return QPointF(std::round((std::max(0.0, (width() - view_width()) / 2) - scroll_left) * dpr) / dpr,
                   std::round((std::max(0.0, (height() - view_height()) / 2) - scroll_top) * dpr) / dpr);
}

QPointF DocumentCanvas::to_source(const QPointF& view) const
{
    return QPointF(shown->crop().topLeft()) + (view - origin()) / scale;
}

QPointF DocumentCanvas::to_view(const QPointF& source) const
{
    return origin() + (source - QPointF(shown->crop().topLeft())) * scale;
}

double DocumentCanvas::view_x_of(double source_x) const
{
    return to_view(QPointF(source_x, 0)).x();
}

double DocumentCanvas::view_y_of(double source_y) const
{
    return to_view(QPointF(0, source_y)).y();
}

void DocumentCanvas::actual_size()
{
    set_zoom(1 / (window() ? window()->devicePixelRatio() : 1.0));
}

void DocumentCanvas::zoom_by(double factor)
{
    set_zoom(scale * factor);
}

void DocumentCanvas::update_fit()
{
    if (!fit_window || !shown || width() <= 0 || height() <= 0)
    {
        return;
    }

    // a picture's own density decides its natural size, on any display
    const QSize size = shown->image_size();
    const double next = std::min({width() / size.width(), height() / size.height(), 1 / shown->density()});
    if (next != scale)
    {
        scale = std::max(next, SMALLEST_ZOOM);
        scaled = QImage();
        emit zoomChanged();
    }
    clamp_scroll();
    update();
}

void DocumentCanvas::clamp_scroll()
{
    const double x = std::clamp(scroll_left, 0.0, std::max(0.0, view_width() - width()));
    const double y = std::clamp(scroll_top, 0.0, std::max(0.0, view_height() - height()));
    if (x != scroll_left || y != scroll_top)
    {
        scroll_left = x;
        scroll_top = y;
    }
    emit scrollChanged();
}

void DocumentCanvas::image_changed()
{
    scaled = QImage();
    update_fit();
    emit zoomChanged();
    update();
}

void DocumentCanvas::geometryChange(const QRectF& next, const QRectF& previous)
{
    QQuickPaintedItem::geometryChange(next, previous);
    update_fit();
    clamp_scroll();
}

void DocumentCanvas::refresh_scaled()
{
    const double dpr = window() ? window()->devicePixelRatio() : 1.0;
    const double device_scale = scale * dpr;
    if (device_scale >= 1 || !scaled.isNull())
    {
        return;
    }

    const QImage whole = shown->composed().copy(shown->crop());
    scaled = whole.scaled(QSize(std::max(1, int(std::lround(whole.width() * device_scale))),
                                std::max(1, int(std::lround(whole.height() * device_scale)))),
                          Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
}

void DocumentCanvas::paint(QPainter *painter)
{
    if (!shown)
    {
        return;
    }

    const QRectF target(origin(), QSizeF(view_width(), view_height()));
    if (gesture == Gesture::Draw || gesture == Gesture::MoveSelected || gesture == Gesture::HandleSelected)
    {
        painter->setRenderHint(QPainter::SmoothPixmapTransform, scale < 1);
        painter->drawImage(target, draft_image, shown->crop());
    }
    else
    {
        refresh_scaled();
        if (!scaled.isNull())
        {
            painter->drawImage(target.topLeft(), scaled);
        }
        else
        {
            painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
            painter->drawImage(target, shown->composed(), shown->crop());
        }
    }

    draw_selection(painter);
    draw_crop(painter);
}

static void draw_handle(QPainter *painter, const QPointF& at)
{
    painter->setPen(QPen(QColor(0x26, 0x6e, 0xd8), 1));
    painter->setBrush(Qt::white);
    painter->drawEllipse(at, HANDLE_RADIUS, HANDLE_RADIUS);
}

void DocumentCanvas::draw_selection(QPainter *painter) const
{
    const int index = shown->selected();
    if (index < 0 || gesture == Gesture::Draw)
    {
        return;
    }

    const Annotation& annotation = gesture == Gesture::None ? shown->annotations()[index] : draft;
    painter->setRenderHint(QPainter::Antialiasing);
    const QList<QPointF> handles = handles_of(annotation);
    if (handles.isEmpty())
    {
        const QRectF box = bounds_of(annotation);
        painter->setPen(QPen(QColor(0x26, 0x6e, 0xd8), 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(QRectF(to_view(box.topLeft()), to_view(box.bottomRight())));
    }
    for (const QPointF& handle : handles)
    {
        draw_handle(painter, to_view(handle));
    }
}

static QList<QPointF> crop_handles(const QRectF& box)
{
    const QPointF middle = box.center();
    return {box.topLeft(), QPointF(middle.x(), box.top()), box.topRight(), QPointF(box.right(), middle.y()),
            box.bottomRight(), QPointF(middle.x(), box.bottom()), box.bottomLeft(), QPointF(box.left(), middle.y())};
}

void DocumentCanvas::draw_crop(QPainter *painter) const
{
    if (crop_area.isEmpty())
    {
        return;
    }

    const QRectF box(to_view(crop_area.topLeft()), to_view(QPointF(crop_area.bottomRight()) + QPointF(1, 1)));
    QPainterPath shade;
    shade.addRect(QRectF(origin(), QSizeF(view_width(), view_height())));
    shade.addRect(box);
    painter->fillPath(shade, QColor(0, 0, 0, 90));
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(Qt::white, 1, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(box);
    for (const QPointF& handle : crop_handles(box))
    {
        draw_handle(painter, handle);
    }
}

QRect DocumentCanvas::clamped_crop(const QRectF& area) const
{
    return area.normalized().toRect() & shown->crop();
}

int DocumentCanvas::crop_handle_at(const QPointF& view) const
{
    if (crop_area.isEmpty())
    {
        return -1;
    }

    const QRectF box(to_view(crop_area.topLeft()), to_view(QPointF(crop_area.bottomRight()) + QPointF(1, 1)));
    const QList<QPointF> handles = crop_handles(box);
    for (int index = 0; index < handles.size(); index++)
    {
        if (QLineF(handles[index], view).length() <= HANDLE_REACH)
        {
            return index;
        }
    }
    return -1;
}

int DocumentCanvas::selected_handle_at(const QPointF& view) const
{
    const int index = shown->selected();
    if (index < 0)
    {
        return -1;
    }

    const QList<QPointF> handles = handles_of(shown->annotations()[index]);
    for (int handle = 0; handle < handles.size(); handle++)
    {
        if (QLineF(to_view(handles[handle]), view).length() <= HANDLE_REACH)
        {
            return handle;
        }
    }
    return -1;
}

void DocumentCanvas::mousePressEvent(QMouseEvent *event)
{
    if (!shown)
    {
        return;
    }

    forceActiveFocus();
    const QPointF source = to_source(event->position());
    const Tool tool = Tool(shown->tool());
    gesture_start = source;

    if (tool == Tool::Crop)
    {
        active_handle = crop_handle_at(event->position());
        crop_before = crop_area;
        gesture = active_handle >= 0 ? Gesture::CropHandle
            : crop_area.contains(source.toPoint()) ? Gesture::CropMove
                                                   : Gesture::CropNew;
        return;
    }

    if (tool == Tool::Select)
    {
        active_handle = selected_handle_at(event->position());
        if (active_handle < 0)
        {
            shown->set_selected(shown->annotation_at(source, HIT_SLACK_POINTS / scale));
        }
        if (shown->selected() >= 0)
        {
            before_gesture = shown->annotations()[shown->selected()];
            draft = before_gesture;
            gesture = active_handle >= 0 ? Gesture::HandleSelected : Gesture::MoveSelected;
            draft_image = shown->composed();
        }
        return;
    }

    if (tool == Tool::Text)
    {
        emit textRequested(source.x(), source.y(), shown->annotation_at(source, 0));
        return;
    }

    draft = shown->styled(tool);
    draft.points = {source, source};
    draft_image = shown->composed_with(draft);
    gesture = Gesture::Draw;
    update();
}

void DocumentCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (!shown || gesture == Gesture::None)
    {
        return;
    }

    const QPointF source = to_source(event->position());
    switch (gesture)
    {
    case Gesture::Draw:
        if (draft.kind == Tool::Sketch || draft.kind == Tool::Highlight)
        {
            draft.points.append(source);
        }
        else
        {
            draft.points.last() = source;
        }
        draft_image = shown->composed_with(draft);
        break;
    case Gesture::MoveSelected:
    case Gesture::HandleSelected:
    {
        draft = before_gesture;
        if (gesture == Gesture::MoveSelected)
        {
            translate(draft, source - gesture_start);
        }
        else
        {
            move_handle(draft, active_handle, source);
        }
        draft_image = shown->composed();
        burn(draft_image, draft);
        break;
    }
    case Gesture::CropNew:
        crop_area = clamped_crop(QRectF(gesture_start, source));
        emit cropDraftChanged();
        break;
    case Gesture::CropMove:
    {
        QRect moved = crop_before.translated((source - gesture_start).toPoint());
        const QRect bounds = shown->crop();
        moved.moveLeft(std::clamp(moved.left(), bounds.left(), std::max(bounds.left(), bounds.right() + 1 - moved.width())));
        moved.moveTop(std::clamp(moved.top(), bounds.top(), std::max(bounds.top(), bounds.bottom() + 1 - moved.height())));
        crop_area = moved;
        emit cropDraftChanged();
        break;
    }
    case Gesture::CropHandle:
    {
        QRectF area = crop_before;
        if (active_handle == 7 || active_handle == 0 || active_handle == 6)
        {
            area.setLeft(source.x());
        }
        if (active_handle >= 2 && active_handle <= 4)
        {
            area.setRight(source.x());
        }
        if (active_handle >= 0 && active_handle <= 2)
        {
            area.setTop(source.y());
        }
        if (active_handle >= 4 && active_handle <= 6)
        {
            area.setBottom(source.y());
        }
        crop_area = clamped_crop(area);
        emit cropDraftChanged();
        break;
    }
    default:
        break;
    }
    update();
}

void DocumentCanvas::mouseReleaseEvent(QMouseEvent *)
{
    if (!shown)
    {
        return;
    }

    const Gesture ending = gesture;
    gesture = Gesture::None;
    if (ending == Gesture::Draw)
    {
        const QRectF box = box_of(draft);
        if (box.width() >= SMALLEST_SHAPE || box.height() >= SMALLEST_SHAPE)
        {
            shown->add(draft);
        }
    }
    else if (ending == Gesture::MoveSelected || ending == Gesture::HandleSelected)
    {
        shown->replace(shown->selected(), draft);
    }
    draft_image = QImage();
    active_handle = -1;
    update();
}

void DocumentCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!shown)
    {
        return;
    }

    const QPointF source = to_source(event->position());
    if (Tool(shown->tool()) == Tool::Crop && crop_area.contains(source.toPoint()))
    {
        apply_crop();
        return;
    }

    const int index = shown->annotation_at(source, HIT_SLACK_POINTS / scale);
    if (Tool(shown->tool()) == Tool::Select && index >= 0 && shown->annotations()[index].kind == Tool::Text)
    {
        emit textRequested(source.x(), source.y(), index);
    }
}

void DocumentCanvas::wheelEvent(QWheelEvent *event)
{
    const QPointF delta = event->pixelDelta().isNull() ? QPointF(event->angleDelta()) * SCROLL_STEP : QPointF(event->pixelDelta());
    if (event->modifiers() & Qt::ControlModifier)
    {
        zoom_by(std::pow(1.0015, delta.y() / SCROLL_STEP));
    }
    else
    {
        scroll_left -= delta.x();
        scroll_top -= delta.y();
        clamp_scroll();
        update();
    }
    event->accept();
}

void DocumentCanvas::apply_crop()
{
    if (shown && !crop_area.isEmpty())
    {
        const QRect area = crop_area;
        clear_crop();
        shown->crop_to(area);
        shown->set_tool(int(Tool::Select));
    }
}

void DocumentCanvas::clear_crop()
{
    crop_area = QRect();
    emit cropDraftChanged();
    update();
}

void DocumentCanvas::nudge_selected(int x, int y)
{
    if (!shown || shown->selected() < 0)
    {
        return;
    }

    Annotation annotation = shown->annotations()[shown->selected()];
    translate(annotation, QPointF(x, y));
    shown->replace(shown->selected(), annotation);
}
