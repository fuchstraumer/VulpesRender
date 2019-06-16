#pragma once
#ifndef VPR_ALLOC_2_MUTEX_HPP
#define VPR_ALLOC_2_MUTEX_HPP
#include <shared_mutex>

namespace vpr {

    /*
    * Alias over C++17's shared_mutex, a mutex that allows us to have better read/write locking 
    * semantics. We can now lock for just reading, guarding from writers, and in turn can block
    * readers until the writer has completed.
    * \ingroup Allocation
    */
    struct rw_mutex {

        void lock_read() noexcept {
            mutex.lock_shared();
        }

        void unlock_read() noexcept {
            mutex.unlock_shared();
        }

        void lock_write() noexcept {
            mutex.lock();
        }

        void unlock_write() noexcept {
            mutex.unlock();
        }

    private:
        std::shared_mutex mutex;
    };

    enum class lock_mode : uint8_t {
        Invalid = 0,
        Read = 1,
        Write = 2
    };

    /*
        \brief A variant of lock_guard for shared_mutex, that allows for different locking semantics based on the current needs. 
        This is used in place of the mutex itself where possible, at it can greatly reduce the chance of mis-using mutexes in our code.
    */
    struct rw_lock_guard {

        rw_lock_guard(lock_mode _mode, rw_mutex& _mut) noexcept : mut(_mut), mode(_mode) {
            if (mode == lock_mode::Read) {
                mut.lock_read();
            }
            else {
                mut.lock_write();
            }
        }

        ~rw_lock_guard() noexcept {
            if (mode == lock_mode::Read) {
                mut.unlock_read();
            }
            else if (mode == lock_mode::Write) {
                mut.unlock_write();
            }
        }

        rw_lock_guard(rw_lock_guard&& other) noexcept : mut(std::move(other.mut)), mode(std::move(other.mode)) {}
        rw_lock_guard& operator=(rw_lock_guard&& other)  = delete;
        rw_lock_guard(const rw_lock_guard&) = delete;
        rw_lock_guard& operator=(const rw_lock_guard&) = delete;

    private:
        lock_mode mode{ lock_mode::Invalid };
        rw_mutex& mut;
    };

}

#endif //!VPR_ALLOC_2_MUTEX_HPP