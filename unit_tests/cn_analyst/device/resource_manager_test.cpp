#include "../../../cn_analyst/device/src/resource_manager.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

using namespace hisilicon::device;

// Test helper
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAILED: " << message << std::endl; \
        return false; \
    }

#define RUN_TEST(test_func) \
    std::cout << "Running " << #test_func << "..." << std::endl; \
    if (test_func()) { \
        std::cout << "  PASSED" << std::endl; \
        passed++; \
    } else { \
        std::cout << "  FAILED" << std::endl; \
        failed++; \
    }

// Test: Initialization
bool test_initialization() {
    resource_limits limits;
    limits.max_vi_devices = 4;
    limits.max_vpss_groups = 32;
    limits.max_venc_channels = 16;
    
    TEST_ASSERT(resource_manager::init(limits), "Should initialize successfully");
    TEST_ASSERT(resource_manager::is_initialized(), "Should be initialized");
    
    resource_manager::release();
    TEST_ASSERT(!resource_manager::is_initialized(), "Should not be initialized after release");
    
    return true;
}

// Test: Double initialization should fail
bool test_double_init() {
    resource_limits limits;
    TEST_ASSERT(resource_manager::init(limits), "First init should succeed");
    TEST_ASSERT(!resource_manager::init(limits), "Second init should fail");
    
    resource_manager::release();
    return true;
}

// Test: VPSS group allocation
bool test_vpss_allocation() {
    resource_limits limits;
    limits.max_vpss_groups = 4;
    resource_manager::init(limits);
    
    // Allocate groups
    std::vector<int32_t> groups;
    for (int i = 0; i < 4; ++i) {
        int32_t grp;
        TEST_ASSERT(resource_manager::allocate_vpss_group(grp), "Should allocate VPSS group");
        groups.push_back(grp);
    }
    
    // Try to allocate beyond limit
    int32_t grp;
    TEST_ASSERT(!resource_manager::allocate_vpss_group(grp), "Should fail when limit reached");
    TEST_ASSERT(!resource_manager::is_vpss_group_available(), "No groups should be available");
    
    // Free a group
    resource_manager::free_vpss_group(groups[0]);
    TEST_ASSERT(resource_manager::is_vpss_group_available(), "One group should be available");
    
    // Allocate again
    TEST_ASSERT(resource_manager::allocate_vpss_group(grp), "Should allocate after free");
    
    resource_manager::release();
    return true;
}

// Test: VENC channel allocation
bool test_venc_allocation() {
    resource_limits limits;
    limits.max_venc_channels = 8;
    limits.max_h264_channels = 6;
    limits.max_h265_channels = 4;
    resource_manager::init(limits);
    
    // Allocate H.264 channels
    std::vector<int32_t> h264_chns;
    for (int i = 0; i < 6; ++i) {
        int32_t chn;
        TEST_ASSERT(resource_manager::allocate_venc_channel(venc_type::H264, chn), 
                   "Should allocate H.264 channel");
        h264_chns.push_back(chn);
    }
    
    // Try to allocate beyond H.264 limit
    int32_t chn;
    TEST_ASSERT(!resource_manager::allocate_venc_channel(venc_type::H264, chn), 
               "Should fail when H.264 limit reached");
    
    // Allocate H.265 channels (should still work, total limit not reached)
    std::vector<int32_t> h265_chns;
    for (int i = 0; i < 2; ++i) {
        TEST_ASSERT(resource_manager::allocate_venc_channel(venc_type::H265, chn), 
                   "Should allocate H.265 channel");
        h265_chns.push_back(chn);
    }
    
    // Now total limit is reached (6 H.264 + 2 H.265 = 8)
    TEST_ASSERT(!resource_manager::allocate_venc_channel(venc_type::H265, chn), 
               "Should fail when total limit reached");
    
    // Free an H.264 channel
    resource_manager::free_venc_channel(h264_chns[0]);
    
    // Should be able to allocate H.265 now
    TEST_ASSERT(resource_manager::allocate_venc_channel(venc_type::H265, chn), 
               "Should allocate after free");
    
    resource_manager::release();
    return true;
}

// Test: VI device allocation
bool test_vi_allocation() {
    resource_limits limits;
    limits.max_vi_devices = 4;
    resource_manager::init(limits);
    
    // Allocate all devices
    std::vector<int32_t> devices;
    for (int i = 0; i < 4; ++i) {
        int32_t dev;
        TEST_ASSERT(resource_manager::allocate_vi_device(dev), "Should allocate VI device");
        devices.push_back(dev);
    }
    
    // Try to allocate beyond limit
    int32_t dev;
    TEST_ASSERT(!resource_manager::allocate_vi_device(dev), "Should fail when limit reached");
    
    // Free and reallocate
    resource_manager::free_vi_device(devices[1]);
    TEST_ASSERT(resource_manager::allocate_vi_device(dev), "Should allocate after free");
    
    resource_manager::release();
    return true;
}

// Test: Resource status query
bool test_status_query() {
    resource_limits limits;
    limits.max_vi_devices = 4;
    limits.max_vpss_groups = 8;
    limits.max_venc_channels = 16;
    limits.max_h264_channels = 12;
    limits.max_h265_channels = 8;
    resource_manager::init(limits);
    
    // Get initial status
    resource_status status = resource_manager::get_status();
    TEST_ASSERT(status.vi_devices.total == 4, "VI total should be 4");
    TEST_ASSERT(status.vi_devices.allocated == 0, "VI allocated should be 0");
    TEST_ASSERT(status.vi_devices.available == 4, "VI available should be 4");
    
    // Allocate some resources
    int32_t vi_dev, vpss_grp, venc_chn;
    resource_manager::allocate_vi_device(vi_dev);
    resource_manager::allocate_vpss_group(vpss_grp);
    resource_manager::allocate_venc_channel(venc_type::H264, venc_chn);
    
    // Check updated status
    status = resource_manager::get_status();
    TEST_ASSERT(status.vi_devices.allocated == 1, "VI allocated should be 1");
    TEST_ASSERT(status.vpss_groups.allocated == 1, "VPSS allocated should be 1");
    TEST_ASSERT(status.venc_h264.allocated == 1, "H.264 allocated should be 1");
    
    resource_manager::release();
    return true;
}

// Test: Thread safety (concurrent allocations)
bool test_thread_safety() {
    resource_limits limits;
    limits.max_vpss_groups = 32;
    resource_manager::init(limits);
    
    const int num_threads = 8;
    const int allocs_per_thread = 4;
    std::vector<std::thread> threads;
    std::vector<std::vector<int32_t>> allocated(num_threads);
    
    // Launch threads that allocate resources
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, &allocated]() {
            for (int i = 0; i < allocs_per_thread; ++i) {
                int32_t grp;
                if (resource_manager::allocate_vpss_group(grp)) {
                    allocated[t].push_back(grp);
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Count total allocations
    int total = 0;
    for (const auto& vec : allocated) {
        total += vec.size();
    }
    
    TEST_ASSERT(total == num_threads * allocs_per_thread, 
               "Should allocate correct total number");
    
    // Check for duplicates
    std::vector<int32_t> all_ids;
    for (const auto& vec : allocated) {
        all_ids.insert(all_ids.end(), vec.begin(), vec.end());
    }
    std::sort(all_ids.begin(), all_ids.end());
    auto unique_end = std::unique(all_ids.begin(), all_ids.end());
    TEST_ASSERT(unique_end == all_ids.end(), "No duplicate IDs should exist");
    
    resource_manager::release();
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;
    
    std::cout << "=== Resource Manager Unit Tests ===" << std::endl;
    
    RUN_TEST(test_initialization);
    RUN_TEST(test_double_init);
    RUN_TEST(test_vpss_allocation);
    RUN_TEST(test_venc_allocation);
    RUN_TEST(test_vi_allocation);
    RUN_TEST(test_status_query);
    RUN_TEST(test_thread_safety);
    
    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return (failed == 0) ? 0 : 1;
}
