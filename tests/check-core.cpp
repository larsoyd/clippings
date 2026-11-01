#include "check-core.hpp"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "../annotation.hpp"
#include "../document.hpp"
#include "../export.hpp"
#include "../frozen-desktop.hpp"

static QImage solid(int width, int height, QColor color)
{
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(color);
    return image;
}

static QImage noisy(int width, int height)
{
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            image.setPixelColor(x, y, QColor((x * 37) % 256, (y * 91) % 256, ((x + y) * 13) % 256));
        }
    }
    return image;
}

static Annotation shape(Tool kind, QPointF from, QPointF to)
{
    Annotation annotation;
    annotation.kind = kind;
    annotation.points = {from, to};
    annotation.stroke = Qt::black;
    annotation.width = 4;
    return annotation;
}

void CheckCore::compose_copies_exact_pixels()
{
    FrozenDesktop desktop;
    const QImage picture = noisy(100, 80);
    desktop.add({QStringLiteral("a"), QRect(0, 0, 100, 80), picture});
    const QImage part = desktop.compose(QRect(10, 20, 30, 40));
    QCOMPARE(part.size(), QSize(30, 40));
    QCOMPARE(part.pixelColor(0, 0), picture.pixelColor(10, 20));
    QCOMPARE(part.pixelColor(29, 39), picture.pixelColor(39, 59));
}

void CheckCore::compose_crosses_mixed_scales_and_negative_origin()
{
    FrozenDesktop desktop;
    desktop.add({QStringLiteral("left"), QRect(-100, 0, 100, 100), solid(100, 100, Qt::red)});
    desktop.add({QStringLiteral("right"), QRect(0, 0, 100, 100), solid(200, 200, Qt::blue)});
    QCOMPARE(desktop.bounds(), QRect(-100, 0, 200, 100));

    const QRect area(-50, 10, 100, 50);
    QCOMPARE(desktop.density(area), 2.0);
    const QImage both = desktop.compose(area);
    QCOMPARE(both.size(), QSize(200, 100));
    QCOMPARE(both.pixelColor(10, 50), QColor(Qt::red));
    QCOMPARE(both.pixelColor(190, 50), QColor(Qt::blue));
    QCOMPARE(both.pixelColor(98, 50), QColor(Qt::red));
    QCOMPARE(both.pixelColor(101, 50), QColor(Qt::blue));
}

void CheckCore::density_ignores_the_order_of_displays()
{
    FrozenDesktop desktop;
    desktop.add({QStringLiteral("sharp"), QRect(0, 0, 100, 100), solid(200, 200, Qt::blue)});
    desktop.add({QStringLiteral("coarse"), QRect(100, 0, 100, 100), solid(100, 100, Qt::red)});
    QCOMPARE(desktop.density(QRect(50, 0, 100, 50)), 2.0);
}

void CheckCore::compose_leaves_gaps_clear()
{
    FrozenDesktop desktop;
    desktop.add({QStringLiteral("a"), QRect(0, 0, 10, 10), solid(10, 10, Qt::green)});
    desktop.add({QStringLiteral("b"), QRect(20, 0, 10, 10), solid(10, 10, Qt::green)});
    const QImage wide = desktop.compose(desktop.bounds());
    QCOMPARE(wide.pixelColor(15, 5).alpha(), 0);
    QCOMPARE(wide.pixelColor(5, 5).alpha(), 255);
}

void CheckCore::compose_outside_every_display_is_empty()
{
    FrozenDesktop desktop;
    desktop.add({QStringLiteral("a"), QRect(0, 0, 10, 10), solid(10, 10, Qt::green)});
    QVERIFY(desktop.compose(QRect(50, 50, 5, 5)).isNull());
}

void CheckCore::output_lookup_honours_negative_layout()
{
    FrozenDesktop desktop;
    desktop.add({QStringLiteral("left"), QRect(-100, 0, 100, 100), solid(100, 100, Qt::red)});
    desktop.add({QStringLiteral("right"), QRect(0, 0, 100, 100), solid(200, 200, Qt::blue)});
    QCOMPARE(desktop.output_at(QPoint(-1, 5))->name, QStringLiteral("left"));
    QCOMPARE(desktop.output_at(QPoint(0, 5))->name, QStringLiteral("right"));
    QVERIFY(desktop.output_at(QPoint(0, 500)) == nullptr);
    QCOMPARE(desktop.output(QStringLiteral("right"))->scale(), 2.0);
}

void CheckCore::annotations_stay_in_source_pixels()
{
    Document document(solid(200, 100, Qt::white), QStringLiteral("t"), 2);
    document.add(shape(Tool::Rectangle, QPointF(20, 20), QPointF(60, 60)));
    const QImage result = document.flattened();
    QCOMPARE(result.size(), QSize(200, 100));
    QCOMPARE(result.pixelColor(20, 40), QColor(Qt::black));
    QCOMPARE(result.pixelColor(40, 40), QColor(Qt::white));
}

void CheckCore::arrow_points_at_its_tip()
{
    Document document(solid(200, 100, Qt::white), QStringLiteral("t"), 1);
    document.add(shape(Tool::Arrow, QPointF(10, 50), QPointF(190, 50)));
    const QImage result = document.flattened();
    QCOMPARE(result.pixelColor(100, 50), QColor(Qt::black));
    QCOMPARE(result.pixelColor(176, 44), QColor(Qt::black));
    QCOMPARE(result.pixelColor(100, 44), QColor(Qt::white));
    QCOMPARE(result.pixelColor(195, 50), QColor(Qt::white));
}

void CheckCore::redaction_removes_the_covered_pixels()
{
    Document document(noisy(64, 64), QStringLiteral("t"), 1);
    Annotation cover = document.styled(Tool::Redact);
    cover.points = {QPointF(8.4, 8.4), QPointF(40.2, 40.2)};
    document.add(cover);

    const QImage result = document.flattened();
    for (int y = 8; y <= 40; y++)
    {
        for (int x = 8; x <= 40; x++)
        {
            QCOMPARE(result.pixelColor(x, y), QColor(Qt::black));
        }
    }
    QCOMPARE(result.pixelColor(7, 7), noisy(64, 64).pixelColor(7, 7));
}

void CheckCore::redaction_clears_colour_under_transparency()
{
    QImage clear(32, 32, QImage::Format_ARGB32);
    clear.fill(QColor(200, 10, 10, 40));
    Document document(clear, QStringLiteral("t"), 1);
    Annotation cover = document.styled(Tool::Redact);
    cover.points = {QPointF(0, 0), QPointF(32, 32)};
    document.add(cover);
    const QColor result = document.flattened().pixelColor(5, 5);
    QCOMPARE(result, QColor(Qt::black));
    QCOMPARE(result.alpha(), 255);
}

void CheckCore::pixelate_and_blur_only_touch_their_area()
{
    const QImage picture = noisy(100, 100);
    for (Tool kind : {Tool::Pixelate, Tool::Blur})
    {
        Document document(picture, QStringLiteral("t"), 1);
        document.add(shape(kind, QPointF(20, 20), QPointF(60, 60)));
        const QImage result = document.flattened();
        QCOMPARE(result.pixelColor(19, 19), picture.pixelColor(19, 19));
        QCOMPARE(result.pixelColor(61, 61), picture.pixelColor(61, 61));

        int same = 0;
        for (int y = 21; y < 59; y++)
        {
            for (int x = 21; x < 59; x++)
            {
                same += result.pixelColor(x, y) == picture.pixelColor(x, y);
            }
        }
        QVERIFY2(same < 100, qPrintable(QStringLiteral("unchanged=%1").arg(same)));
    }
}

void CheckCore::export_equals_what_the_view_shows()
{
    Document document(noisy(120, 90), QStringLiteral("t"), 1);
    document.add(shape(Tool::Ellipse, QPointF(10, 10), QPointF(80, 60)));
    document.add(shape(Tool::Blur, QPointF(30, 30), QPointF(70, 50)));
    document.crop_to(QRect(5, 5, 100, 70));
    QCOMPARE(document.flattened(), document.composed().copy(document.crop()));
}

void CheckCore::covers_read_earlier_annotations()
{
    Document document(solid(100, 100, Qt::white), QStringLiteral("t"), 1);
    Annotation bar = shape(Tool::Rectangle, QPointF(30, 30), QPointF(70, 70));
    bar.fill = Qt::black;
    document.add(bar);
    document.add(shape(Tool::Pixelate, QPointF(20, 20), QPointF(80, 80)));
    QVERIFY(document.flattened().pixelColor(50, 50).lightness() < 200);
}

void CheckCore::crop_keeps_annotation_positions()
{
    Document document(solid(100, 100, Qt::white), QStringLiteral("t"), 1);
    document.add(shape(Tool::Line, QPointF(0, 50), QPointF(100, 50)));
    document.crop_to(QRect(0, 40, 100, 20));
    QCOMPARE(document.image_size(), QSize(100, 20));
    QCOMPARE(document.flattened().pixelColor(50, 10), QColor(Qt::black));
    document.undo();
    QCOMPARE(document.image_size(), QSize(100, 100));
    QCOMPARE(document.flattened().pixelColor(50, 50), QColor(Qt::black));
    document.crop_to(QRect(90, 90, 50, 50));
    QCOMPARE(document.image_size(), QSize(10, 10));
    document.crop_to(QRect(500, 500, 10, 10));
    QCOMPARE(document.image_size(), QSize(10, 10));
}

void CheckCore::text_grows_the_picture()
{
    Document document(solid(200, 80, Qt::white), QStringLiteral("t"), 1);
    document.set_font_size(30);
    document.set_stroke_color(Qt::black);
    document.add_text(10, 10, QStringLiteral("Hg"));
    QCOMPARE(document.annotations().size(), 1);
    QVERIFY(document.flattened() != solid(200, 80, Qt::white));
    document.add_text(10, 10, QStringLiteral("   "));
    QCOMPARE(document.annotations().size(), 1);
    document.set_text(0, QStringLiteral("changed"));
    QCOMPARE(document.text_of(0), QStringLiteral("changed"));
    document.set_text(0, QString());
    QCOMPARE(document.annotations().size(), 0);
}

void CheckCore::undo_and_redo_walk_states()
{
    Document document(solid(50, 50, Qt::white), QStringLiteral("t"), 1);
    QVERIFY(!document.can_undo());
    document.add(shape(Tool::Line, QPointF(0, 25), QPointF(50, 25)));
    QCOMPARE(document.flattened().pixelColor(25, 25), QColor(Qt::black));
    document.undo();
    QCOMPARE(document.flattened().pixelColor(25, 25), QColor(Qt::white));
    QVERIFY(document.can_redo());
    document.redo();
    QCOMPARE(document.flattened().pixelColor(25, 25), QColor(Qt::black));
    document.redo();
    QVERIFY(!document.can_redo());
}

void CheckCore::new_edit_drops_redo()
{
    Document document(solid(50, 50, Qt::white), QStringLiteral("t"), 1);
    document.add(shape(Tool::Line, QPointF(0, 10), QPointF(50, 10)));
    document.undo();
    document.add(shape(Tool::Line, QPointF(0, 20), QPointF(50, 20)));
    QVERIFY(!document.can_redo());
    QCOMPARE(document.annotations().size(), 1);
}

void CheckCore::save_clears_modified_and_undo_sets_it()
{
    QTemporaryDir folder;
    const QString path = folder.filePath(QStringLiteral("x.png"));
    Document document(solid(50, 50, Qt::white), QStringLiteral("t"), 1);
    QVERIFY(document.modified());
    QVERIFY(document.save_to(path).isEmpty());
    QVERIFY(!document.modified());
    QCOMPARE(document.title(), QStringLiteral("x.png"));
    document.add(shape(Tool::Line, QPointF(0, 10), QPointF(50, 10)));
    QVERIFY(document.modified());
    document.undo();
    QVERIFY(!document.modified());
    document.undo();
    QVERIFY(!document.modified());
}

void CheckCore::lost_save_point_stays_modified()
{
    QTemporaryDir folder;
    Document document(solid(50, 50, Qt::white), QStringLiteral("t"), 1);
    document.add(shape(Tool::Line, QPointF(0, 10), QPointF(50, 10)));
    QVERIFY(document.save_to(folder.filePath(QStringLiteral("x.png"))).isEmpty());
    document.undo();
    document.add(shape(Tool::Line, QPointF(0, 20), QPointF(50, 20)));
    QVERIFY(document.modified());
    document.undo();
    QVERIFY(document.modified());
}

void CheckCore::history_stops_growing()
{
    Document document(solid(8, 8, Qt::white), QStringLiteral("t"), 1);
    for (int index = 0; index < 260; index++)
    {
        document.add(shape(Tool::Line, QPointF(0, 1), QPointF(8, 1)));
    }
    int steps = 0;
    while (document.can_undo() && steps < 400)
    {
        document.undo();
        steps++;
    }
    QCOMPARE(steps, 199);
    QVERIFY(!document.annotations().isEmpty());
}

void CheckCore::restyling_selection_is_one_step()
{
    Document document(solid(50, 50, Qt::white), QStringLiteral("t"), 1);
    document.add(shape(Tool::Rectangle, QPointF(5, 5), QPointF(40, 40)));
    document.set_selected(0);
    QCOMPARE(document.stroke_color(), QColor(Qt::black));
    document.set_stroke_color(Qt::green);
    QCOMPARE(document.annotations()[0].stroke, QColor(Qt::green));
    document.undo();
    QCOMPARE(document.annotations()[0].stroke, QColor(Qt::black));
    document.set_selected(-1);
    document.set_stroke_color(Qt::blue);
    QCOMPARE(document.annotations()[0].stroke, QColor(Qt::black));
}

void CheckCore::replacing_with_the_same_is_no_edit()
{
    Document document(solid(50, 50, Qt::white), QStringLiteral("t"), 1);
    document.add(shape(Tool::Line, QPointF(0, 10), QPointF(50, 10)));
    document.replace(0, document.annotations()[0]);
    document.undo();
    QVERIFY(!document.can_undo());
    QCOMPARE(document.annotations().size(), 0);
}

void CheckCore::selected_follows_removal()
{
    Document document(solid(50, 50, Qt::white), QStringLiteral("t"), 1);
    document.add(shape(Tool::Line, QPointF(0, 10), QPointF(50, 10)));
    document.set_selected(0);
    QCOMPARE(document.selected(), 0);
    document.remove_selected();
    QCOMPARE(document.selected(), -1);
    QCOMPARE(document.annotations().size(), 0);
    document.undo();
    document.set_selected(0);
    document.undo();
    QCOMPARE(document.selected(), -1);
    document.set_selected(7);
    QCOMPARE(document.selected(), -1);
}

void CheckCore::hit_finds_topmost()
{
    Document document(solid(100, 100, Qt::white), QStringLiteral("t"), 1);
    Annotation lower = shape(Tool::Rectangle, QPointF(10, 10), QPointF(90, 90));
    lower.fill = Qt::red;
    document.add(lower);
    document.add(shape(Tool::Line, QPointF(0, 50), QPointF(100, 50)));
    QCOMPARE(document.annotation_at(QPointF(50, 50), 2), 1);
    QCOMPARE(document.annotation_at(QPointF(50, 30), 2), 0);
    QCOMPARE(document.annotation_at(QPointF(3, 3), 2), -1);
}

void CheckCore::hollow_shapes_ignore_their_middle()
{
    const Annotation hollow = shape(Tool::Rectangle, QPointF(10, 10), QPointF(90, 90));
    QVERIFY(hit(hollow, QPointF(10, 50), 2));
    QVERIFY(!hit(hollow, QPointF(50, 50), 2));
}

void CheckCore::write_refuses_unknown_format()
{
    QTemporaryDir folder;
    const QString problem = write_image(solid(4, 4, Qt::red), folder.filePath(QStringLiteral("a.nope")));
    QVERIFY2(problem.contains(QStringLiteral("nope")), qPrintable(problem));
    QVERIFY(!QFile::exists(folder.filePath(QStringLiteral("a.nope"))));
}

void CheckCore::failed_write_keeps_old_file()
{
    QTemporaryDir folder;
    const QString path = folder.filePath(QStringLiteral("a.png"));
    QVERIFY(write_image(solid(4, 4, Qt::red), path).isEmpty());
    const QByteArray before = [&] { QFile file(path); return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray(); }();
    QVERIFY(QFile::setPermissions(folder.path(), QFile::ReadOwner | QFile::ExeOwner));
    const QString problem = write_image(solid(4, 4, Qt::blue), path);
    QVERIFY(QFile::setPermissions(folder.path(), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    QVERIFY(!problem.isEmpty());
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), before);
}

void CheckCore::jpeg_flattens_on_white()
{
    QTemporaryDir folder;
    const QString path = folder.filePath(QStringLiteral("a.jpg"));
    QImage clear(16, 16, QImage::Format_ARGB32_Premultiplied);
    clear.fill(Qt::transparent);
    QVERIFY(write_image(clear, path).isEmpty());
    QVERIFY(QImage(path).pixelColor(8, 8).lightness() > 250);
}

void CheckCore::png_round_trips_alpha()
{
    QTemporaryDir folder;
    const QString path = folder.filePath(QStringLiteral("a.png"));
    QImage clear(16, 16, QImage::Format_ARGB32_Premultiplied);
    clear.fill(QColor(255, 0, 0, 128));
    QVERIFY(write_image(clear, path).isEmpty());
    QCOMPARE(QImage(path).pixelColor(8, 8).alpha(), 128);
}

void CheckCore::copy_reports_missing_tool()
{
    QVERIFY(!copy_image(solid(4, 4, Qt::red), QStringLiteral("/nonexistent/wl-copy")).isEmpty());
}

void CheckCore::copy_reports_failing_tool()
{
    QTemporaryDir folder;
    const QString tool = folder.filePath(QStringLiteral("fail"));
    QFile file(tool);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("#!/bin/sh\nexit 3\n");
    file.close();
    file.setPermissions(file.permissions() | QFile::ExeOwner);
    QVERIFY(copy_image(solid(4, 4, Qt::red), tool).contains(QStringLiteral("3")));
}

void CheckCore::copy_sends_png()
{
    QTemporaryDir folder;
    const QString tool = folder.filePath(QStringLiteral("store"));
    const QString received = folder.filePath(QStringLiteral("in"));
    QFile file(tool);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QStringLiteral("#!/bin/sh\ncat > '%1'\n").arg(received).toUtf8());
    file.close();
    file.setPermissions(file.permissions() | QFile::ExeOwner);
    QVERIFY(copy_image(solid(4, 4, Qt::red), tool).isEmpty());
    QCOMPARE(QImage(received, "PNG").pixelColor(1, 1), QColor(Qt::red));
}

QTEST_MAIN(CheckCore)
