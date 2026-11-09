#include "mediaresolver.h"

#include <QTest>

class TestMediaResolver : public QObject
{
    Q_OBJECT

private slots:
    void ids()
    {
        QCOMPARE(MediaResolver::redgifsId("https://www.redgifs.com/watch/SteelBothPeafowl"), QStringLiteral("steelbothpeafowl"));
        QCOMPARE(MediaResolver::redgifsId("https://www.redgifs.com/ifr/steelbothpeafowl"), QStringLiteral("steelbothpeafowl"));
        QCOMPARE(MediaResolver::redgifsId("https://v3.redgifs.com/watch/abc-some-title"), QStringLiteral("abc"));
        QVERIFY(MediaResolver::redgifsId("https://www.redgifs.com/").isEmpty());
        QCOMPARE(MediaResolver::streamableCode("https://streamable.com/xyz12"), QStringLiteral("xyz12"));
    }

    void skipsDeadImages()
    {
        MediaResolver r;
        r.setConverterEnabled(false);
        const QString a = "https://down.example/pictrs/image/a.jpeg?thumbnail=640&format=jpg";
        QCOMPARE(r.imageSources(a).size(), 4);
        r.reportImage(a, false);
        QCOMPARE(r.imageSources(a).size(), 3); // that copy is skipped from now on
        // Six failures and no success: the whole server is treated as down.
        for (int i = 0; i < 6; ++i)
            r.reportImage(QStringLiteral("https://down.example/pictrs/image/%1.png").arg(i), false);
        QVERIFY(r.imageSources(a).isEmpty());
        QCOMPARE(r.imageSources(a, "https://up.example/b.jpg"), QStringList{"https://up.example/b.jpg"});
        // A server that worked once is never written off.
        r.reportImage("https://ok.example/x.png", true);
        for (int i = 0; i < 8; ++i)
            r.reportImage(QStringLiteral("https://ok.example/%1.png").arg(i), false);
        QCOMPARE(r.imageSources("https://ok.example/new.png").size(), 1);
    }

    void converter()
    {
        QCOMPARE(MediaResolver::convertedUrl("https://i.ibb.co/cS9JXXCG/1000032992.png"),
                 QStringLiteral("https://wsrv.nl/?url=https%3A%2F%2Fi.ibb.co%2FcS9JXXCG%2F1000032992.png&w=1600&we&output=jpg"));
        // pict-rs resizing options are dropped: the service converts the original.
        QVERIFY(MediaResolver::convertedUrl("https://a.org/pictrs/image/x.png?thumbnail=640&format=jpg")
                    .startsWith("https://wsrv.nl/?url=https%3A%2F%2Fa.org%2Fpictrs%2Fimage%2Fx.png&"));

        MediaResolver r;
        const bool was = r.converterEnabled();
        r.setConverterEnabled(true);
        const QStringList s = r.imageSources("https://i.ibb.co/a.png");
        QCOMPARE(s.size(), 2);
        QVERIFY(s.last().startsWith("https://wsrv.nl/"));
        // Conversion failures never write the service off.
        for (int i = 0; i < 10; ++i)
            r.reportImage(MediaResolver::convertedUrl(QStringLiteral("https://gone.example/%1.png").arg(i)), false);
        QCOMPARE(r.imageSources("https://i.ibb.co/b.png").size(), 2);
        r.setConverterEnabled(was);
    }

    void animationCheck()
    {
        // A 3-frame 8x8 GIF.
        const QByteArray gif = QByteArray::fromBase64(
            "R0lGODlhCAAIAPcfMQAAACQAAEgAAGwAAJAAALQAANgAAPwAAAAkACQkAEgkAGwkAJAkALQkANgkAPwkAABIACRIAEhIAGxIAJBIALRIANhIAPxIAABsACRsAEhsAGxsAJBsALRsANhsAPxsAACQACSQAEiQAGyQAJCQALSQANiQAPyQAAC0ACS0AEi0AGy0AJC0ALS0ANi0APy0AADYACTYAEjYAGzYAJDYALTYANjYAPzYAAD8ACT8AEj8AGz8AJD8ALT8ANj8APz8AAAAVSQAVUgAVWwAVZAAVbQAVdgAVfwAVQAkVSQkVUgkVWwkVZAkVbQkVdgkVfwkVQBIVSRIVUhIVWxIVZBIVbRIVdhIVfxIVQBsVSRsVUhsVWxsVZBsVbRsVdhsVfxsVQCQVSSQVUiQVWyQVZCQVbSQVdiQVfyQVQC0VSS0VUi0VWy0VZC0VbS0Vdi0Vfy0VQDYVSTYVUjYVWzYVZDYVbTYVdjYVfzYVQD8VST8VUj8VWz8VZD8VbT8Vdj8Vfz8VQAAqiQAqkgAqmwAqpAAqrQAqtgAqvwAqgAkqiQkqkgkqmwkqpAkqrQkqtgkqvwkqgBIqiRIqkhIqmxIqpBIqrRIqthIqvxIqgBsqiRsqkhsqmxsqpBsqrRsqthsqvxsqgCQqiSQqkiQqmyQqpCQqrSQqtiQqvyQqgC0qiS0qki0qmy0qpC0qrS0qti0qvy0qgDYqiTYqkjYqmzYqpDYqrTYqtjYqvzYqgD8qiT8qkj8qmz8qpD8qrT8qtj8qvz8qgAA/yQA/0gA/2wA/5AA/7QA/9gA//wA/wAk/yQk/0gk/2wk/5Ak/7Qk/9gk//wk/wBI/yRI/0hI/2xI/5BI/7RI/9hI//xI/wBs/yRs/0hs/2xs/5Bs/7Rs/9hs//xs/wCQ/ySQ/0iQ/2yQ/5CQ/7SQ/9iQ//yQ/wC0/yS0/0i0/2y0/5C0/7S0/9i0//y0/wDY/yTY/0jY/2zY/5DY/7TY/9jY//zY/wD8/yT8/0j8/2z8/5D8/7T8/9j8//z8/yH/C05FVFNDQVBFMi4wAwEAAAAh+QQEIQAfACwAAAAACAAIAAAINQABHMDxA9gxfP8EHgP2A8eBhPgWNjwAAEBEhg4rXpyoUWLGAzd44MEHjdihAy924CA57FhAACH5BAUiAAAALAAABgAIAAIAAAgTAC/8yIELXTBjRwQSRAfs2JGAAAAh+QQFIQAAACwAAAYACAACAAAIEwBf8MCBD9uwYwcEEoQ27NCBgAAAOw==");
        int frames = 0;
        QVERIFY(MediaResolver::isSafeAnimation(gif, &frames));
        QCOMPARE(frames, 3);
        // Cut off in the middle, or damaged: never handed to Qt's GIF player.
        QVERIFY(!MediaResolver::isSafeAnimation(gif.left(gif.size() * 2 / 3)));
        QVERIFY(!MediaResolver::isSafeAnimation(gif.left(40)));
        QVERIFY(!MediaResolver::isSafeAnimation(QByteArray("<html>not a gif</html>")));
        QVERIFY(!MediaResolver::isSafeAnimation({}));
    }

    void messages()
    {
        QVERIFY(MediaResolver::describeFailure(410, "https://www.redgifs.com/watch/x").contains("deleted from redgifs.com"));
        QVERIFY(MediaResolver::describeFailure(502, "https://x.org/a").contains("trouble"));
        QVERIFY(MediaResolver::describePlaybackError("Could not open file", "https://lemmynsfw.com/a.mp4").contains("lemmynsfw.com"));
    }

    void parsing()
    {
        QCOMPARE(MediaResolver::parseRedgifs("{\"gif\":{\"urls\":{\"sd\":\"https://media.redgifs.com/a-mobile.mp4\",\"hd\":\"https://media.redgifs.com/a.mp4\"}}}"),
                 QStringLiteral("https://media.redgifs.com/a.mp4"));
        QCOMPARE(MediaResolver::parseRedgifs("{\"gif\":{\"urls\":{\"sd\":\"https://media.redgifs.com/a-mobile.mp4\"}}}"),
                 QStringLiteral("https://media.redgifs.com/a-mobile.mp4"));
        QVERIFY(MediaResolver::parseRedgifs("{\"error\":{}}").isEmpty());
        QCOMPARE(MediaResolver::parseStreamable("{\"files\":{\"mp4\":{\"url\":\"//cdn-cf-east.streamable.com/video/mp4/x.mp4?token=1\"}}}"),
                 QStringLiteral("https://cdn-cf-east.streamable.com/video/mp4/x.mp4?token=1"));
    }
};

QTEST_GUILESS_MAIN(TestMediaResolver)
#include "tst_mediaresolver.moc"
