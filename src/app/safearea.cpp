#include "safearea.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTimer>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

SafeArea::SafeArea(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_ANDROID
    // Insets are only known once the window is attached: ask now, and again shortly after.
    for (int delay : {0, 300, 1200})
        QTimer::singleShot(delay, this, &SafeArea::refresh);
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState s) {
        if (s == Qt::ApplicationActive)
            refresh();
    });
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        connect(screen, &QScreen::orientationChanged, this, [this] {
            QTimer::singleShot(250, this, &SafeArea::refresh);
        });
        connect(screen, &QScreen::geometryChanged, this, [this] {
            QTimer::singleShot(250, this, &SafeArea::refresh);
        });
    }
#endif
}

void SafeArea::apply(qreal top, qreal bottom, qreal left, qreal right)
{
    if (qFuzzyCompare(top + 1, m_top + 1) && qFuzzyCompare(bottom + 1, m_bottom + 1)
        && qFuzzyCompare(left + 1, m_left + 1) && qFuzzyCompare(right + 1, m_right + 1))
        return;
    m_top = top;
    m_bottom = bottom;
    m_left = left;
    m_right = right;
    emit changed();
}

void SafeArea::setLightBars(bool light)
{
    if (light == m_lightBars)
        return;
    m_lightBars = light;
    emit lightBarsChanged();
    applyBarAppearance();
}

#ifdef Q_OS_ANDROID
namespace {

QJniObject activityWindow()
{
    QJniObject activity(QNativeInterface::QAndroidApplication::context());
    if (!activity.isValid())
        return {};
    return activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
}

// x, y of a view inside its window.
QPoint locationInWindow(const QJniObject &view)
{
    QJniEnvironment env;
    jintArray arr = env->NewIntArray(2);
    view.callMethod<void>("getLocationInWindow", "([I)V", arr);
    jint xy[2] = {0, 0};
    env->GetIntArrayRegion(arr, 0, 2, xy);
    env->DeleteLocalRef(arr);
    return {xy[0], xy[1]};
}

} // namespace
#endif

void SafeArea::refresh()
{
#ifdef Q_OS_ANDROID
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([this] {
        const QJniObject window = activityWindow();
        if (!window.isValid())
            return;
        const QJniObject decor = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
        const QJniObject insets = decor.callObjectMethod("getRootWindowInsets", "()Landroid/view/WindowInsets;");
        if (!insets.isValid())
            return;

        int top = 0, bottom = 0, left = 0, right = 0;
        if (QNativeInterface::QAndroidApplication::sdkVersion() >= 30) {
            const jint types = QJniObject::callStaticMethod<jint>("android/view/WindowInsets$Type", "systemBars", "()I")
                | QJniObject::callStaticMethod<jint>("android/view/WindowInsets$Type", "displayCutout", "()I");
            const QJniObject in = insets.callObjectMethod("getInsets", "(I)Landroid/graphics/Insets;", types);
            if (in.isValid()) {
                top = in.getField<jint>("top");
                bottom = in.getField<jint>("bottom");
                left = in.getField<jint>("left");
                right = in.getField<jint>("right");
            }
        } else {
            top = insets.callMethod<jint>("getSystemWindowInsetTop");
            bottom = insets.callMethod<jint>("getSystemWindowInsetBottom");
            left = insets.callMethod<jint>("getSystemWindowInsetLeft");
            right = insets.callMethod<jint>("getSystemWindowInsetRight");
        }

        // Only the part the app is actually drawn under counts: where the system already keeps
        // the app clear of a bar (older Android versions), the content view starts after it.
        const QJniObject content = decor.callObjectMethod("findViewById", "(I)Landroid/view/View;", jint(0x01020002)); // android.R.id.content
        if (content.isValid()) {
            const QPoint at = locationInWindow(content);
            const int w = content.callMethod<jint>("getWidth");
            const int h = content.callMethod<jint>("getHeight");
            const int decorW = decor.callMethod<jint>("getWidth");
            const int decorH = decor.callMethod<jint>("getHeight");
            if (w > 0 && h > 0) {
                top = qMax(0, top - at.y());
                left = qMax(0, left - at.x());
                bottom = qMax(0, bottom - (decorH - (at.y() + h)));
                right = qMax(0, right - (decorW - (at.x() + w)));
            }
        }

        QMetaObject::invokeMethod(this, [this, top, bottom, left, right] {
            const QScreen *screen = QGuiApplication::primaryScreen();
            const qreal dpr = screen ? screen->devicePixelRatio() : 1.0;
            apply(top / dpr, bottom / dpr, left / dpr, right / dpr);
        }, Qt::QueuedConnection);
    });
#endif
}

void SafeArea::applyBarAppearance()
{
#ifdef Q_OS_ANDROID
    const bool light = m_lightBars;
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([light] {
        const QJniObject window = activityWindow();
        if (!window.isValid())
            return;
        if (QNativeInterface::QAndroidApplication::sdkVersion() >= 30) {
            const QJniObject controller =
                window.callObjectMethod("getInsetsController", "()Landroid/view/WindowInsetsController;");
            const jint mask = 0x8 | 0x10; // APPEARANCE_LIGHT_STATUS_BARS | APPEARANCE_LIGHT_NAVIGATION_BARS
            if (controller.isValid())
                controller.callMethod<void>("setSystemBarsAppearance", "(II)V", light ? mask : jint(0), mask);
        } else {
            QJniObject decor = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
            jint flags = decor.callMethod<jint>("getSystemUiVisibility");
            const jint lightFlags = 0x2000 | 0x10; // SYSTEM_UI_FLAG_LIGHT_STATUS_BAR | _LIGHT_NAVIGATION_BAR
            flags = light ? (flags | lightFlags) : (flags & ~lightFlags);
            decor.callMethod<void>("setSystemUiVisibility", "(I)V", flags);
        }
    });
#endif
}
