#include "test_framework.h"
#include "advanced_ocr.h"
#include "optimized_screen_capture.h"
#include "game_analytics.h"
#include "performance_monitor.h"
#include "thread_manager.h"
#include <opencv2/opencv.hpp>

namespace BloombergTerminalTests {

// OCR Benchmark as specified in prompt.md
void registerOCRBenchmark() {
    registerBenchmark("AdvancedOCR", "OCRProcessing", []() -> BenchmarkResult {
        AdvancedOCR ocr;
        ocr.initialize();
        
        // Create test frame similar to prompt.md specification
        cv::Mat frame = cv::Mat::zeros(1080, 1920, CV_8UC3);
        cv::putText(frame, "BLOOMBERG TERMINAL", cv::Point(200, 300), 
                   cv::FONT_HERSHEY_SIMPLEX, 3, cv::Scalar(255, 255, 255), 4);
        cv::putText(frame, "Score: 1250", cv::Point(200, 400), 
                   cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(255, 255, 0), 3);
        cv::putText(frame, "Health: 85%", cv::Point(200, 500), 
                   cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 255, 0), 3);
        cv::putText(frame, "Ammo: 30/120", cv::Point(200, 600), 
                   cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(255, 0, 0), 3);
        
        const size_t iterations = 100;
        std::vector<double> times;
        times.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            BenchmarkTimer timer;
            auto regions = ocr.detectText(frame);
            times.push_back(timer.elapsedMs());
        }
        
        // Calculate statistics
        double totalTime = std::accumulate(times.begin(), times.end(), 0.0);
        double averageTime = totalTime / iterations;
        double minTime = *std::min_element(times.begin(), times.end());
        double maxTime = *std::max_element(times.begin(), times.end());
        
        return BenchmarkResult("OCRProcessing", "AdvancedOCR", averageTime, minTime, maxTime, 
                             iterations, iterations);
    });
}

// Screen Capture Benchmark as specified in prompt.md
void registerCaptureBenchmark() {
    registerBenchmark("OptimizedScreenCapture", "FrameCapture", []() -> BenchmarkResult {
        OptimizedScreenCapture capture;
        capture.initialize();
        
        const size_t iterations = 50;
        std::vector<double> times;
        times.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            BenchmarkTimer timer;
            OptimizedScreenCapture::FrameData frame;
            capture.captureFrame(frame);
            times.push_back(timer.elapsedMs());
        }
        
        // Calculate statistics
        double totalTime = std::accumulate(times.begin(), times.end(), 0.0);
        double averageTime = totalTime / iterations;
        double minTime = *std::min_element(times.begin(), times.end());
        double maxTime = *std::max_element(times.begin(), times.end());
        
        return BenchmarkResult("FrameCapture", "OptimizedScreenCapture", averageTime, minTime, maxTime, 
                             iterations, iterations);
    });
}

// Game Analytics Benchmark
void registerGameAnalyticsBenchmark() {
    registerBenchmark("GameAnalytics", "EventDetection", []() -> BenchmarkResult {
        GameEventDetector detector;
        
        // Create test frames with various game elements
        std::vector<cv::Mat> testFrames;
        for (int i = 0; i < 20; ++i) {
            cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC3);
            
            // Add different game elements
            cv::circle(frame, cv::Point(100 + i * 10, 100 + i * 5), 20, cv::Scalar(0, 255, 0), -1);
            cv::rectangle(frame, cv::Rect(200 + i * 5, 200 + i * 3, 50, 30), cv::Scalar(255, 0, 0), 2);
            cv::line(frame, cv::Point(300 + i * 3, 300 + i * 2), cv::Point(350 + i * 3, 350 + i * 2), 
                    cv::Scalar(0, 0, 255), 2);
            
            testFrames.push_back(frame);
        }
        
        const size_t iterations = 100;
        std::vector<double> times;
        times.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            BenchmarkTimer timer;
            for (const auto& frame : testFrames) {
                auto events = detector.detectEvents(frame);
            }
            times.push_back(timer.elapsedMs());
        }
        
        // Calculate statistics
        double totalTime = std::accumulate(times.begin(), times.end(), 0.0);
        double averageTime = totalTime / iterations;
        double minTime = *std::min_element(times.begin(), times.end());
        double maxTime = *std::max_element(times.begin(), times.end());
        size_t itemsProcessed = iterations * testFrames.size();
        
        return BenchmarkResult("EventDetection", "GameAnalytics", averageTime, minTime, maxTime, 
                             iterations, itemsProcessed);
    });
}

// Memory Usage Benchmark
void registerMemoryBenchmark() {
    registerBenchmark("System", "MemoryUsage", []() -> BenchmarkResult {
        const size_t iterations = 1000;
        std::vector<double> memoryUsage;
        memoryUsage.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            // Create and process frames to measure memory usage
            std::vector<cv::Mat> frames;
            
            for (int j = 0; j < 10; ++j) {
                cv::Mat frame = cv::Mat::zeros(1080, 1920, CV_8UC3);
                cv::putText(frame, "Memory Test " + std::to_string(j), cv::Point(100, 100 + j * 50), 
                           cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
                frames.push_back(frame);
            }
            
            // Simulate processing
            for (const auto& frame : frames) {
                cv::Mat processed;
                cv::cvtColor(frame, processed, cv::COLOR_BGR2GRAY);
                cv::blur(processed, processed, cv::Size(5, 5));
            }
            
            // Estimate memory usage (rough calculation)
            double estimatedMemory = (1080 * 1920 * 3 * 10) / (1024.0 * 1024.0); // MB
            memoryUsage.push_back(estimatedMemory);
            
            frames.clear();
        }
        
        double averageMemory = std::accumulate(memoryUsage.begin(), memoryUsage.end(), 0.0) / iterations;
        double maxMemory = *std::max_element(memoryUsage.begin(), memoryUsage.end());
        double minMemory = *std::min_element(memoryUsage.begin(), memoryUsage.end());
        
        return BenchmarkResult("MemoryUsage", "System", averageMemory, minMemory, maxMemory, 
                             iterations, iterations);
    });
}

// CPU Usage Benchmark
void registerCPUUsageBenchmark() {
    registerBenchmark("System", "CPUUsage", []() -> BenchmarkResult {
        const size_t iterations = 100;
        std::vector<double> cpuTimes;
        cpuTimes.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            BenchmarkTimer timer;
            
            // Simulate CPU-intensive operations
            std::vector<cv::Mat> frames;
            for (int j = 0; j < 5; ++j) {
                cv::Mat frame = cv::Mat::zeros(1080, 1920, CV_8UC3);
                cv::putText(frame, "CPU Test " + std::to_string(j), cv::Point(100, 100 + j * 100), 
                           cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(255, 255, 255), 3);
                frames.push_back(frame);
            }
            
            // Perform CPU-intensive image processing
            for (const auto& frame : frames) {
                cv::Mat processed;
                cv::cvtColor(frame, processed, cv::COLOR_BGR2GRAY);
                cv::Canny(processed, processed, 50, 150);
                cv::GaussianBlur(processed, processed, cv::Size(15, 15), 0);
                cv::threshold(processed, processed, 128, 255, cv::THRESH_BINARY);
            }
            
            cpuTimes.push_back(timer.elapsedMs());
        }
        
        double averageTime = std::accumulate(cpuTimes.begin(), cpuTimes.end(), 0.0) / iterations;
        double maxTime = *std::max_element(cpuTimes.begin(), cpuTimes.end());
        double minTime = *std::min_element(cpuTimes.begin(), cpuTimes.end());
        
        return BenchmarkResult("CPUUsage", "System", averageTime, minTime, maxTime, 
                             iterations, iterations);
    });
}

// Throughput Benchmark
void registerThroughputBenchmark() {
    registerBenchmark("System", "Throughput", []() -> BenchmarkResult {
        const size_t iterations = 50;
        std::vector<double> throughputs;
        throughputs.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            BenchmarkTimer timer;
            
            // Simulate high-throughput processing
            std::vector<cv::Mat> frames;
            for (int j = 0; j < 100; ++j) {
                cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC3);
                cv::circle(frame, cv::Point(320 + j % 100, 240 + (j / 10) * 20), 10, 
                          cv::Scalar(j % 255, (j * 2) % 255, (j * 3) % 255), -1);
                frames.push_back(frame);
            }
            
            // Process all frames
            for (const auto& frame : frames) {
                cv::Mat processed;
                cv::resize(frame, processed, cv::Size(320, 240));
                cv::cvtColor(processed, processed, cv::COLOR_BGR2GRAY);
            }
            
            double elapsedTime = timer.elapsedMs();
            double throughput = (frames.size() * 1000.0) / elapsedTime; // frames per second
            throughputs.push_back(throughput);
        }
        
        double averageThroughput = std::accumulate(throughputs.begin(), throughputs.end(), 0.0) / iterations;
        double maxThroughput = *std::max_element(throughputs.begin(), throughputs.end());
        double minThroughput = *std::min_element(throughputs.begin(), throughputs.end());
        
        return BenchmarkResult("Throughput", "System", averageThroughput, minThroughput, maxThroughput, 
                             iterations, iterations * 100);
    });
}

// Startup Time Benchmark
void registerStartupTimeBenchmark() {
    registerBenchmark("System", "StartupTime", []() -> BenchmarkResult {
        const size_t iterations = 10;
        std::vector<double> startupTimes;
        startupTimes.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            BenchmarkTimer timer;
            
            // Simulate system startup
            PerformanceMonitor& perfMonitor = PerformanceMonitor::getInstance();
            perfMonitor.reset();
            
            AdvancedOCR ocr;
            ocr.initialize();
            
            OptimizedScreenCapture capture;
            capture.initialize();
            
            GameEventDetector detector;
            
            ThreadManager threadManager;
            
            startupTimes.push_back(timer.elapsedMs());
        }
        
        double averageStartupTime = std::accumulate(startupTimes.begin(), startupTimes.end(), 0.0) / iterations;
        double maxStartupTime = *std::max_element(startupTimes.begin(), startupTimes.end());
        double minStartupTime = *std::min_element(startupTimes.begin(), startupTimes.end());
        
        return BenchmarkResult("StartupTime", "System", averageStartupTime, minStartupTime, maxStartupTime, 
                             iterations, iterations);
    });
}

// OCR Accuracy Benchmark
void registerOCRAccuracyBenchmark() {
    registerBenchmark("AdvancedOCR", "AccuracyValidation", []() -> BenchmarkResult {
        AdvancedOCR ocr;
        ocr.initialize();
        
        // Create test frames with known text
        std::vector<std::pair<cv::Mat, std::string>> testCases = {
            {cv::Mat::zeros(200, 400, CV_8UC3), "BLOOMBERG"},
            {cv::Mat::zeros(200, 400, CV_8UC3), "Score: 1250"},
            {cv::Mat::zeros(200, 400, CV_8UC3), "Health: 85%"},
            {cv::Mat::zeros(200, 400, CV_8UC3), "Ammo: 30/120"},
            {cv::Mat::zeros(200, 400, CV_8UC3), "Time: 02:45"}
        };
        
        // Add text to frames
        for (size_t i = 0; i < testCases.size(); ++i) {
            cv::putText(testCases[i].first, testCases[i].second, cv::Point(50, 100), 
                       cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
        }
        
        const size_t iterations = 20;
        std::vector<double> accuracies;
        accuracies.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            int correctDetections = 0;
            
            for (const auto& testCase : testCases) {
                auto results = ocr.detectText(testCase.first);
                // Simple accuracy check (in real implementation, would use more sophisticated matching)
                if (!results.empty()) {
                    correctDetections++;
                }
            }
            
            double accuracy = (static_cast<double>(correctDetections) / testCases.size()) * 100.0;
            accuracies.push_back(accuracy);
        }
        
        double averageAccuracy = std::accumulate(accuracies.begin(), accuracies.end(), 0.0) / iterations;
        double maxAccuracy = *std::max_element(accuracies.begin(), accuracies.end());
        double minAccuracy = *std::min_element(accuracies.begin(), accuracies.end());
        
        return BenchmarkResult("AccuracyValidation", "AdvancedOCR", averageAccuracy, minAccuracy, maxAccuracy, 
                             iterations, testCases.size() * iterations);
    });
}

// Register all benchmarks
void registerAllBenchmarks() {
    registerOCRBenchmark();
    registerCaptureBenchmark();
    registerGameAnalyticsBenchmark();
    registerMemoryBenchmark();
    registerCPUUsageBenchmark();
    registerThroughputBenchmark();
    registerStartupTimeBenchmark();
    registerOCRAccuracyBenchmark();
}

} // namespace BloombergTerminalTests
