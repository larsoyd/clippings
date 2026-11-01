#include "export.hpp"

#include <QBuffer>
#include <QFileInfo>
#include <QImageWriter>
#include <QPainter>
#include <QProcess>
#include <QSaveFile>

static const int COPY_TIMEOUT_MS = 3000;
static const int JPEG_QUALITY = 92;

QStringList image_formats()
{
    QStringList known;
    const QList<QByteArray> supported = QImageWriter::supportedImageFormats();
    for (const char *format : {"png", "jpeg", "tiff", "webp", "bmp"})
    {
        if (supported.contains(format))
        {
            known.append(QString::fromLatin1(format));
        }
    }
    return known;
}

static QByteArray format_for(const QString& path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("jpg"))
    {
        return "jpeg";
    }
    if (suffix == QLatin1String("tif"))
    {
        return "tiff";
    }
    return suffix.toLatin1();
}

static QImage on_white(const QImage& image)
{
    QImage opaque(image.size(), QImage::Format_RGB32);
    opaque.fill(Qt::white);
    QPainter(&opaque).drawImage(0, 0, image);
    return opaque;
}

QString write_image(const QImage& image, const QString& path)
{
    const QByteArray format = format_for(path);
    if (!QImageWriter::supportedImageFormats().contains(format))
    {
        return QStringLiteral("The format “%1” can’t be written.").arg(QString::fromLatin1(format));
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return file.errorString();
    }

    QImageWriter writer(&file, format);
    if (format == "jpeg")
    {
        writer.setQuality(JPEG_QUALITY);
    }
    if (!writer.write(format == "jpeg" ? on_white(image) : image))
    {
        file.cancelWriting();
        return writer.errorString();
    }
    return file.commit() ? QString() : file.errorString();
}

QString copy_image(const QImage& image, const QString& wl_copy)
{
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG"))
    {
        return QStringLiteral("The image could not be encoded.");
    }

    QProcess process;
    process.start(wl_copy, {QStringLiteral("--type"), QStringLiteral("image/png")});
    if (!process.waitForStarted(COPY_TIMEOUT_MS))
    {
        return QStringLiteral("%1 could not be started.").arg(wl_copy);
    }

    process.write(png);
    process.closeWriteChannel();
    if (!process.waitForFinished(COPY_TIMEOUT_MS))
    {
        process.kill();
        process.waitForFinished(COPY_TIMEOUT_MS);
        return QStringLiteral("%1 did not answer.").arg(wl_copy);
    }
    return process.exitCode() == 0 ? QString() : QStringLiteral("%1 failed with status %2.").arg(wl_copy).arg(process.exitCode());
}
