#include "format.h"

#include <QDateTime>

Format::Format(QObject *parent)
    : QObject(parent)
{
}

QString Format::relativeTime(qint64 createdUtc)
{
    return relativeTime(createdUtc, QDateTime::currentSecsSinceEpoch());
}

QString Format::relativeTime(qint64 createdUtc, qint64 nowUtc)
{
    const qint64 s = qMax<qint64>(0, nowUtc - createdUtc);
    if (s < 60)
        return QStringLiteral("now");
    if (s < 3600)
        return QStringLiteral("%1m").arg(s / 60);
    if (s < 86400)
        return QStringLiteral("%1h").arg(s / 3600);
    if (s < 86400 * 30)
        return QStringLiteral("%1d").arg(s / 86400);
    if (s < 86400 * 365)
        return QStringLiteral("%1mo").arg(s / (86400 * 30));
    return QStringLiteral("%1y").arg(s / (86400 * 365));
}

QString Format::compact(qint64 n)
{
    const bool negative = n < 0;
    const double v = double(negative ? -n : n);
    QString out;
    auto oneDecimal = [](double x, const char *suffix) {
        QString s = QString::number(x, 'f', x < 100 ? 1 : 0);
        if (s.endsWith(QLatin1String(".0")))
            s.chop(2);
        return s + QLatin1String(suffix);
    };
    if (v < 1000)
        out = QString::number(qint64(v));
    else if (v < 999950)
        out = oneDecimal(v / 1000.0, "k");
    else
        out = oneDecimal(v / 1000000.0, "M");
    return negative ? QLatin1Char('-') + out : out;
}

QColor Format::nameColor(const QString &name)
{
    // Hand-picked palette that reads well on both light and dark cards.
    static const QColor palette[] = {
        QColor(0xFF5E7E), QColor(0xFF9F43), QColor(0xF7C531), QColor(0x3DD6B0),
        QColor(0x4CC9F0), QColor(0x6C8CFF), QColor(0xA66CFF), QColor(0xFF6CCB),
    };
    const size_t n = sizeof(palette) / sizeof(palette[0]);
    return palette[qHash(name.toLower()) % n];
}

QString Format::webUrl(const QString &permalink)
{
    if (permalink.startsWith(QLatin1String("http")))
        return permalink;
    return QStringLiteral("https://www.reddit.com") + permalink;
}
