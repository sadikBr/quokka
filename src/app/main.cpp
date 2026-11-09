#include "library.h"
#include "networkfactory.h"
#include "redditclient.h"
#include "services.h"

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QtGlobal>

#include <memory>

namespace {
quokka::RedditClient *g_reddit = nullptr;
}

quokka::RedditClient *services::reddit()
{
    return g_reddit;
}

void services::setReddit(quokka::RedditClient *client)
{
    g_reddit = client;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Quokka"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Quokka"));
    QGuiApplication::setOrganizationName(QStringLiteral("Quokka"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("sadikbr.github.io"));
    QGuiApplication::setApplicationVersion(QStringLiteral(QUOKKA_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.sadikbr.quokka"));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/qt/qml/Quokka/resources/logo.svg")));

    // One style everywhere; Theme.qml supplies the colours.
    QQuickStyle::setStyle(QStringLiteral("Material"));

    NetworkFactory networkFactory(quokka::RedditClient::defaultUserAgent());

    // QUOKKA_FIXTURES=<dir> serves recorded JSON instead of hitting Reddit (offline UI work).
    std::unique_ptr<quokka::Transport> transport;
    const QString fixtures = qEnvironmentVariable("QUOKKA_FIXTURES");
    QObject apiNamOwner;
    if (!fixtures.isEmpty()) {
        transport = std::make_unique<quokka::FixtureTransport>(fixtures);
    } else {
        transport = std::make_unique<quokka::PublicJsonTransport>(
            networkFactory.create(&apiNamOwner), quokka::RedditClient::defaultUserAgent());
    }
    quokka::RedditClient reddit(std::move(transport));
    services::setReddit(&reddit);

    Library library;
    Library::setInstance(&library);

    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(&networkFactory);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    engine.loadFromModule("Quokka", "Main");
#else
    engine.load(QUrl(QStringLiteral("qrc:/Quokka/Main.qml")));
#endif

    // QUOKKA_SCREENSHOT=<file.png>[:<delay ms>] saves the window and quits (docs, CI checks).
    const QString shot = qEnvironmentVariable("QUOKKA_SCREENSHOT");
    if (!shot.isEmpty() && !engine.rootObjects().isEmpty()) {
        const int sep = shot.lastIndexOf(QLatin1Char(':'));
        bool ok = false;
        const int delay = sep > 0 ? shot.mid(sep + 1).toInt(&ok) : 0;
        const QString file = ok ? shot.left(sep) : shot;
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        // QUOKKA_WINDOW_SIZE=<w>x<h>, e.g. 412x900 for a phone-sized capture.
        const QStringList size = qEnvironmentVariable("QUOKKA_WINDOW_SIZE").split(QLatin1Char('x'));
        if (window && size.size() == 2)
            window->resize(size[0].toInt(), size[1].toInt());
        QTimer::singleShot(ok ? delay : 2500, &app, [window, file] {
            if (window)
                window->grabWindow().save(file);
            QCoreApplication::exit(0);
        });
    }

    const int rc = app.exec();
    services::setReddit(nullptr);
    return rc;
}
