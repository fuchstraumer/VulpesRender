#include "vpr_stdafx.h"
#include "util/TaskPool.hpp"

namespace vulpes {

    namespace util {

        TaskPool::TaskPool() : Complete(false) {
            worker = std::thread(&TaskPool::queueLoop, this);
        }

        TaskPool::~TaskPool() {
            if(worker.joinable()){
                Wait();
                queueMutex.lock();
                Complete = true;
                condVar.notify_one();
                queueMutex.unlock();
                worker.join();
            }
        }

        void TaskPool::Run() {
            worker = std::thread(&TaskPool::queueLoop, this);
        }

        void TaskPool::Wait() {
            std::unique_lock<std::mutex> wait_lock(queueMutex);
            condVar.wait(wait_lock, [this]() { return taskQueue.empty(); });
        }

        size_t TaskPool::NumTasks() const noexcept {
            return taskQueue.size();
        }

        void TaskPool::queueLoop() {
            // do-while so this executes at least once (thread enters state to wait for a task)
            do {
                std::packaged_task<void()> curr_task;

                {
                    std::unique_lock<std::mutex> wait_for_task_lock;
                    condVar.wait(wait_for_task_lock, [this]() { return !taskQueue.empty() || Complete; });
                    if(Complete) {
                        break;
                    }
                    curr_task = std::move(taskQueue.front());
                }

                // run task
                curr_task();

                taskQueue.pop_front();
                condVar.notify_one();

            } while(!taskQueue.empty());

        }
    }

}