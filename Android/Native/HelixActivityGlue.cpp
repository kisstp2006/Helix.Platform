#include <Helix/Core/Strings/String.h>
#include <Helix/Platform/Threading/Thread.h>
#include <Helix/Platform/Window/Android.h>

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>

// The native methods of org.helix.platform.HelixActivity, linked into the application's shared library by
// helix_add_android_app: the application's HelixMain runs on a thread of its own; the UI thread hands it the surface,
// focus, touches and keys.
namespace
{
    using namespace helix;
    using namespace helix::platform;

    struct Glue
    {
        jobject assets = nullptr;
        Array<String> args = Array<String>();
        Thread thread;
        bool running = false;
    };

    Glue& State()
    {
        static Glue g;
        return g;
    }

    void RunApplication(void*)
    {
        Glue& g = State();
        android::AttachThread();
        Array<char*> argv;
        for (String& a : g.args)
            argv.Push(const_cast<char*>(a.CStr()));
        argv.Push(nullptr);
        HelixMain(int(g.args.Size()), argv.Data());
        android::FinishActivity();
        android::DetachThread();
    }

    // The program name, then the intent's "args" split at spaces.
    void Split(StringView text, Array<String>& out)
    {
        out.Push(String("helix"));
        usize at = 0;
        while (at < text.Size())
        {
            while (at < text.Size() && text[at] == ' ')
                at++;
            usize end = at;
            while (end < text.Size() && text[end] != ' ')
                end++;
            if (end > at)
                out.Push(String(text.Substr(at, end - at)));
            at = end;
        }
    }

    Event TouchEvent(jint action, jint id, jfloat x, jfloat y)
    {
        Event e;
        // MotionEvent: ACTION_DOWN 0, UP 1, MOVE 2, CANCEL 3, POINTER_DOWN 5, POINTER_UP 6.
        e.type = action == 0 || action == 5 ? EventType::TouchBegin
               : action == 1 || action == 6 ? EventType::TouchEnd
               : action == 2                ? EventType::TouchMove
                                            : EventType::TouchCancel;
        e.touch = {i32(id), f32(x), f32(y)};
        return e;
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_org_helix_platform_HelixActivity_nativeCreate(JNIEnv* env, jobject activity, jobject assets, jstring args, jfloat density)
    {
        Glue& g = State();
        if (g.running)
            return;
        JavaVM* vm = nullptr;
        env->GetJavaVM(&vm);
        android::SetActivity(vm, activity);
        g.assets = env->NewGlobalRef(assets);
        android::SetAssetManager(AAssetManager_fromJava(env, g.assets));
        android::SetContentScale(f32(density));
        g.args.Clear();
        const char* text = args ? env->GetStringUTFChars(args, nullptr) : nullptr;
        Split(text ? StringView(text) : StringView(), g.args);
        if (text)
            env->ReleaseStringUTFChars(args, text);
        g.running = g.thread.Start({.function = &RunApplication, .user = nullptr, .name = "HelixMain"});
    }

    JNIEXPORT void JNICALL Java_org_helix_platform_HelixActivity_nativeSurface(JNIEnv* env, jobject, jobject surface)
    {
        ANativeWindow* next = surface ? ANativeWindow_fromSurface(env, surface) : nullptr;
        ANativeWindow* previous = android::GetNativeWindow();
        android::SetNativeWindow(next);
        if (previous && previous != next)
        {
            // The application's device lets the old surface go before the UI thread does.
            android::WaitForNativeWindowRelease(previous);
            ANativeWindow_release(previous);
        }
        else if (previous == next && next)
            ANativeWindow_release(next);
    }

    JNIEXPORT void JNICALL Java_org_helix_platform_HelixActivity_nativeFocus(JNIEnv*, jobject, jboolean focused)
    {
        android::SetFocused(focused == JNI_TRUE);
    }

    JNIEXPORT void JNICALL Java_org_helix_platform_HelixActivity_nativeTouch(JNIEnv*, jobject, jint action, jint id, jfloat x, jfloat y)
    {
        android::PushEvent(TouchEvent(action, id, x, y));
    }

    JNIEXPORT void JNICALL Java_org_helix_platform_HelixActivity_nativeKey(JNIEnv*, jobject, jboolean down, jint keyCode)
    {
        Event e;
        e.type = down == JNI_TRUE ? EventType::KeyDown : EventType::KeyUp;
        // KEYCODE_BACK is the escape key of the reference's Android player.
        e.key = {keyCode == 4 ? KeyCode::Escape : KeyCode::None, u32(keyCode)};
        android::PushEvent(e);
    }

    JNIEXPORT void JNICALL Java_org_helix_platform_HelixActivity_nativeDestroy(JNIEnv* env, jobject)
    {
        Glue& g = State();
        if (!g.running)
            return;
        Event quit;
        quit.type = EventType::Quit;
        android::PushEvent(quit);
        g.thread.Join();
        g.running = false;
        android::SetAssetManager(nullptr);
        android::SetActivity(nullptr, nullptr);
        env->DeleteGlobalRef(g.assets);
        g.assets = nullptr;
    }
}
