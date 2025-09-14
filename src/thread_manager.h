#pragma once

#include <windows.h>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>
#include <future>
#include <chrono>

// Advanced Thread Manager with Thread Pools
class ThreadManager {
public:
    enum class TaskPriority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    };

    struct Task {
        std::function<void()> function;
        TaskPriority priority;
        std::string name;
        int64_t timestamp;
        std::promise<void> promise;
        
        Task() : priority(TaskPriority::NORMAL), timestamp(0) {}
        Task(std::function<void()> func, TaskPriority prio = TaskPriority::NORMAL, const std::string& taskName = "")
            : function(func), priority(prio), name(taskName) {
            timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }
    };

    struct ThreadPool {
        std::vector<std::thread> threads;
        std::queue<Task> taskQueue;
        std::mutex queueMutex;
        std::condition_variable condition;
        std::atomic<bool> shouldStop;
        std::string name;
        int maxThreads;
        
        ThreadPool(const std::string& poolName, int maxThreads = 4) 
            : shouldStop(false), name(poolName), maxThreads(maxThreads) {}
    };

    struct ThreadStatistics {
        int totalTasks;
        int completedTasks;
        int failedTasks;
        double averageTaskTime;
        double totalExecutionTime;
        std::map<std::string, int> taskCounts;
        
        ThreadStatistics() : totalTasks(0), completedTasks(0), failedTasks(0), 
                           averageTaskTime(0.0), totalExecutionTime(0.0) {}
    };

private:
    // Thread pools
    std::map<std::string, std::unique_ptr<ThreadPool>> threadPools;
    
    // Main thread pool
    std::unique_ptr<ThreadPool> mainPool;
    std::unique_ptr<ThreadPool> ioPool;
    std::unique_ptr<ThreadPool> computePool;
    std::unique_ptr<ThreadPool> capturePool;
    
    // Task management
    std::atomic<int> totalTasks;
    std::atomic<int> activeTasks;
    std::atomic<bool> isShuttingDown;
    
    // Statistics
    std::map<std::string, ThreadStatistics> poolStatistics;
    std::mutex statisticsMutex;
    
    // Configuration
    int defaultMaxThreads;
    int maxTotalThreads;
    std::chrono::milliseconds taskTimeout;
    
public:
    ThreadManager();
    ~ThreadManager();
    
    // Initialization
    bool initialize(int maxThreads = 8, int maxTotalThreads = 32);
    void shutdown();
    
    // Thread pool management
    bool createThreadPool(const std::string& name, int maxThreads = 4);
    bool destroyThreadPool(const std::string& name);
    ThreadPool* getThreadPool(const std::string& name);
    
    // Task submission
    std::future<void> submitTask(std::function<void()> task, TaskPriority priority = TaskPriority::NORMAL, 
                                const std::string& poolName = "main");
    std::future<void> submitTaskToPool(const std::string& poolName, std::function<void()> task, 
                                      TaskPriority priority = TaskPriority::NORMAL);
    
    // Specialized task submission
    std::future<void> submitIOTask(std::function<void()> task, TaskPriority priority = TaskPriority::NORMAL);
    std::future<void> submitComputeTask(std::function<void()> task, TaskPriority priority = TaskPriority::NORMAL);
    std::future<void> submitCaptureTask(std::function<void()> task, TaskPriority priority = TaskPriority::HIGH);
    
    // Task management
    void waitForAllTasks(const std::string& poolName = "");
    void waitForTask(std::future<void>& future, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    bool cancelTask(const std::string& taskName);
    
    // Thread pool control
    void pauseThreadPool(const std::string& name);
    void resumeThreadPool(const std::string& name);
    void setThreadPoolSize(const std::string& name, int newSize);
    
    // Statistics
    ThreadStatistics getPoolStatistics(const std::string& poolName) const;
    std::map<std::string, ThreadStatistics> getAllStatistics() const;
    int getTotalTasks() const { return totalTasks.load(); }
    int getActiveTasks() const { return activeTasks.load(); }
    double getOverallEfficiency() const;
    
    // Configuration
    void setDefaultMaxThreads(int threads) { defaultMaxThreads = threads; }
    void setMaxTotalThreads(int threads) { maxTotalThreads = threads; }
    void setTaskTimeout(std::chrono::milliseconds timeout) { taskTimeout = timeout; }
    
private:
    // Thread pool worker
    void workerThread(ThreadPool* pool);
    
    // Task execution
    void executeTask(Task& task, const std::string& poolName);
    
    // Statistics update
    void updateStatistics(const std::string& poolName, const std::string& taskName, 
                         double executionTime, bool success);
    
    // Utility functions
    bool isValidPoolName(const std::string& name) const;
    void cleanupCompletedTasks();
};

// Smart Pointer-based Dialog Manager
class SmartDialogManager {
public:
    struct DialogInfo {
        std::string title;
        std::string content;
        int width;
        int height;
        bool hasCancel;
        std::function<void()> onClose;
        
        DialogInfo() : width(500), height(300), hasCancel(false) {}
    };

private:
    // Smart pointer management
    std::vector<std::unique_ptr<class ModernDialog>> activeDialogs;
    std::mutex dialogMutex;
    
    // Dialog factory
    std::function<std::unique_ptr<class ModernDialog>(const DialogInfo&)> dialogFactory;
    
    // Statistics
    std::atomic<int> totalDialogsCreated;
    std::atomic<int> activeDialogCount;
    
public:
    SmartDialogManager();
    ~SmartDialogManager();
    
    // Dialog management
    std::unique_ptr<class ModernDialog> createDialog(const DialogInfo& info);
    void showDialog(const DialogInfo& info);
    void closeAllDialogs();
    void closeDialog(const std::string& title);
    
    // Smart pointer utilities
    void registerDialog(std::unique_ptr<class ModernDialog> dialog);
    void unregisterDialog(class ModernDialog* dialog);
    
    // Statistics
    int getTotalDialogsCreated() const { return totalDialogsCreated.load(); }
    int getActiveDialogCount() const { return activeDialogCount.load(); }
    
    // Factory pattern
    void setDialogFactory(std::function<std::unique_ptr<class ModernDialog>(const DialogInfo&)> factory);
    
private:
    void cleanupDialogs();
    bool isDialogActive(const std::string& title) const;
};

// RAII-based Resource Manager
template<typename T>
class RAIIResource {
private:
    T* resource;
    std::function<void(T*)> deleter;
    
public:
    RAIIResource(T* res, std::function<void(T*)> del) : resource(res), deleter(del) {}
    
    ~RAIIResource() {
        if (resource && deleter) {
            deleter(resource);
        }
    }
    
    T* get() const { return resource; }
    T* operator->() const { return resource; }
    T& operator*() const { return *resource; }
    
    // Disable copy
    RAIIResource(const RAIIResource&) = delete;
    RAIIResource& operator=(const RAIIResource&) = delete;
    
    // Enable move
    RAIIResource(RAIIResource&& other) noexcept 
        : resource(other.resource), deleter(other.deleter) {
        other.resource = nullptr;
        other.deleter = nullptr;
    }
    
    RAIIResource& operator=(RAIIResource&& other) noexcept {
        if (this != &other) {
            if (resource && deleter) {
                deleter(resource);
            }
            resource = other.resource;
            deleter = other.deleter;
            other.resource = nullptr;
            other.deleter = nullptr;
        }
        return *this;
    }
};

// Memory-safe Dialog Wrapper
class SafeDialog {
private:
    std::unique_ptr<class ModernDialog> dialog;
    std::string title;
    bool isShown;
    
public:
    SafeDialog(std::unique_ptr<class ModernDialog> dlg, const std::string& ttl)
        : dialog(std::move(dlg)), title(ttl), isShown(false) {}
    
    ~SafeDialog() {
        if (dialog && isShown) {
            dialog->hide();
        }
    }
    
    void show() {
        if (dialog) {
            dialog->show();
            isShown = true;
        }
    }
    
    void hide() {
        if (dialog && isShown) {
            dialog->hide();
            isShown = false;
        }
    }
    
    bool isVisible() const { return isShown; }
    const std::string& getTitle() const { return title; }
    
    // Disable copy
    SafeDialog(const SafeDialog&) = delete;
    SafeDialog& operator=(const SafeDialog&) = delete;
    
    // Enable move
    SafeDialog(SafeDialog&& other) noexcept 
        : dialog(std::move(other.dialog)), title(std::move(other.title)), isShown(other.isShown) {
        other.isShown = false;
    }
    
    SafeDialog& operator=(SafeDialog&& other) noexcept {
        if (this != &other) {
            if (dialog && isShown) {
                dialog->hide();
            }
            dialog = std::move(other.dialog);
            title = std::move(other.title);
            isShown = other.isShown;
            other.isShown = false;
        }
        return *this;
    }
};
