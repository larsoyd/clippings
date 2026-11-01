#include "frozen-desktop.hpp"

#include <QPainter>

#include <algorithm>
#include <cmath>

double FrozenOutput::scale() const
{
    return logical.width() > 0 ? double(image.width()) / logical.width() : 1.0;
}

void FrozenDesktop::add(const FrozenOutput& output)
{
    list.append(output);
}

void FrozenDesktop::clear()
{
    list.clear();
}

bool FrozenDesktop::empty() const
{
    return list.isEmpty();
}

const QList<FrozenOutput>& FrozenDesktop::outputs() const
{
    return list;
}

const FrozenOutput *FrozenDesktop::output(const QString& name) const
{
    for (const FrozenOutput& candidate : list)
    {
        if (candidate.name == name)
        {
            return &candidate;
        }
    }
    return nullptr;
}

const FrozenOutput *FrozenDesktop::output_at(const QPoint& point) const
{
    for (const FrozenOutput& candidate : list)
    {
        if (candidate.logical.contains(point))
        {
            return &candidate;
        }
    }
    return nullptr;
}

QRect FrozenDesktop::bounds() const
{
    QRect all;
    for (const FrozenOutput& output : list)
    {
        all |= output.logical;
    }
    return all;
}

double FrozenDesktop::density(const QRect& area) const
{
    double sharpest = 0;
    for (const FrozenOutput& output : list)
    {
        if (output.logical.intersects(area))
        {
            sharpest = std::max(sharpest, output.scale());
        }
    }
    return sharpest;
}

QImage FrozenDesktop::compose(const QRect& area) const
{
    const double target_scale = density(area);
    if (target_scale <= 0)
    {
        return {};
    }

    QImage result(int(std::ceil(area.width() * target_scale)), int(std::ceil(area.height() * target_scale)),
                  QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    for (const FrozenOutput& output : list)
    {
        const QRect part = output.logical & area;
        if (part.isEmpty())
        {
            continue;
        }

        const double scale = output.scale();
        const QPoint from = part.topLeft() - output.logical.topLeft();
        const QPoint to = part.topLeft() - area.topLeft();
        const QRect source(std::lround(from.x() * scale), std::lround(from.y() * scale),
                           std::lround(part.width() * scale), std::lround(part.height() * scale));
        const QRect target(std::lround(to.x() * target_scale), std::lround(to.y() * target_scale),
                           std::lround(part.width() * target_scale), std::lround(part.height() * target_scale));
        painter.drawImage(target, output.image, source);
    }
    return result;
}
