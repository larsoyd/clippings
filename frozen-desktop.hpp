#pragma once

#include <QImage>
#include <QList>
#include <QRect>
#include <QString>

struct FrozenOutput
{
    QString name;
    // in the compositor's layout, which can start below zero
    QRect logical;
    QImage image;

    double scale() const;
};

// one instant of every display, held so selection can run over a still picture
class FrozenDesktop
{
  public:
    void add(const FrozenOutput& output);
    void clear();
    bool empty() const;
    const QList<FrozenOutput>& outputs() const;
    const FrozenOutput *output(const QString& name) const;
    const FrozenOutput *output_at(const QPoint& point) const;
    QRect bounds() const;

    // the area at the density of its sharpest display, where no display reaches stays clear
    QImage compose(const QRect& area) const;
    double density(const QRect& area) const;

  private:
    QList<FrozenOutput> list;
};
