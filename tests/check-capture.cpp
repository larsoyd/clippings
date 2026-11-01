#include "check-capture.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#include "../capture-session.hpp"
#include "../instance-door.hpp"
#include "../wayfire-client.hpp"

static const int OWN_PID = 4242;

class FakeCompositor : public Compositor
{
  public:
    QList<DisplayInfo> shown{{QStringLiteral("L"), QRect(-100, 0, 100, 100)}, {QStringLiteral("R"), QRect(0, 0, 100, 100)}};
    QList<WindowInfo> open;
    QHash<qint64, QImage> pictures;
    mutable QList<qint64> focused_calls;
    qint64 has_focus = 7;

    QList<DisplayInfo> displays() const override { return shown; }
    QList<WindowInfo> windows() const override { return open; }
    qint64 focused() const override { return has_focus; }
    QPoint pointer() const override { return {}; }
    bool focus(qint64 id) const override { focused_calls.append(id); return true; }
    QImage window_picture(qint64 id, const QString&) const override { return pictures.value(id); }
};

class FakeOverlays : public OverlayHost
{
  public:
    int selection_shown = 0;
    int selection_hidden = 0;
    int countdown_shown = 0;
    int countdown_hidden = 0;
    void show_selection() override { selection_shown++; }
    void hide_selection() override { selection_hidden++; }
    void show_countdown(const QPoint&) override { countdown_shown++; }
    void hide_countdown() override { countdown_hidden++; }
};

static QString write_tool(const QTemporaryDir& folder, const QByteArray& body)
{
    const QString path = folder.filePath(QStringLiteral("grim"));
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write("#!/bin/bash\n" + body);
        file.close();
        file.setPermissions(file.permissions() | QFile::ExeOwner);
    }
    return path;
}

// answers with a solid picture: 50x50 red for L, 100x100 blue for R at density 2
static const QByteArray FAKE_GRIM = R"(
name=""; prev=""
for a in "$@"; do [ "$prev" = -o ] && name=$a; prev=$a; done
echo "$name $*" >> "$(dirname "$0")/calls"
if [ "$name" = L ]; then w=100; h=100; c='\xff\x00\x00'; else w=200; h=200; c='\x00\x00\xff'; fi
[ -z "$name" ] && { w=10; h=10; c='\x00\xff\x00'; }
printf 'P6\n%d %d\n255\n' $w $h
for ((i = 0; i < w * h; i++)); do printf '%b' "$c"; done
)";

struct Rig
{
    QTemporaryDir folder;
    FakeCompositor compositor;
    FakeOverlays overlays;
    ScreenGrabber grabber;
    CaptureSession session;

    Rig() : grabber(write_tool(folder, FAKE_GRIM), 5000), session(compositor, grabber, OWN_PID)
    {
        session.set_overlays(&overlays);
        session.set_scratch_folder(folder.path());
    }

    bool frozen()
    {
        return QTest::qWaitFor([this] { return session.selecting(); }, 8000);
    }
    int grab_count()
    {
        QFile calls(folder.filePath(QStringLiteral("calls")));
        return calls.open(QIODevice::ReadOnly) ? int(calls.readAll().count('\n')) : 0;
    }
    void drag(QPoint from, QPoint to)
    {
        session.pointer_moved(from.x(), from.y());
        session.pressed(from.x(), from.y());
        session.pointer_moved(to.x(), to.y());
        session.released(to.x(), to.y());
    }
};

static QJsonObject view(int id, QRect geometry, double focus, const QString& output = QStringLiteral("R"))
{
    return {{QStringLiteral("id"), id}, {QStringLiteral("pid"), 1}, {QStringLiteral("role"), QStringLiteral("toplevel")},
            {QStringLiteral("mapped"), true}, {QStringLiteral("minimized"), false}, {QStringLiteral("output-name"), output},
            {QStringLiteral("last-focus-timestamp"), focus}, {QStringLiteral("title"), QStringLiteral("t")},
            {QStringLiteral("geometry"), QJsonObject{{QStringLiteral("x"), geometry.x()}, {QStringLiteral("y"), geometry.y()},
                                                     {QStringLiteral("width"), geometry.width()}, {QStringLiteral("height"), geometry.height()}}}};
}

void CheckCapture::outputs_filter_and_stack_windows()
{
    const QList<DisplayInfo> displays{{QStringLiteral("R"), QRect(1000, 0, 500, 500)}};
    QJsonObject minimized = view(3, QRect(0, 0, 50, 50), 30);
    minimized[QStringLiteral("minimized")] = true;
    QJsonObject helper = view(4, QRect(0, 0, 50, 50), 0);
    QJsonObject layer = view(5, QRect(0, 0, 50, 50), 50);
    layer[QStringLiteral("role")] = QStringLiteral("desktop-environment");
    QJsonObject pinned = view(6, QRect(0, 0, 10, 10), 1);
    pinned[QStringLiteral("always-on-top")] = true;

    const QList<WindowInfo> windows = windows_from(
        QJsonArray{view(1, QRect(10, 10, 100, 100), 10), view(2, QRect(20, 20, 100, 100), 20), minimized, helper, layer, pinned}, displays);
    QCOMPARE(windows.size(), 3);
    QCOMPARE(windows[0].id, 6);
    QCOMPARE(windows[1].id, 2);
    QCOMPARE(windows[2].id, 1);
    QCOMPARE(windows[1].geometry, QRect(1020, 20, 100, 100));
    QCOMPARE(window_at(windows, QPoint(1050, 50))->id, 2);
    QCOMPARE(window_at(windows, QPoint(1015, 15))->id, 1);
    QVERIFY(window_at(windows, QPoint(5, 5)) == nullptr);
}

void CheckCapture::hidden_workspaces_are_left_out()
{
    const QList<DisplayInfo> displays{{QStringLiteral("R"), QRect(0, 0, 500, 500)}};
    QCOMPARE(windows_from(QJsonArray{view(1, QRect(600, 0, 100, 100), 10)}, displays).size(), 0);
    QCOMPARE(windows_from(QJsonArray{view(1, QRect(490, 0, 100, 100), 10, QStringLiteral("gone"))}, displays).size(), 0);
}

void CheckCapture::grab_collects_every_display()
{
    QTemporaryDir folder;
    ScreenGrabber grabber(write_tool(folder, FAKE_GRIM));
    QSignalSpy spy(&grabber, &ScreenGrabber::grabbed);
    grabber.grab({QStringLiteral("L"), QStringLiteral("R")}, true);
    QVERIFY(spy.wait(8000));
    const auto pictures = spy.at(0).at(0).value<QHash<QString, QImage>>();
    QCOMPARE(pictures.size(), 2);
    QCOMPARE(pictures.value(QStringLiteral("L")).pixelColor(1, 1), QColor(Qt::red));
    QCOMPARE(pictures.value(QStringLiteral("R")).size(), QSize(200, 200));
    QFile calls(folder.filePath(QStringLiteral("calls")));
    QVERIFY(calls.open(QIODevice::ReadOnly));
    QVERIFY(calls.readAll().contains("-c"));
    QVERIFY(!grabber.busy());
}

void CheckCapture::grab_reports_a_failing_tool()
{
    QTemporaryDir folder;
    ScreenGrabber grabber(write_tool(folder, "echo broken >&2; exit 4\n"));
    QSignalSpy spy(&grabber, &ScreenGrabber::failed);
    grabber.grab({QStringLiteral("L"), QStringLiteral("R")}, false);
    QVERIFY(spy.wait(3000));
    const QString reason = spy.at(0).at(0).toString();
    QVERIFY2(reason.contains(QStringLiteral("code=4")) && reason.contains(QStringLiteral("broken")), qPrintable(reason));
    QTest::qWait(100);
    QCOMPARE(spy.size(), 1);
    QVERIFY(!grabber.busy());
}

void CheckCapture::grab_gives_up_on_a_slow_tool()
{
    QTemporaryDir folder;
    ScreenGrabber grabber(write_tool(folder, "exec sleep 30\n"), 200);
    QSignalSpy spy(&grabber, &ScreenGrabber::failed);
    QElapsedTimer clock;
    clock.start();
    grabber.grab({QStringLiteral("L")}, false);
    QVERIFY(spy.wait(3000));
    QVERIFY2(clock.elapsed() < 2000, qPrintable(QString::number(clock.elapsed())));
    QVERIFY(spy.at(0).at(0).toString().contains(QStringLiteral("200 ms")));
    QVERIFY(!grabber.busy());
}

void CheckCapture::grab_rejects_garbage()
{
    QTemporaryDir folder;
    ScreenGrabber grabber(write_tool(folder, "echo nonsense\n"));
    QSignalSpy spy(&grabber, &ScreenGrabber::failed);
    grabber.grab({QStringLiteral("L")}, false);
    QVERIFY(spy.wait(3000));
    QVERIFY(spy.at(0).at(0).toString().contains(QStringLiteral("unreadable")));
}

void CheckCapture::settle_waits_for_two_equal_pictures()
{
    QTemporaryDir folder;
    const QByteArray tool = R"(
n=$(cat "$(dirname "$0")/n" 2>/dev/null || echo 0); echo $((n + 1)) > "$(dirname "$0")/n"
printf 'P6\n1 1\n255\n'
if [ "$n" -lt 3 ]; then printf "\\x$((n + 10))\\x00\\x00"; else printf '\xff\x00\x00'; fi
)";
    ScreenGrabber grabber(write_tool(folder, tool));
    QSignalSpy spy(&grabber, &ScreenGrabber::settled);
    grabber.settle(QRect(0, 0, 10, 10), 5000);
    QVERIFY(spy.wait(5000));
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    QFile count(folder.filePath(QStringLiteral("n")));
    QVERIFY(count.open(QIODevice::ReadOnly));
    QCOMPARE(count.readAll().trimmed().toInt(), 5);
}

void CheckCapture::settle_keeps_its_deadline()
{
    QTemporaryDir folder;
    const QByteArray tool = R"(
n=$(cat "$(dirname "$0")/n" 2>/dev/null || echo 0); echo $((n + 1)) > "$(dirname "$0")/n"
printf 'P6\n1 1\n255\n'; printf "\\x$((n % 80 + 20))\\x00\\x00"
)";
    ScreenGrabber grabber(write_tool(folder, tool));
    QSignalSpy spy(&grabber, &ScreenGrabber::settled);
    QElapsedTimer clock;
    clock.start();
    grabber.settle(QRect(0, 0, 10, 10), 200);
    QVERIFY(spy.wait(3000));
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    QVERIFY2(clock.elapsed() >= 190 && clock.elapsed() < 1500, qPrintable(QString::number(clock.elapsed())));
}

void CheckCapture::capture_freezes_before_showing_anything()
{
    Rig rig;
    QCOMPARE(rig.session.state(), CaptureSession::State::Idle);
    QVERIFY(rig.session.begin(CaptureSession::Selection, false));
    QCOMPARE(rig.overlays.selection_shown, 0);
    QVERIFY(rig.frozen());
    QCOMPARE(rig.overlays.selection_shown, 1);
    QCOMPARE(rig.grab_count(), 2);
    QCOMPARE(rig.session.desktop().bounds(), QRect(-100, 0, 200, 100));
}

void CheckCapture::second_begin_is_refused_and_changes_nothing()
{
    Rig rig;
    QVERIFY(rig.session.begin(CaptureSession::Selection, false));
    QVERIFY(!rig.session.begin(CaptureSession::Screen, false));
    QVERIFY(rig.frozen());
    rig.drag(QPoint(10, 10), QPoint(40, 40));
    QVERIFY(!rig.session.begin(CaptureSession::Screen, false));
    QCOMPARE(rig.session.mode(), int(CaptureSession::Selection));
    QCOMPARE(rig.session.selection(), QRect(10, 10, 30, 30));
    QCOMPARE(rig.overlays.selection_shown, 1);
    QCOMPARE(rig.grab_count(), 2);
}

void CheckCapture::drag_selects_and_accept_composes()
{
    Rig rig;
    QSignalSpy spy(&rig.session, &CaptureSession::finished);
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.drag(QPoint(10, 10), QPoint(41, 31));
    QCOMPARE(rig.session.selection(), QRect(10, 10, 31, 21));
    rig.session.accept();
    QCOMPARE(spy.size(), 1);
    const QImage image = spy.at(0).at(0).value<QImage>();
    QCOMPARE(image.size(), QSize(62, 42));
    QCOMPARE(image.pixelColor(3, 3), QColor(Qt::blue));
    QCOMPARE(spy.at(0).at(1).toDouble(), 2.0);
    QCOMPARE(rig.session.state(), CaptureSession::State::Idle);
    QVERIFY(rig.overlays.selection_hidden >= 1);
}

void CheckCapture::selection_crosses_displays()
{
    Rig rig;
    QSignalSpy spy(&rig.session, &CaptureSession::finished);
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.drag(QPoint(-20, 10), QPoint(19, 29));
    rig.session.accept();
    const QImage image = spy.at(0).at(0).value<QImage>();
    QCOMPARE(image.size(), QSize(78, 38));
    QCOMPARE(image.pixelColor(5, 5), QColor(Qt::red));
    QCOMPARE(image.pixelColor(70, 5), QColor(Qt::blue));
}

void CheckCapture::small_drag_chooses_nothing()
{
    Rig rig;
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.drag(QPoint(10, 10), QPoint(11, 11));
    QVERIFY(rig.session.selection().isEmpty());
}

void CheckCapture::empty_selection_does_not_accept()
{
    Rig rig;
    QSignalSpy spy(&rig.session, &CaptureSession::finished);
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.session.accept();
    QCOMPARE(spy.size(), 0);
    QVERIFY(rig.session.selecting());
}

void CheckCapture::selection_moves_and_resizes()
{
    Rig rig;
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.drag(QPoint(20, 20), QPoint(50, 50));
    const QRect first = rig.session.selection();
    rig.drag(QPoint(30, 30), QPoint(35, 37));
    QCOMPARE(rig.session.selection(), first.translated(5, 7));
    const QRect second = rig.session.selection();
    const QPoint corner = second.topLeft() + QPoint(second.width(), second.height());
    rig.drag(corner, corner + QPoint(10, 5));
    QCOMPARE(rig.session.selection().topLeft(), second.topLeft());
    QCOMPARE(rig.session.selection().size(), second.size() + QSize(10, 5));
    const QRect third = rig.session.selection();
    const QPoint left_edge(third.left(), third.top() + third.height() / 2);
    rig.drag(left_edge, left_edge + QPoint(-8, 20));
    QCOMPARE(rig.session.selection().left(), third.left() - 8);
    QCOMPARE(rig.session.selection().top(), third.top());
    QCOMPARE(rig.session.selection().height(), third.height());
    rig.drag(rig.session.selection().center(), QPoint(5000, 5000));
    QCOMPARE(rig.session.selection().right(), 99);
    QCOMPARE(rig.session.selection().bottom(), 99);
}

void CheckCapture::keyboard_nudges_selection_inside_desktop()
{
    Rig rig;
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.session.pointer_moved(50, 50);
    rig.session.nudge(1, 0, false);
    QVERIFY(!rig.session.selection().isEmpty());
    rig.drag(QPoint(-60, 40), QPoint(-20, 60));
    const QRect before = rig.session.selection();
    rig.session.nudge(1, 0, false);
    QCOMPARE(rig.session.selection(), before.translated(1, 0));
    rig.session.nudge(10, 5, true);
    QCOMPARE(rig.session.selection().size(), before.size() + QSize(10, 5));
    rig.session.nudge(-5000, -5000, false);
    QCOMPARE(rig.session.selection().topLeft(), QPoint(-100, 0));
    rig.session.nudge(-5000, -5000, true);
    QCOMPARE(rig.session.selection().size(), QSize(1, 1));
}

void CheckCapture::window_mode_uses_the_window_picture()
{
    Rig rig;
    WindowInfo window;
    window.id = 11;
    window.pid = 1;
    window.geometry = QRect(10, 10, 30, 30);
    window.title = QStringLiteral("Note");
    rig.compositor.open = {window};
    QImage picture(60, 60, QImage::Format_ARGB32_Premultiplied);
    picture.fill(QColor(0, 200, 0, 128));
    rig.compositor.pictures.insert(11, picture);

    QSignalSpy spy(&rig.session, &CaptureSession::finished);
    rig.session.begin(CaptureSession::Window, false);
    QVERIFY(rig.frozen());
    rig.session.pointer_moved(20, 20);
    QCOMPARE(rig.session.target(), window.geometry);
    QCOMPARE(rig.session.target_title(), QStringLiteral("Note"));
    rig.session.pointer_moved(80, 80);
    QVERIFY(rig.session.target().isEmpty());
    rig.session.released(80, 80);
    QCOMPARE(spy.size(), 0);
    rig.session.pointer_moved(20, 20);
    rig.session.released(20, 20);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).value<QImage>().pixelColor(5, 5).alpha(), 128);
    QCOMPARE(spy.at(0).at(1).toDouble(), 2.0);
}

void CheckCapture::window_mode_falls_back_to_the_frozen_screen()
{
    Rig rig;
    WindowInfo window;
    window.id = 12;
    window.pid = 1;
    window.geometry = QRect(10, 10, 30, 30);
    rig.compositor.open = {window};
    QSignalSpy spy(&rig.session, &CaptureSession::finished);
    rig.session.begin(CaptureSession::Window, false);
    QVERIFY(rig.frozen());
    rig.session.pointer_moved(20, 20);
    rig.session.accept();
    QCOMPARE(spy.at(0).at(0).value<QImage>().pixelColor(5, 5), QColor(Qt::blue));
}

void CheckCapture::screen_and_all_modes()
{
    Rig rig;
    QSignalSpy spy(&rig.session, &CaptureSession::finished);
    rig.session.begin(CaptureSession::Screen, false);
    QVERIFY(rig.frozen());
    rig.session.pointer_moved(-50, 50);
    QCOMPARE(rig.session.target(), QRect(-100, 0, 100, 100));
    QCOMPARE(rig.session.pixel_size(), QSize(100, 100));
    rig.session.set_mode(CaptureSession::AllScreens);
    QCOMPARE(rig.session.target(), QRect(-100, 0, 200, 100));
    QCOMPARE(rig.session.pixel_size(), QSize(400, 200));
    rig.session.accept();
    const QImage image = spy.at(0).at(0).value<QImage>();
    QCOMPARE(image.size(), QSize(400, 200));
}

void CheckCapture::cancel_restores_focus_and_gives_nothing()
{
    Rig rig;
    QSignalSpy done(&rig.session, &CaptureSession::finished);
    QSignalSpy cancelled(&rig.session, &CaptureSession::cancelled);
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.drag(QPoint(10, 10), QPoint(40, 40));
    rig.session.cancel();
    QCOMPARE(done.size(), 0);
    QCOMPARE(cancelled.size(), 1);
    QCOMPARE(rig.compositor.focused_calls, QList<qint64>{7});
    QCOMPARE(rig.session.state(), CaptureSession::State::Idle);
    QVERIFY(rig.session.desktop().empty());
    QVERIFY(rig.session.begin(CaptureSession::Selection, false));
}

void CheckCapture::cancel_while_grabbing_stops_the_grab()
{
    Rig rig;
    QVERIFY(rig.session.begin(CaptureSession::Selection, false));
    QVERIFY(rig.grabber.busy());
    rig.session.cancel();
    QVERIFY(!rig.grabber.busy());
    QTest::qWait(300);
    QCOMPARE(rig.overlays.selection_shown, 0);
    QCOMPARE(rig.session.state(), CaptureSession::State::Idle);
}

void CheckCapture::failed_grab_ends_the_capture()
{
    QTemporaryDir folder;
    FakeCompositor compositor;
    FakeOverlays overlays;
    ScreenGrabber grabber(write_tool(folder, "exit 1\n"));
    CaptureSession session(compositor, grabber, OWN_PID);
    session.set_overlays(&overlays);
    QSignalSpy spy(&session, &CaptureSession::failed);
    QVERIFY(session.begin(CaptureSession::Selection, false));
    QVERIFY(spy.wait(3000));
    QCOMPARE(session.state(), CaptureSession::State::Idle);
    QCOMPARE(overlays.selection_shown, 0);
}

void CheckCapture::display_change_ends_the_capture()
{
    Rig rig;
    QSignalSpy spy(&rig.session, &CaptureSession::failed);
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.session.displays_changed();
    QCOMPARE(spy.size(), 1);
    QVERIFY(rig.overlays.selection_hidden >= 1);
    QCOMPARE(rig.session.state(), CaptureSession::State::Idle);
}

static WindowInfo own_window()
{
    WindowInfo window;
    window.id = 21;
    window.pid = OWN_PID;
    window.geometry = QRect(10, 10, 30, 30);
    return window;
}

void CheckCapture::leaving_out_own_windows_waits_for_them()
{
    Rig rig;
    rig.compositor.open = {own_window()};
    QSignalSpy hide(&rig.session, &CaptureSession::hideOwnWindows);
    rig.session.begin(CaptureSession::Selection, true);
    QCOMPARE(hide.size(), 1);
    QCOMPARE(rig.session.state(), CaptureSession::State::Hiding);
    QTest::qWait(120);
    QCOMPARE(rig.session.state(), CaptureSession::State::Hiding);
    QCOMPARE(rig.overlays.selection_shown, 0);
    rig.compositor.open.clear();
    QVERIFY(rig.frozen());
    QFile calls(rig.folder.filePath(QStringLiteral("calls")));
    QVERIFY(calls.open(QIODevice::ReadOnly));
    QVERIFY(calls.readAll().contains("-g"));
}

void CheckCapture::own_windows_return_after_cancel()
{
    Rig rig;
    rig.compositor.open = {own_window()};
    QSignalSpy show(&rig.session, &CaptureSession::showOwnWindows);
    rig.session.begin(CaptureSession::Selection, true);
    rig.compositor.open.clear();
    QVERIFY(rig.frozen());
    rig.session.cancel();
    QCOMPARE(show.size(), 1);
}

void CheckCapture::countdown_grabs_a_fresh_picture()
{
    Rig rig;
    QSignalSpy spy(&rig.session, &CaptureSession::finished);
    rig.session.set_delay(1);
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.drag(QPoint(10, 10), QPoint(40, 40));
    const int before = rig.grab_count();
    rig.session.accept();
    QCOMPARE(rig.session.state(), CaptureSession::State::Counting);
    QCOMPARE(rig.overlays.countdown_shown, 1);
    QCOMPARE(spy.size(), 0);
    QVERIFY(spy.wait(9000));
    QVERIFY(rig.grab_count() > before + 2);
    QVERIFY(rig.overlays.countdown_hidden >= 1);
    QCOMPARE(spy.at(0).at(0).value<QImage>().size(), QSize(60, 60));
}

void CheckCapture::cancel_during_countdown_grabs_nothing()
{
    Rig rig;
    QSignalSpy spy(&rig.session, &CaptureSession::finished);
    rig.session.set_delay(1);
    rig.session.begin(CaptureSession::Selection, false);
    QVERIFY(rig.frozen());
    rig.drag(QPoint(10, 10), QPoint(40, 40));
    rig.session.accept();
    const int before = rig.grab_count();
    rig.session.cancel();
    QTest::qWait(1300);
    QCOMPARE(spy.size(), 0);
    QCOMPARE(rig.grab_count(), before);
}

void CheckCapture::door_hands_over_and_answers()
{
    QTemporaryDir folder;
    const QString path = folder.filePath(QStringLiteral("s"));
    InstanceDoor door;
    QVERIFY(door.listen(path));
    QSignalSpy spy(&door, &InstanceDoor::commandReceived);
    bool answered = false;
    QThread *thread = QThread::create([&] { answered = InstanceDoor::hand_over(path, {{QStringLiteral("capture"), QStringLiteral("selection")}}); });
    thread->start();
    QVERIFY(QTest::qWaitFor([&] { return thread->isFinished(); }, 4000));
    delete thread;
    QVERIFY(answered);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toJsonObject().value(QLatin1String("capture")).toString(), QStringLiteral("selection"));
}

void CheckCapture::door_without_listener_refuses()
{
    QTemporaryDir folder;
    QVERIFY(!InstanceDoor::hand_over(folder.filePath(QStringLiteral("s")), {}));
}

void CheckCapture::door_replaces_a_dead_socket()
{
    QTemporaryDir folder;
    const QString path = folder.filePath(QStringLiteral("s"));
    QFile stale(path);
    QVERIFY(stale.open(QIODevice::WriteOnly));
    stale.close();
    InstanceDoor door;
    QVERIFY(door.listen(path));
}

void CheckCapture::door_keeps_a_live_socket()
{
    QTemporaryDir folder;
    const QString path = folder.filePath(QStringLiteral("s"));
    InstanceDoor first;
    QVERIFY(first.listen(path));
    InstanceDoor second;
    QVERIFY(!second.listen(path));
}

void CheckCapture::start_lock_excludes_a_second_holder()
{
    QTemporaryDir folder;
    const QString path = folder.filePath(QStringLiteral("l"));
    StartLock first(path);
    QVERIFY(first.held());
    QElapsedTimer clock;
    clock.start();
    StartLock second(path, 100);
    QVERIFY(!second.held());
    QVERIFY(clock.elapsed() >= 100);
    first.release();
    StartLock third(path, 100);
    QVERIFY(third.held());
}

QTEST_MAIN(CheckCapture)
