#include "document.hpp"

#include "export.hpp"

#include <QFileInfo>

static const int MOST_STATES = 200;
static const QColor HIGHLIGHT_COLOR(0xff, 0xe4, 0x4d);

Document::Document(const QImage& image, const QString& title, double density, QObject *parent)
    : QObject(parent), source(image.convertToFormat(QImage::Format_ARGB32_Premultiplied)), name(title),
      capture_density(density)
{
    states.append({{}, source.rect()});
    recompose();
}

QString Document::title() const
{
    return name;
}

QString Document::path() const
{
    return file_path;
}

bool Document::modified() const
{
    return at != saved_at;
}

bool Document::can_undo() const
{
    return at > 0;
}

bool Document::can_redo() const
{
    return at + 1 < states.size();
}

QSize Document::image_size() const
{
    return state().crop.size();
}

double Document::density() const
{
    return capture_density;
}

QRect Document::crop() const
{
    return state().crop;
}

int Document::tool() const
{
    return int(current_tool);
}

void Document::set_tool(int tool)
{
    if (tool == int(current_tool))
    {
        return;
    }

    current_tool = Tool(tool);
    set_selected(-1);
    emit toolChanged();
}

QColor Document::stroke_color() const
{
    return stroke;
}

void Document::set_stroke_color(const QColor& color)
{
    stroke = color;
    restyle_selected();
}

QColor Document::fill_color() const
{
    return fill;
}

void Document::set_fill_color(const QColor& color)
{
    fill = color;
    restyle_selected();
}

double Document::line_width() const
{
    return width;
}

void Document::set_line_width(double value)
{
    width = value;
    restyle_selected();
}

double Document::font_size() const
{
    return text_size;
}

void Document::set_font_size(double size)
{
    text_size = size;
    restyle_selected();
}

QString Document::font_family() const
{
    return text_family;
}

void Document::set_font_family(const QString& family)
{
    text_family = family;
    restyle_selected();
}

int Document::selected() const
{
    return selected_index;
}

void Document::set_selected(int index)
{
    const int checked = index < state().annotations.size() ? index : -1;
    if (checked == selected_index)
    {
        return;
    }

    selected_index = checked;
    if (selected_index >= 0)
    {
        const Annotation& chosen = state().annotations[selected_index];
        stroke = chosen.stroke;
        fill = chosen.fill;
        width = chosen.width;
        text_size = chosen.font_size;
        emit styleChanged();
    }
    emit selectedChanged();
}

const QList<Annotation>& Document::annotations() const
{
    return state().annotations;
}

const QImage& Document::composed() const
{
    return composed_image;
}

QImage Document::composed_with(const Annotation& preview) const
{
    QImage copy = composed_image;
    burn(copy, preview);
    return copy;
}

QImage Document::flattened() const
{
    return composed_image.copy(state().crop);
}

void Document::add(const Annotation& annotation)
{
    State next = state();
    next.annotations.append(annotation);
    push(next);
}

Annotation Document::styled(Tool kind) const
{
    Annotation annotation;
    annotation.kind = kind;
    annotation.stroke = stroke;
    annotation.fill = fill;
    annotation.width = width;
    annotation.font_size = text_size;
    annotation.font_family = text_family;
    if (kind == Tool::Redact)
    {
        annotation.stroke = Qt::black;
    }
    else if (kind == Tool::Highlight)
    {
        annotation.stroke = HIGHLIGHT_COLOR;
    }
    return annotation;
}

void Document::replace(int index, const Annotation& annotation)
{
    if (index < 0 || index >= state().annotations.size() || state().annotations[index] == annotation)
    {
        return;
    }

    State next = state();
    next.annotations[index] = annotation;
    push(next);
}

int Document::annotation_at(const QPointF& point, qreal slack) const
{
    const QList<Annotation>& list = state().annotations;
    for (int index = int(list.size()) - 1; index >= 0; index--)
    {
        if (hit(list[index], point, slack))
        {
            return index;
        }
    }
    return -1;
}

void Document::undo()
{
    if (can_undo())
    {
        walk_to(at - 1);
    }
}

void Document::redo()
{
    if (can_redo())
    {
        walk_to(at + 1);
    }
}

void Document::remove_selected()
{
    if (selected_index < 0)
    {
        return;
    }

    State next = state();
    next.annotations.removeAt(selected_index);
    set_selected(-1);
    push(next);
}

void Document::crop_to(const QRect& area)
{
    const QRect inside = area & source.rect();
    if (inside.isEmpty() || inside == state().crop)
    {
        return;
    }

    State next = state();
    next.crop = inside;
    push(next);
}

void Document::add_text(double x, double y, const QString& text)
{
    if (text.trimmed().isEmpty())
    {
        return;
    }

    Annotation annotation = styled(Tool::Text);
    annotation.points = {QPointF(x, y)};
    annotation.text = text;
    add(annotation);
}

void Document::set_text(int index, const QString& text)
{
    if (index < 0 || index >= state().annotations.size())
    {
        return;
    }

    if (text.trimmed().isEmpty())
    {
        set_selected(index);
        remove_selected();
        return;
    }

    Annotation annotation = state().annotations[index];
    annotation.text = text;
    replace(index, annotation);
}

QString Document::text_of(int index) const
{
    return index >= 0 && index < state().annotations.size() ? state().annotations[index].text : QString();
}

QString Document::save_to(const QString& path)
{
    const QString problem = write_image(flattened(), path);
    if (!problem.isEmpty())
    {
        return problem;
    }

    file_path = path;
    name = QFileInfo(path).fileName();
    saved_at = at;
    emit titleChanged();
    emit historyChanged();
    return {};
}

void Document::discard()
{
    saved_at = at;
    emit historyChanged();
}

int Document::annotation_count() const
{
    return int(state().annotations.size());
}

void Document::opened_from(const QString& path)
{
    file_path = path;
    saved_at = at;
    emit titleChanged();
    emit historyChanged();
}

const Document::State& Document::state() const
{
    return states[at];
}

void Document::push(const State& next)
{
    if (saved_at > at)
    {
        saved_at = -1;
    }

    states.resize(at + 1);
    states.append(next);
    at++;
    if (states.size() > MOST_STATES)
    {
        states.removeFirst();
        at--;
        saved_at = saved_at > 0 ? saved_at - 1 : -1;
    }

    recompose();
    emit historyChanged();
}

void Document::walk_to(int index)
{
    at = index;
    if (selected_index >= state().annotations.size())
    {
        set_selected(-1);
    }
    recompose();
    emit historyChanged();
}

void Document::recompose()
{
    composed_image = source;
    for (const Annotation& annotation : state().annotations)
    {
        burn(composed_image, annotation);
    }
    emit imageChanged();
}

void Document::restyle_selected()
{
    if (selected_index >= 0)
    {
        Annotation annotation = state().annotations[selected_index];
        annotation.stroke = stroke;
        annotation.fill = fill;
        annotation.width = width;
        annotation.font_size = text_size;
        if (!text_family.isEmpty())
        {
            annotation.font_family = text_family;
        }
        replace(selected_index, annotation);
    }
    emit styleChanged();
}
