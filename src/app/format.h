#pragma once

#include <QColor>
#include <QObject>
#include <QQmlEngine>

// Small formatting helpers for QML: `Format.relativeTime(created)`, `Format.compact(12345)`...
class Format : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Format(QObject *parent = nullptr);

    // "now", "5m", "3h", "2d", "4mo", "1y"
    Q_INVOKABLE static QString relativeTime(qint64 createdUtc);
    static QString relativeTime(qint64 createdUtc, qint64 nowUtc);

    // Long form: "12 days", "5 months", "3 years".
    Q_INVOKABLE static QString age(qint64 createdUtc);
    static QString age(qint64 createdUtc, qint64 nowUtc);

    // 999 -> "999", 1234 -> "1.2k", 45678 -> "45.7k", 1234567 -> "1.2M"
    Q_INVOKABLE static QString compact(qint64 n);

    // A stable, cheerful colour for a subreddit / user name.
    Q_INVOKABLE static QColor nameColor(const QString &name);

    // The Qt version the app runs with, e.g. "6.8.3".
    Q_INVOKABLE static QString qtVersion();

    // "/r/x/comments/..." -> "https://www.reddit.com/r/x/comments/..."
    Q_INVOKABLE static QString webUrl(const QString &permalink);
};
