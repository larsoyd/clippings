#pragma once

#include <QHash>
#include <QImage>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QTimer>

// runs one grim per display at once, so a large display can't stall the windows
class ScreenGrabber : public QObject
{
    Q_OBJECT

  public:
    explicit ScreenGrabber(const QString& program = QStringLiteral("grim"), int timeout_ms = 5000, QObject *parent = nullptr);

    ~ScreenGrabber() override;

    bool busy() const;
    void stop();
    void grab(const QStringList& outputs, bool with_pointer);
    // grabs the area until two pictures in a row agree, or the deadline passes
    void settle(const QRect& area, int deadline_ms);

  signals:
    void grabbed(const QHash<QString, QImage>& pictures);
    void failed(const QString& reason);
    void settled(bool quiet);

  private:
    struct Run
    {
        QProcess *process;
        QString output;

        bool operator==(const Run& other) const { return process == other.process; }
    };

    void start_run(const QString& output, const QStringList& arguments);
    void run_finished(Run run, int code, QProcess::ExitStatus status);
    void stop_all();
    void fail(const QString& reason);
    void settle_step();

    QString program;
    int timeout_ms;
    QList<Run> runs;
    QHash<QString, QImage> pictures;
    QTimer deadline;

    QRect settle_area;
    QImage settle_last;
    qint64 settle_ends = 0;
    bool settling = false;
};
