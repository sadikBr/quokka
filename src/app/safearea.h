#pragma once

#include <QObject>
#include <QQmlEngine>

// The parts of the screen covered by system bars (status bar, navigation / gesture bar,
// camera cut-outs) that the app is drawn under, in device-independent pixels. QML: `SafeArea`.
// Android 15+ draws apps edge-to-edge; elsewhere these are 0.
class SafeArea : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(qreal top READ top NOTIFY changed)
    Q_PROPERTY(qreal bottom READ bottom NOTIFY changed)
    Q_PROPERTY(qreal left READ left NOTIFY changed)
    Q_PROPERTY(qreal right READ right NOTIFY changed)
    // Dark status / navigation bar icons (for light backgrounds).
    Q_PROPERTY(bool lightBars READ lightBars WRITE setLightBars NOTIFY lightBarsChanged)

public:
    explicit SafeArea(QObject *parent = nullptr);

    qreal top() const { return m_top; }
    qreal bottom() const { return m_bottom; }
    qreal left() const { return m_left; }
    qreal right() const { return m_right; }
    bool lightBars() const { return m_lightBars; }
    void setLightBars(bool light);

    // Ask the system again (rotation, window resize, returning to the app).
    Q_INVOKABLE void refresh();

signals:
    void changed();
    void lightBarsChanged();

private:
    void apply(qreal top, qreal bottom, qreal left, qreal right);
    void applyBarAppearance();

    qreal m_top = 0;
    qreal m_bottom = 0;
    qreal m_left = 0;
    qreal m_right = 0;
    bool m_lightBars = false;
};
