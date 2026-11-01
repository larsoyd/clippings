#include "annotation.hpp"

#include <QFont>
#include <QFontMetricsF>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

static const int PIXELATE_MIN_BLOCK = 8;
static const int BLUR_SHRINK = 12;

bool is_cover(Tool kind)
{
    return kind == Tool::Redact || kind == Tool::Pixelate || kind == Tool::Blur;
}

QRectF box_of(const Annotation& annotation)
{
    if (annotation.points.isEmpty())
    {
        return {};
    }

    qreal left = annotation.points.first().x();
    qreal right = left;
    qreal top = annotation.points.first().y();
    qreal bottom = top;
    for (const QPointF& point : annotation.points)
    {
        left = std::min(left, point.x());
        right = std::max(right, point.x());
        top = std::min(top, point.y());
        bottom = std::max(bottom, point.y());
    }
    return QRectF(QPointF(left, top), QPointF(right, bottom));
}

static QFont font_of(const Annotation& annotation)
{
    QFont font(annotation.font_family);
    font.setPixelSize(std::max(1, int(std::lround(annotation.font_size))));
    return font;
}

static QRectF text_box(const Annotation& annotation)
{
    const QPointF at = annotation.points.first();
    const QFontMetricsF metrics(font_of(annotation));
    const QRectF room(at, QSizeF(100000, 100000));
    return metrics.boundingRect(room, Qt::AlignLeft | Qt::AlignTop, annotation.text);
}

static qreal arrow_head(const Annotation& annotation)
{
    return std::max<qreal>(12, annotation.width * 3.5);
}

QRectF bounds_of(const Annotation& annotation)
{
    if (annotation.kind == Tool::Text)
    {
        return text_box(annotation);
    }

    const qreal reach = is_cover(annotation.kind) ? 0
        : annotation.kind == Tool::Arrow ? std::max(annotation.width, arrow_head(annotation))
                                         : annotation.width;
    return box_of(annotation).adjusted(-reach, -reach, reach, reach);
}

static qreal distance_to_segment(const QPointF& point, const QPointF& from, const QPointF& to)
{
    const QPointF along = to - from;
    const qreal length = QPointF::dotProduct(along, along);
    const qreal share = length > 0 ? std::clamp(QPointF::dotProduct(point - from, along) / length, 0.0, 1.0) : 0.0;
    return QLineF(point, from + along * share).length();
}

static bool near_path(const Annotation& annotation, const QPointF& point, qreal reach)
{
    for (int index = 1; index < annotation.points.size(); index++)
    {
        if (distance_to_segment(point, annotation.points[index - 1], annotation.points[index]) <= reach)
        {
            return true;
        }
    }
    return false;
}

bool hit(const Annotation& annotation, const QPointF& point, qreal slack)
{
    const qreal reach = annotation.width / 2 + slack;
    switch (annotation.kind)
    {
    case Tool::Line:
    case Tool::Arrow:
    case Tool::Sketch:
    case Tool::Highlight:
        return near_path(annotation, point, reach);
    case Tool::Rectangle:
    case Tool::Ellipse:
    {
        const QRectF box = box_of(annotation);
        const bool inside_outer = box.adjusted(-reach, -reach, reach, reach).contains(point);
        const bool inside_inner = box.adjusted(reach, reach, -reach, -reach).contains(point);
        return inside_outer && (annotation.fill.alpha() > 0 || !inside_inner);
    }
    default:
        return bounds_of(annotation).adjusted(-slack, -slack, slack, slack).contains(point);
    }
}

void translate(Annotation& annotation, const QPointF& by)
{
    for (QPointF& point : annotation.points)
    {
        point += by;
    }
}

QList<QPointF> handles_of(const Annotation& annotation)
{
    switch (annotation.kind)
    {
    case Tool::Line:
    case Tool::Arrow:
        return annotation.points;
    case Tool::Rectangle:
    case Tool::Ellipse:
    case Tool::Redact:
    case Tool::Pixelate:
    case Tool::Blur:
    {
        const QPointF first = annotation.points.first();
        const QPointF second = annotation.points.last();
        return {first, second, QPointF(first.x(), second.y()), QPointF(second.x(), first.y())};
    }
    default:
        return {};
    }
}

void move_handle(Annotation& annotation, int handle, const QPointF& to)
{
    if (annotation.points.size() != 2)
    {
        return;
    }

    switch (handle)
    {
    case 0:
        annotation.points[0] = to;
        break;
    case 1:
        annotation.points[1] = to;
        break;
    case 2:
        annotation.points[0].setX(to.x());
        annotation.points[1].setY(to.y());
        break;
    case 3:
        annotation.points[1].setX(to.x());
        annotation.points[0].setY(to.y());
        break;
    default:
        break;
    }
}

static QPainterPath stroke_path(const Annotation& annotation)
{
    QPainterPath path(annotation.points.first());
    for (int index = 1; index < annotation.points.size(); index++)
    {
        path.lineTo(annotation.points[index]);
    }
    return path;
}

static void draw_arrow(QPainter& painter, const Annotation& annotation)
{
    const QPointF tail = annotation.points.first();
    const QPointF tip = annotation.points.last();
    const QLineF shaft(tail, tip);
    if (shaft.length() <= 0)
    {
        return;
    }

    const qreal head = std::min(arrow_head(annotation), shaft.length());
    const QPointF unit = (tip - tail) / shaft.length();
    const QPointF across(-unit.y(), unit.x());
    const QPointF base = tip - unit * head;

    QPen pen(annotation.stroke, annotation.width, Qt::SolidLine, Qt::FlatCap);
    painter.setPen(pen);
    painter.drawLine(tail, base);
    QPolygonF wings{tip, base + across * head * 0.5, base - across * head * 0.5};
    painter.setPen(Qt::NoPen);
    painter.setBrush(annotation.stroke);
    painter.drawPolygon(wings);
}

static void replace_area(QImage& image, const QRect& area, const QImage& with)
{
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(area, with);
}

static void pixelate(QImage& image, const QRect& area, int block)
{
    const QImage part = image.copy(area);
    const QSize coarse(std::max(1, area.width() / block), std::max(1, area.height() / block));
    const QImage small = part.scaled(coarse, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    replace_area(image, area, small.scaled(area.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
}

static void blur(QImage& image, const QRect& area)
{
    const QImage part = image.copy(area);
    const QSize coarse(std::max(1, area.width() / BLUR_SHRINK), std::max(1, area.height() / BLUR_SHRINK));
    const QImage small = part.scaled(coarse, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    replace_area(image, area, small.scaled(area.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

void burn(QImage& image, const Annotation& annotation)
{
    if (annotation.points.isEmpty())
    {
        return;
    }

    if (is_cover(annotation.kind))
    {
        const QRect area = box_of(annotation).toAlignedRect() & image.rect();
        if (area.isEmpty())
        {
            return;
        }

        if (annotation.kind == Tool::Redact)
        {
            // source mode so a transparent pixel loses its colour as well
            QPainter painter(&image);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(area, QColor(annotation.stroke.red(), annotation.stroke.green(), annotation.stroke.blue()));
        }
        else if (annotation.kind == Tool::Pixelate)
        {
            pixelate(image, area, std::max(PIXELATE_MIN_BLOCK, int(annotation.width * 2)));
        }
        else
        {
            blur(image, area);
        }
        return;
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(annotation.stroke, annotation.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(annotation.fill.alpha() > 0 ? QBrush(annotation.fill) : QBrush(Qt::NoBrush));

    switch (annotation.kind)
    {
    case Tool::Line:
    case Tool::Sketch:
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(stroke_path(annotation));
        break;
    case Tool::Highlight:
        painter.setBrush(Qt::NoBrush);
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        painter.setPen(QPen(annotation.stroke, annotation.width * 4, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        painter.drawPath(stroke_path(annotation));
        break;
    case Tool::Arrow:
        draw_arrow(painter, annotation);
        break;
    case Tool::Rectangle:
        painter.drawRect(box_of(annotation));
        break;
    case Tool::Ellipse:
        painter.drawEllipse(box_of(annotation));
        break;
    case Tool::Text:
        painter.setFont(font_of(annotation));
        painter.drawText(text_box(annotation), Qt::AlignLeft | Qt::AlignTop, annotation.text);
        break;
    default:
        break;
    }
}
