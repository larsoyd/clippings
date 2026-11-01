#pragma once

#include <QColor>
#include <QImage>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>

enum class Tool
{
    Select,
    Crop,
    Arrow,
    Line,
    Rectangle,
    Ellipse,
    Text,
    Sketch,
    Highlight,
    Redact,
    Pixelate,
    Blur,
};

// every coordinate is a source image pixel, so zoom and display density never touch it
struct Annotation
{
    Tool kind = Tool::Line;
    QList<QPointF> points;
    QColor stroke = Qt::red;
    QColor fill = Qt::transparent;
    qreal width = 4;
    QString text;
    QString font_family;
    qreal font_size = 24;

    bool operator==(const Annotation& other) const = default;
};

bool is_cover(Tool kind);
QRectF box_of(const Annotation& annotation);
QRectF bounds_of(const Annotation& annotation);
bool hit(const Annotation& annotation, const QPointF& point, qreal slack);
void translate(Annotation& annotation, const QPointF& by);
QList<QPointF> handles_of(const Annotation& annotation);
void move_handle(Annotation& annotation, int handle, const QPointF& to);

// burns one annotation into the pixels, with covers replacing what they cover
void burn(QImage& image, const Annotation& annotation);
