#include "screen-grabber.hpp"

#include <QDateTime>

#include <cstdio>

ScreenGrabber::ScreenGrabber(const QString& grim, int timeout, QObject *parent)
    : QObject(parent), program(grim), timeout_ms(timeout)
{
    deadline.setSingleShot(true);
    connect(&deadline, &QTimer::timeout, this, [this] {
        fail(QStringLiteral("%1 gave no picture within %2 ms").arg(program).arg(timeout_ms));
    });
}

ScreenGrabber::~ScreenGrabber()
{
    stop_all();
}

void ScreenGrabber::stop()
{
    deadline.stop();
    settling = false;
    stop_all();
}

bool ScreenGrabber::busy() const
{
    return !runs.isEmpty();
}

void ScreenGrabber::grab(const QStringList& outputs, bool with_pointer)
{
    if (busy() || outputs.isEmpty())
    {
        emit failed(outputs.isEmpty() ? QStringLiteral("no displays") : QStringLiteral("a capture is running"));
        return;
    }

    pictures.clear();
    settling = false;
    deadline.start(timeout_ms);
    for (const QString& output : outputs)
    {
        QStringList arguments{QStringLiteral("-t"), QStringLiteral("ppm"), QStringLiteral("-o"), output};
        if (with_pointer)
        {
            arguments.prepend(QStringLiteral("-c"));
        }
        arguments.append(QStringLiteral("-"));
        start_run(output, arguments);
    }
}

void ScreenGrabber::settle(const QRect& area, int deadline_ms)
{
    settling = true;
    settle_area = area;
    settle_last = QImage();
    settle_ends = QDateTime::currentMSecsSinceEpoch() + deadline_ms;
    settle_step();
}

void ScreenGrabber::settle_step()
{
    pictures.clear();
    deadline.start(timeout_ms);
    const QString geometry = QStringLiteral("%1,%2 %3x%4").arg(settle_area.x()).arg(settle_area.y())
                                 .arg(settle_area.width()).arg(settle_area.height());
    start_run(QString(), {QStringLiteral("-t"), QStringLiteral("ppm"), QStringLiteral("-g"), geometry, QStringLiteral("-")});
}

void ScreenGrabber::start_run(const QString& output, const QStringList& arguments)
{
    auto *process = new QProcess(this);
    const Run run{process, output};
    runs.append(run);
    connect(process, &QProcess::finished, this, [this, run](int code, QProcess::ExitStatus status) {
        run_finished(run, code, status);
    });
    connect(process, &QProcess::errorOccurred, this, [this, run](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
        {
            fail(QStringLiteral("%1 could not be started").arg(program));
        }
    });
    process->start(program, arguments);
}

void ScreenGrabber::run_finished(Run run, int code, QProcess::ExitStatus status)
{
    runs.removeOne(run);
    const QByteArray bytes = run.process->readAllStandardOutput();
    const QByteArray errors = run.process->readAllStandardError().trimmed();
    run.process->deleteLater();
    if (status != QProcess::NormalExit || code != 0)
    {
        fail(QStringLiteral("%1 failed output=%2 code=%3 %4").arg(program, run.output).arg(code).arg(QString::fromUtf8(errors)));
        return;
    }

    QImage picture;
    if (!picture.loadFromData(bytes, "PPM"))
    {
        fail(QStringLiteral("%1 gave an unreadable picture output=%2 bytes=%3").arg(program, run.output).arg(bytes.size()));
        return;
    }

    if (settling)
    {
        const bool quiet = picture == settle_last;
        settle_last = picture;
        const bool out_of_time = QDateTime::currentMSecsSinceEpoch() >= settle_ends;
        if (quiet || out_of_time)
        {
            deadline.stop();
            settling = false;
            emit settled(quiet);
        }
        else
        {
            settle_step();
        }
        return;
    }

    pictures.insert(run.output, picture);
    if (runs.isEmpty())
    {
        deadline.stop();
        emit grabbed(pictures);
    }
}

void ScreenGrabber::stop_all()
{
    const QList<Run> running = runs;
    runs.clear();
    for (const Run& run : running)
    {
        run.process->disconnect(this);
        run.process->kill();
        run.process->waitForFinished(500);
        run.process->deleteLater();
    }
}

void ScreenGrabber::fail(const QString& reason)
{
    deadline.stop();
    stop_all();
    settling = false;
    std::fprintf(stderr, "[grab] failed reason=%s\n", qPrintable(reason));
    emit failed(reason);
}
