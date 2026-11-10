#include <helix/platform/thread.h>
#include <helix/platform/time.h>

namespace helix::platform
{
    Semaphore::Semaphore(u32 initial) : count_(i32(initial)) {}

    void Semaphore::Release(u32 count)
    {
        count_.fetch_add(i32(count), std::memory_order_seq_cst);
        if (waiters_.load(std::memory_order_seq_cst) == 0)
            return;
        if (count == 1)
            count_.notify_one();
        else
            count_.notify_all();
    }

    bool Semaphore::TryAcquire()
    {
        i32 c = count_.load(std::memory_order_relaxed);
        while (c > 0)
            if (count_.compare_exchange_weak(c, c - 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        return false;
    }

    void Semaphore::Acquire()
    {
        for (int spin = 0; spin < 64; spin++)
        {
            if (TryAcquire())
                return;
            SpinPause();
        }
        for (;;)
        {
            if (TryAcquire())
                return;
            waiters_.fetch_add(1, std::memory_order_seq_cst);
            if (count_.load(std::memory_order_seq_cst) <= 0)
                count_.wait(0, std::memory_order_acquire);
            waiters_.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    f64 Now() { return f64(Ticks()) / f64(TicksPerSecond()); }
}
