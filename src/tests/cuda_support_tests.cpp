#include "../test_framework.h"
#include "../cuda_support.h"

namespace CudaSupportTests {
    
    TestFramework::TestResult testCudaAvailability() {
        bool available = CudaSupport::isAvailable();
        
        // We can't predict if CUDA is available, but the function should not crash
        ASSERT_TRUE(available == true || available == false, "CUDA availability should return valid boolean");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testDeviceCount() {
        int count = CudaSupport::getDeviceCount();
        
        ASSERT_TRUE(count >= 0, "Device count should be non-negative");
        ASSERT_TRUE(count <= 16, "Device count should be reasonable (max 16 devices)");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testDeviceName() {
        std::string name = CudaSupport::getDeviceName(0);
        
        if (CudaSupport::isAvailable() && CudaSupport::getDeviceCount() > 0) {
            ASSERT_TRUE(!name.empty(), "Device name should not be empty when CUDA is available");
        } else {
            // When CUDA is not available, name should be empty or indicate no device
            ASSERT_TRUE(name.empty() || name == "No CUDA device", "Device name should indicate no device when CUDA unavailable");
        }
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testComputeCapability() {
        bool hasMinCapability = CudaSupport::hasMinComputeCapability(0, 2.0);
        
        if (CudaSupport::isAvailable() && CudaSupport::getDeviceCount() > 0) {
            ASSERT_TRUE(hasMinCapability == true || hasMinCapability == false, "Compute capability check should return valid boolean");
        } else {
            ASSERT_FALSE(hasMinCapability, "Should return false when CUDA is not available");
        }
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testBestDeviceSelection() {
        int bestDevice = CudaSupport::selectBestDevice();
        
        ASSERT_TRUE(bestDevice >= -1, "Best device should be -1 (no device) or non-negative");
        
        if (CudaSupport::isAvailable() && CudaSupport::getDeviceCount() > 0) {
            ASSERT_TRUE(bestDevice >= 0, "Should select a valid device when CUDA is available");
            ASSERT_TRUE(bestDevice < CudaSupport::getDeviceCount(), "Selected device should be within valid range");
        } else {
            ASSERT_TRUE(bestDevice == -1, "Should return -1 when no CUDA devices available");
        }
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testInvalidDeviceHandling() {
        // Test with invalid device index
        std::string name = CudaSupport::getDeviceName(999);
        bool hasCapability = CudaSupport::hasMinComputeCapability(999, 2.0);
        
        ASSERT_TRUE(name.empty(), "Invalid device index should return empty name");
        ASSERT_FALSE(hasCapability, "Invalid device index should return false for compute capability");
        
        TEST_PASS();
    }
    
    void registerTests() {
        auto& framework = TestFramework::getInstance();
        
        framework.addTest("CudaSupport", "CUDA Availability", testCudaAvailability);
        framework.addTest("CudaSupport", "Device Count", testDeviceCount);
        framework.addTest("CudaSupport", "Device Name", testDeviceName);
        framework.addTest("CudaSupport", "Compute Capability", testComputeCapability);
        framework.addTest("CudaSupport", "Best Device Selection", testBestDeviceSelection);
        framework.addTest("CudaSupport", "Invalid Device Handling", testInvalidDeviceHandling);
    }
}
