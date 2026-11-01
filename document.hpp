#pragma once

#include <QColor>
#include <QImage>
#include <QList>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QString>

#include "annotation.hpp"

// one image with its edits: the source stays as captured, and undo walks whole edit states
class Document : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString path READ path NOTIFY titleChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY historyChanged)
    Q_PROPERTY(bool canUndo READ can_undo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ can_redo NOTIFY historyChanged)
    Q_PROPERTY(QSize imageSize READ image_size NOTIFY imageChanged)
    Q_PROPERTY(double density READ density CONSTANT)
    Q_PROPERTY(int tool READ tool WRITE set_tool NOTIFY toolChanged)
    Q_PROPERTY(QColor strokeColor READ stroke_color WRITE set_stroke_color NOTIFY styleChanged)
    Q_PROPERTY(QColor fillColor READ fill_color WRITE set_fill_color NOTIFY styleChanged)
    Q_PROPERTY(double lineWidth READ line_width WRITE set_line_width NOTIFY styleChanged)
    Q_PROPERTY(double fontSize READ font_size WRITE set_font_size NOTIFY styleChanged)
    Q_PROPERTY(QString fontFamily READ font_family WRITE set_font_family NOTIFY styleChanged)
    Q_PROPERTY(int selected READ selected WRITE set_selected NOTIFY selectedChanged)
    Q_PROPERTY(int annotationCount READ annotation_count NOTIFY historyChanged)

  public:
    Document(const QImage& source, const QString& title, double density, QObject *parent = nullptr);

    QString title() const;
    QString path() const;
    bool modified() const;
    bool can_undo() const;
    bool can_redo() const;
    QSize image_size() const;
    double density() const;
    QRect crop() const;

    int tool() const;
    void set_tool(int tool);
    QColor stroke_color() const;
    void set_stroke_color(const QColor& color);
    QColor fill_color() const;
    void set_fill_color(const QColor& color);
    double line_width() const;
    void set_line_width(double width);
    double font_size() const;
    void set_font_size(double size);
    QString font_family() const;
    void set_font_family(const QString& family);
    int selected() const;
    void set_selected(int index);

    const QList<Annotation>& annotations() const;
    int annotation_count() const;
    // the whole source with the edits burned in, before cropping
    const QImage& composed() const;
    QImage composed_with(const Annotation& preview) const;
    QImage flattened() const;

    void add(const Annotation& annotation);
    // an annotation with the current style, ready to be drawn
    Annotation styled(Tool kind) const;
    void replace(int index, const Annotation& annotation);
    int annotation_at(const QPointF& point, qreal slack) const;

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void remove_selected();
    Q_INVOKABLE void crop_to(const QRect& area);
    Q_INVOKABLE void add_text(double x, double y, const QString& text);
    Q_INVOKABLE void set_text(int index, const QString& text);
    Q_INVOKABLE QString text_of(int index) const;
    // empty on success, otherwise the reason it failed
    Q_INVOKABLE QString save_to(const QString& path);
    // the changes are given up, so closing asks nothing more
    Q_INVOKABLE void discard();
    // a file opened as it is counts as saved
    void opened_from(const QString& path);

  signals:
    void titleChanged();
    void historyChanged();
    void imageChanged();
    void toolChanged();
    void styleChanged();
    void selectedChanged();

  private:
    struct State
    {
        QList<Annotation> annotations;
        QRect crop;
    };

    const State& state() const;
    void push(const State& next);
    void walk_to(int index);
    void recompose();
    void restyle_selected();

    QImage source;
    QImage composed_image;
    QString name;
    QString file_path;
    double capture_density;

    QList<State> states;
    int at = 0;
    // a save point lost to a later branch of history stays lost
    int saved_at = -1;

    Tool current_tool = Tool::Select;
    QColor stroke = QColor(0xf7, 0x4a, 0x3e);
    QColor fill = Qt::transparent;
    qreal width = 4;
    qreal text_size = 24;
    QString text_family;
    int selected_index = -1;
};
