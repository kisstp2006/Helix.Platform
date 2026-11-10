#include <helix/platform/thread.h>
#include <helix/platform/time.h>

namespace helix::platform
{
    Semaphore::Semaphore(u32 initial) : count_(initial) {}

    void Semaphore::Release(u32 count)
    {
        ScopedLock lock(mutex_);
        count_ += count;
        if (count == 1)
            condition_.NotifyOne();
        else
            condition_.NotifyAll();
    }

    void Semaphore::Acquire()
    {
        ScopedLock lock(mutex_);
        while (count_ == 0)
            condition_.Wait(mutex_);
        count_--;
    }

    bool Semaphore::TryAcquire()
    {
        ScopedLock lock(mutex_);
        if (count_ == 0)
            return false;
        count_--;
        return true;
    }

    f64 Now() { return f64(Ticks()) / f64(TicksPerSecond()); }
}
