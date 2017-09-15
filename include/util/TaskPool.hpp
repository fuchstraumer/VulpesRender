#pragma once
#ifndef VULPES_VK_TASK_POOL_HPP
#define VULPES_VK_TASK_POOL_HPP

#include "vpr_stdafx.h"

namespace vulpes {

    namespace util {

        class TaskPool {
            TaskPool(const TaskPool&) = delete;
            TaskPool& operator=(const TaskPool&) = delete;
        public:

            using task_t = std::packaged_task<void()>;

            TaskPool();
            ~TaskPool();

            template<class Fn, class...Args>
            std::future<typename std::result_of<Fn(Args...)>::type> AddTask(Fn&& f, Args&&... args);

            void Run();
            void Wait();
            size_t NumTasks() const noexcept;

            bool Complete;

        private:

            void queueLoop();

            std::deque<task_t> taskQueue;
            std::thread worker;
            std::mutex queueMutex;
            std::condition_variable condVar;
        };

        template<class Fn, class...Args>
        std::future<typename std::result_of<Fn(Args...)>::type> TaskPool::AddTask(Fn&& f, Args&&... args) {
            std::packaged_task<typename std::result_of<Fn(Args...)>::type()> task(std::bind(f, args...));
            std::future<typename std::result_of<Fn(Args...)>::type> result_future(std::move(task.get_future()));
            std::lock_guard<std::mutex> queue_lock(queueMutex);
            taskQueue.push_back(std::packaged_task<void()>(std::move(task)));
            condVar.notify_one();
            return std::move(result_future);
        }
    
    }

}

#endif //!VULPES_VK_TASK_POOL_HPP