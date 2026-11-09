#pragma once

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QTimer>
#include <QVariantList>

namespace quokka {
class ApiReply;
}

// Redgifs search helpers for QML: suggestions while typing (tags with clip counts, creators,
// niches) and the tags trending now. Each item: { text, type, count }.
class RedgifsSearch : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(QVariantList suggestions READ suggestions NOTIFY suggestionsChanged)
    Q_PROPERTY(QVariantList trending READ trending NOTIFY trendingChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit RedgifsSearch(QObject *parent = nullptr);

    QString query() const { return m_query; }
    void setQuery(const QString &query);   // suggestions follow, shortly after typing stops
    QVariantList suggestions() const { return m_suggestions; }
    QVariantList trending();               // loaded on first use
    bool busy() const { return m_reply != nullptr; }

    // The best tag for free text: the top tag suggestion, else the text itself.
    Q_INVOKABLE QString bestTag(const QString &text) const;

signals:
    void queryChanged();
    void suggestionsChanged();
    void trendingChanged();
    void busyChanged();

private:
    void fetchSuggestions();

    QString m_query;
    QString m_suggestionsFor;
    QVariantList m_suggestions;
    QVariantList m_trending;
    bool m_trendingRequested = false;
    QPointer<quokka::ApiReply> m_reply;
    QTimer m_debounce;
};
