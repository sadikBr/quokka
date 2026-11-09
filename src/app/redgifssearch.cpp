#include "redgifssearch.h"

#include "apiclient.h"
#include "mediasites.h"
#include "services.h"

namespace {
quokka::RedgifsBackend *redgifs()
{
    return dynamic_cast<quokka::RedgifsBackend *>(services::backendFor(QStringLiteral("redgifs")));
}

QVariantList toVariant(const QList<quokka::RedgifsBackend::Suggestion> &list)
{
    QVariantList out;
    for (const auto &s : list)
        out.append(QVariantMap{{QStringLiteral("text"), s.text},
                               {QStringLiteral("type"), s.type},
                               {QStringLiteral("count"), s.count}});
    return out;
}
} // namespace

RedgifsSearch::RedgifsSearch(QObject *parent)
    : QObject(parent)
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(250);
    connect(&m_debounce, &QTimer::timeout, this, &RedgifsSearch::fetchSuggestions);
}

void RedgifsSearch::setQuery(const QString &query)
{
    const QString q = query.trimmed();
    if (q == m_query)
        return;
    m_query = q;
    emit queryChanged();
    if (q.size() < 2) {
        if (m_reply)
            m_reply->abort();
        m_suggestions.clear();
        m_suggestionsFor.clear();
        emit suggestionsChanged();
        emit busyChanged();
        return;
    }
    m_debounce.start();
}

void RedgifsSearch::fetchSuggestions()
{
    auto *backend = redgifs();
    if (!backend || m_query == m_suggestionsFor)
        return;
    if (m_reply)
        m_reply->abort();
    const QString asked = m_query;
    quokka::ApiReply *reply = backend->suggest(asked);
    m_reply = reply;
    emit busyChanged();
    connect(reply, &quokka::ApiReply::finished, this, [this, reply, asked](const QByteArray &body) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        m_suggestionsFor = asked;
        m_suggestions = toVariant(quokka::RedgifsBackend::parseSuggestions(body));
        emit suggestionsChanged();
        emit busyChanged();
    });
    connect(reply, &quokka::ApiReply::failed, this, [this, reply] {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        emit busyChanged();
    });
}

QVariantList RedgifsSearch::trending()
{
    if (!m_trendingRequested) {
        m_trendingRequested = true;
        if (auto *backend = redgifs()) {
            quokka::ApiReply *reply = backend->trendingTags();
            connect(reply, &quokka::ApiReply::finished, this, [this](const QByteArray &body) {
                m_trending = toVariant(quokka::RedgifsBackend::parseTags(body));
                emit trendingChanged();
            });
        }
    }
    return m_trending;
}

QString RedgifsSearch::bestTag(const QString &text) const
{
    const QString t = text.trimmed();
    if (m_suggestionsFor.compare(t, Qt::CaseInsensitive) == 0) {
        for (const QVariant &v : m_suggestions) {
            const QVariantMap s = v.toMap();
            if (s.value(QStringLiteral("type")).toString() == QLatin1String("tag"))
                return s.value(QStringLiteral("text")).toString();
        }
    }
    return t;
}
