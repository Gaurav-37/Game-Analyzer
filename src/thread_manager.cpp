#include "thread_manager.h"
#include <algorithm>
#include <chrono>
#include <iostream>

// ThreadManager Implementation
ThreadManager::ThreadManager() 
    : defaultMaxThreads(8), maxTotalThreads(32), 
      taskTimeout(std::chrono::milliseconds(5000)),
      totalTasks(0), activeTasks(0), isShuttingDown(false) {
}

ThreadManager::~ThreadManager() {
    shutdown();
}

bool ThreadManager::initialize(int maxThreads, int maxTotalThreads) {
    defaultMaxThreads = maxThreads;
    this->maxTotalThreads = maxTotalThreads;
    
    // Create main thread pools
    createThreadPool("main", maxThreads / 2);
    createThreadPool("io", 2);
    createThreadPool("compute", maxThreads / 2);
    createThreadPool("capture", 1);
    
    mainPool = std::unique_ptr<ThreadPool>(getThreadPool("main"));
    ioPool = std::unique_ptr<ThreadPool>(getThreadPool("io"));
    computePool = std::unique_ptr<ThreadPool>(getThreadPool("compute"));
    capturePool = std::unique_ptr<ThreadPool>(getThreadPool("capture"));
    
    return true;
}

void ThreadManager::shutdown() {
    isShuttingDown = true;
    
    // Stop all thread pools
    for (auto& pool : threadPools) {
        pool.second->shouldStop = true;
        pool.second->condition.notify_all();
    }
    
    // Wait for all threads to finish
    for (auto& pool : threadPools) {
        for (auto& thread : pool.second->threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    
    // Clear all pools
    threadPools.clear();
    mainPool = nullptr;
    ioPool = nullptr;
    computePool = nullptr;
    capturePool = nullptr;
}

bool ThreadManager::createThreadPool(const std::string& name, int maxThreads) {
    if (threadPools.find(name) != threadPools.end()) {
        return false; // Pool already exists
    }
    
    auto pool = std::make_unique<ThreadPool>(name, maxThreads);
    
    // Create worker threads
    for (int i = 0; i < maxThreads; ++i) {
        pool->threads.emplace_back(&ThreadManager::workerThread, this, pool.get());
    }
    
    threadPools[name] = std::move(pool);
    return true;
}

bool ThreadManager::destroyThreadPool(const std::string& name) {
    auto it = threadPools.find(name);
    if (it == threadPools.end()) {
        return false;
    }
    
    // Stop the pool
    it->second->shouldStop = true;
    it->second->condition.notify_all();
    
    // Wait for threads to finish
    for (auto& thread : it->second->threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    threadPools.erase(it);
    return true;
}

ThreadManager::ThreadPool* ThreadManager::getThreadPool(const std::string& name) {
    auto it = threadPools.find(name);
    return (it != threadPools.end()) ? it->second.get() : nullptr;
}

std::future<void> ThreadManager::submitTask(std::function<void()> task, TaskPriority priority, const std::string& poolName) {
    if (isShuttingDown) {
        std::promise<void> promise;
        promise.set_exception(std::make_exception_ptr(std::runtime_error("ThreadManager is shutting down")));
        return promise.get_future();
    }
    
    ThreadPool* pool = getThreadPool(poolName);
    if (!pool) {
        pool = mainPool.get();
    }
    
    if (!pool) {
        std::promise<void> promise;
        promise.set_exception(std::make_exception_ptr(std::runtime_error("No thread pool available")));
        return promise.get_future();
    }
    
    Task taskObj(task, priority, "Task_" + std::to_string(totalTasks++));
    
    {
        std::lock_guard<std::mutex> lock(pool->queueMutex);
        pool->taskQueue.push(std::move(taskObj));
    }
    
    pool->condition.notify_one();
    activeTasks++;
    
    return taskObj.promise.get_future();
}

std::future<void> ThreadManager::submitTaskToPool(const std::string& poolName, std::function<void()> task, TaskPriority priority) {
    return submitTask(task, priority, poolName);
}

std::future<void> ThreadManager::submitIOTask(std::function<void()> task, TaskPriority priority) {
    return submitTask(task, priority, "io");
}

std::future<void> ThreadManager::submitComputeTask(std::function<void()> task, TaskPriority priority) {
    return submitTask(task, priority, "compute");
}

std::future<void> ThreadManager::submitCaptureTask(std::function<void()> task, TaskPriority priority) {
    return submitTask(task, priority, "capture");
}

void ThreadManager::waitForAllTasks(const std::string& poolName) {
    if (poolName.empty()) {
        // Wait for all pools
        for (auto& pool : threadPools) {
            waitForAllTasks(pool.first);
        }
    } else {
        ThreadPool* pool = getThreadPool(poolName);
        if (pool) {
            std::unique_lock<std::mutex> lock(pool->queueMutex);
            pool->condition.wait(lock, [pool] { return pool->taskQueue.empty(); });
        }
    }
}

void ThreadManager::waitForTask(std::future<void>& future, std::chrono::milliseconds timeout) {
    if (future.wait_for(timeout) == std::future_status::timeout) {
        throw std::runtime_error("Task timeout");
    }
}

void ThreadManager::pauseThreadPool(const std::string& name) {
    ThreadPool* pool = getThreadPool(name);
    if (pool) {
        pool->shouldStop = true;
    }
}

void ThreadManager::resumeThreadPool(const std::string& name) {
    ThreadPool* pool = getThreadPool(name);
    if (pool) {
        pool->shouldStop = false;
        pool->condition.notify_all();
    }
}

void ThreadManager::setThreadPoolSize(const std::string& name, int newSize) {
    ThreadPool* pool = getThreadPool(name);
    if (!pool) return;
    
    int currentSize = pool->threads.size();
    
    if (newSize > currentSize) {
        // Add more threads
        for (int i = currentSize; i < newSize; ++i) {
            pool->threads.emplace_back(&ThreadManager::workerThread, this, pool);
        }
    } else if (newSize < currentSize) {
        // Remove threads (they will stop when they finish current tasks)
        pool->shouldStop = true;
        pool->condition.notify_all();
        
        // Wait for excess threads to finish
        for (int i = newSize; i < currentSize; ++i) {
            if (pool->threads[i].joinable()) {
                pool->threads[i].join();
            }
        }
        
        pool->threads.resize(newSize);
        pool->shouldStop = false;
    }
}

ThreadManager::ThreadStatistics ThreadManager::getPoolStatistics(const std::string& poolName) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(statisticsMutex));
    auto it = poolStatistics.find(poolName);
    return (it != poolStatistics.end()) ? it->second : ThreadStatistics();
}

std::map<std::string, ThreadManager::ThreadStatistics> ThreadManager::getAllStatistics() const {
    std::lock_guard<std::mutex> lock(statisticsMutex);
    return poolStatistics;
}

double ThreadManager::getOverallEfficiency() const {
    if (totalTasks == 0) return 0.0;
    
    int totalCompleted = 0;
    for (const auto& stat : poolStatistics) {
        totalCompleted += stat.second.completedTasks;
    }
    
    return static_cast<double>(totalCompleted) / totalTasks;
}

void ThreadManager::workerThread(ThreadPool* pool) {
    while (!pool->shouldStop) {
        Task task;
        
        {
            std::unique_lock<std::mutex> lock(pool->queueMutex);
            pool->condition.wait(lock, [pool] { 
                return pool->shouldStop || !pool->taskQueue.empty(); 
            });
            
            if (pool->shouldStop) break;
            
            if (!pool->taskQueue.empty()) {
                task = std::move(pool->taskQueue.front());
                pool->taskQueue.pop();
            }
        }
        
        if (task.function) {
            auto startTime = std::chrono::high_resolution_clock::now();
            bool success = true;
            
            try {
                task.function();
            } catch (...) {
                success = false;
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            double executionTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            
            updateStatistics(pool->name, task.name, executionTime, success);
            
            task.promise.set_value();
            activeTasks--;
        }
    }
}

void ThreadManager::executeTask(Task& task, const std::string& poolName) {
    auto startTime = std::chrono::high_resolution_clock::now();
    bool success = true;
    
    try {
        task.function();
    } catch (...) {
        success = false;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double executionTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    updateStatistics(poolName, task.name, executionTime, success);
}

void ThreadManager::updateStatistics(const std::string& poolName, const std::string& taskName, 
                                    double executionTime, bool success) {
    std::lock_guard<std::mutex> lock(statisticsMutex);
    
    ThreadStatistics& stats = poolStatistics[poolName];
    stats.totalTasks++;
    
    if (success) {
        stats.completedTasks++;
    } else {
        stats.failedTasks++;
    }
    
    stats.totalExecutionTime += executionTime;
    stats.averageTaskTime = stats.totalExecutionTime / stats.totalTasks;
    stats.taskCounts[taskName]++;
}

bool ThreadManager::isValidPoolName(const std::string& name) const {
    return threadPools.find(name) != threadPools.end();
}

void ThreadManager::cleanupCompletedTasks() {
    // This would clean up completed task futures
    // Implementation depends on specific requirements
}

// SmartDialogManager Implementation

SmartDialogManager::~SmartDialogManager() {
    closeAllDialogs();
}

std::unique_ptr<class ModernDialog> SmartDialogManager::createDialog(const DialogInfo& info) {
    std::lock_guard<std::mutex> lock(dialogMutex);
    
    if (dialogFactory) {
        auto dialog = dialogFactory(info);
        if (dialog) {
            registerDialog(std::move(dialog));
            totalDialogsCreated++;
            activeDialogCount++;
        }
        return std::move(dialog);
    }
    
    return nullptr;
}

void SmartDialogManager::showDialog(const DialogInfo& info) {
    auto dialog = createDialog(info);
    if (dialog) {
        dialog->show();
    }
}

void SmartDialogManager::closeAllDialogs() {
    std::lock_guard<std::mutex> lock(dialogMutex);
    
    for (auto& dialog : activeDialogs) {
        if (dialog) {
            dialog->hide();
        }
    }
    
    activeDialogs.clear();
    activeDialogCount = 0;
}

void SmartDialogManager::closeDialog(const std::string& title) {
    std::lock_guard<std::mutex> lock(dialogMutex);
    
    activeDialogs.erase(
        std::remove_if(activeDialogs.begin(), activeDialogs.end(),
            [&title](const std::unique_ptr<class ModernDialog>& dialog) {
                return dialog && dialog->title == title;
            }),
        activeDialogs.end()
    );
    
    activeDialogCount = activeDialogs.size();
}

void SmartDialogManager::registerDialog(std::unique_ptr<class ModernDialog> dialog) {
    std::lock_guard<std::mutex> lock(dialogMutex);
    activeDialogs.push_back(std::move(dialog));
}

void SmartDialogManager::unregisterDialog(class ModernDialog* dialog) {
    std::lock_guard<std::mutex> lock(dialogMutex);
    
    activeDialogs.erase(
        std::remove_if(activeDialogs.begin(), activeDialogs.end(),
            [dialog](const std::unique_ptr<class ModernDialog>& d) {
                return d.get() == dialog;
            }),
        activeDialogs.end()
    );
    
    activeDialogCount = activeDialogs.size();
}

void SmartDialogManager::setDialogFactory(std::function<std::unique_ptr<class ModernDialog>(const DialogInfo&)> factory) {
    dialogFactory = factory;
}

void SmartDialogManager::cleanupDialogs() {
    std::lock_guard<std::mutex> lock(dialogMutex);
    
    // Remove null dialogs
    activeDialogs.erase(
        std::remove_if(activeDialogs.begin(), activeDialogs.end(),
            [](const std::unique_ptr<class ModernDialog>& dialog) {
                return !dialog;
            }),
        activeDialogs.end()
    );
    
    activeDialogCount = activeDialogs.size();
}

bool SmartDialogManager::isDialogActive(const std::string& title) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(dialogMutex));
    
    return std::any_of(activeDialogs.begin(), activeDialogs.end(),
        [&title](const std::unique_ptr<class ModernDialog>& dialog) {
            return dialog && dialog->title == title;
        });
}
