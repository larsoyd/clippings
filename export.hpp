#pragma once

#include <QImage>
#include <QString>
#include <QStringList>

// the formats save offers, png first
QStringList image_formats();

// atomic, so a failed write leaves the old file whole. empty on success, otherwise the reason
QString write_image(const QImage& image, const QString& path);

// through wl-copy, whose daemon keeps the image after this program quits. empty on success
QString copy_image(const QImage& image, const QString& wl_copy = QStringLiteral("wl-copy"));
