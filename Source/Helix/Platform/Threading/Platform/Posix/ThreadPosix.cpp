#include <Helix/Platform/Threading/Thread.h>
#include <Helix/Platform/Time/Time.h>
#include <Helix/Core/Basics/Assert.h>

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

#if HX_PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/threading.h>
#endif

#if HX_PLATFORM_LINUX || HX_PLATFORM_ANDROID
#include <sys/syscall.h>
#endif

namespace helix::platform
{
    static_assert(sizeof(pthread_mutex_t) <= 64 && sizeof(pthread_cond_t) <= 64);

    namespace
    {
        void* ThreadEntry(void* param)
        {
            auto* desc = static_cast<ThreadDesc*>(param);
            SetCurrentThreadName(desc->name);
            desc->function(desc->user);
            return nullptr;
        }

        timespec Deadline(f64 seconds)
        {
            timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            i64 ns = i64(seconds * 1e9) + ts.tv_nsec;
            ts.tv_sec += time_t(ns / 1000000000);
            ts.tv_nsec = long(ns % 1000000000);
            return ts;
        }
    }

    Thread::~Thread() { HX_ASSERT_MSG(!Joinable(), "thread destroyed without Join"); }

    bool Thread::Start(const ThreadDesc& desc)
    {
        HX_ASSERT(!Joinable() && desc.function);
        if (!ThreadsSupported())
            return false;
        desc_ = desc;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        if (desc.stackSize)
            pthread_attr_setstacksize(&attr, desc.stackSize);
        pthread_t t;
        int rc = pthread_create(&t, &attr, &ThreadEntry, &desc_);
        pthread_attr_destroy(&attr);
        if (rc != 0)
            return false;
        handle_ = uptr(t);
        return true;
    }

    void Thread::Join()
    {
        if (!handle_)
            return;
        pthread_join(pthread_t(handle_), nullptr);
        handle_ = 0;
    }

    bool ThreadsSupported()
    {
#if HX_PLATFORM_WEB
#if defined(__EMSCRIPTEN_PTHREADS__)
        return emscripten_has_threading_support();
#else
        return false;
#endif
#else
        return true;
#endif
    }

    u64 CurrentThreadId()
    {
#if HX_PLATFORM_LINUX || HX_PLATFORM_ANDROID
        return u64(syscall(SYS_gettid));
#else
        return u64(uptr(pthread_self()));
#endif
    }

    void SetCurrentThreadName(const char* name)
    {
#if HX_PLATFORM_LINUX || HX_PLATFORM_ANDROID
        char shortName[16];
        usize i = 0;
        for (; name[i] && i < 15; i++)
            shortName[i] = name[i];
        shortName[i] = '\0';
        pthread_setname_np(pthread_self(), shortName);
#else
        (void)name;
#endif
    }

    u32 LogicalCoreCount()
    {
#if HX_PLATFORM_WEB
        return ThreadsSupported() ? u32(emscripten_num_logical_cores()) : 1u;
#else
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        return n > 0 ? u32(n) : 1u;
#endif
    }

    void YieldThread() { sched_yield(); }

    Mutex::Mutex() { pthread_mutex_init(reinterpret_cast<pthread_mutex_t*>(storage_), nullptr); }
    Mutex::~Mutex() { pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(storage_)); }
    void Mutex::Lock() { pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(storage_)); }
    bool Mutex::TryLock() { return pthread_mutex_trylock(reinterpret_cast<pthread_mutex_t*>(storage_)) == 0; }
    void Mutex::Unlock() { pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(storage_)); }

    ConditionVariable::ConditionVariable() { pthread_cond_init(reinterpret_cast<pthread_cond_t*>(storage_), nullptr); }
    ConditionVariable::~ConditionVariable() { pthread_cond_destroy(reinterpret_cast<pthread_cond_t*>(storage_)); }

    void ConditionVariable::Wait(Mutex& mutex)
    {
        pthread_cond_wait(reinterpret_cast<pthread_cond_t*>(storage_), static_cast<pthread_mutex_t*>(mutex.Native()));
    }

    bool ConditionVariable::WaitFor(Mutex& mutex, f64 seconds)
    {
        timespec ts = Deadline(seconds);
        return pthread_cond_timedwait(reinterpret_cast<pthread_cond_t*>(storage_), static_cast<pthread_mutex_t*>(mutex.Native()), &ts) == 0;
    }

    void ConditionVariable::NotifyOne() { pthread_cond_signal(reinterpret_cast<pthread_cond_t*>(storage_)); }
    void ConditionVariable::NotifyAll() { pthread_cond_broadcast(reinterpret_cast<pthread_cond_t*>(storage_)); }
}
