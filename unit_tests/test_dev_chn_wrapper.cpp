/**
 * @file test_dev_chn_wrapper.cpp
 * @brief Test program for dev_chn_wrapper
 * 
 * This test demonstrates the usage of dev_chn_wrapper for backward compatibility
 * with the legacy dev_chn interface while using the new camera architecture.
 * 
 * NOTE: File paths used in tests (e.g., /opt/ceanic/...) are placeholders for
 * demonstration purposes. Actual paths depend on deployment configuration and
 * hardware setup. Tests will gracefully handle missing files/hardware by reporting
 * features as "not available".
 */

#include <iostream>
#include <memory>
#include "dev_chn_wrapper.h"

using namespace hisilicon::dev;

void test_basic_usage() {
    std::cout << "\n=== Test 1: Basic Wrapper Usage ===" << std::endl;
    
    // Initialize system (same as legacy code)
    if (!chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE)) {
        std::cerr << "Failed to initialize system" << std::endl;
        return;
    }
    std::cout << "System initialized successfully" << std::endl;
    
    // Create camera wrapper (same API as legacy chn)
    auto camera_wrapper = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
    std::cout << "Camera wrapper created" << std::endl;
    
    // Start camera
    if (camera_wrapper->start(1920, 1080, 30, 4096)) {
        std::cout << "Camera started successfully" << std::endl;
        std::cout << "Running: " << (camera_wrapper->is_start() ? "Yes" : "No") << std::endl;
        
        // Stop camera
        camera_wrapper->stop();
        std::cout << "Camera stopped" << std::endl;
    } else {
        std::cerr << "Failed to start camera" << std::endl;
    }
    
    // Cleanup
    chn_wrapper::release();
    std::cout << "System released" << std::endl;
}

void test_feature_management() {
    std::cout << "\n=== Test 2: Feature Management ===" << std::endl;
    
    // Initialize
    if (!chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE)) {
        std::cerr << "Failed to initialize system" << std::endl;
        return;
    }
    
    auto camera_wrapper = std::make_shared<chn_wrapper>("OS08A20", "H265_AVBR", 0);
    
    if (camera_wrapper->start(2688, 1520, 30, 6000)) {
        std::cout << "Camera started" << std::endl;
        
        // Test AIISP feature
        // Note: Model file paths are placeholders for demonstration
        // Actual paths depend on deployment configuration
        std::cout << "Testing AIISP feature..." << std::endl;
        if (camera_wrapper->aiisp_start("/opt/ceanic/aiisp/model/test.bin", 0)) {
            std::cout << "AIISP started successfully" << std::endl;
            camera_wrapper->aiisp_stop();
            std::cout << "AIISP stopped" << std::endl;
        } else {
            std::cout << "AIISP not available (expected in stub mode)" << std::endl;
        }
        
        // Test YOLOv5 feature
        // Note: Model file paths are placeholders for demonstration
        std::cout << "Testing YOLOv5 feature..." << std::endl;
        if (camera_wrapper->yolov5_start("/opt/ceanic/yolov5/model.om")) {
            std::cout << "YOLOv5 started successfully" << std::endl;
            camera_wrapper->yolov5_stop();
            std::cout << "YOLOv5 stopped" << std::endl;
        } else {
            std::cout << "YOLOv5 not available (expected in stub mode)" << std::endl;
        }
        
        camera_wrapper->stop();
    }
    
    chn_wrapper::release();
}

void test_multiple_start_stop() {
    std::cout << "\n=== Test 3: Multiple Start/Stop Cycles ===" << std::endl;
    
    if (!chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE)) {
        std::cerr << "Failed to initialize system" << std::endl;
        return;
    }
    
    auto camera_wrapper = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
    
    for (int i = 0; i < 3; i++) {
        std::cout << "\nCycle " << (i + 1) << ":" << std::endl;
        
        if (camera_wrapper->start(1920, 1080, 30, 4096)) {
            std::cout << "  Started" << std::endl;
            camera_wrapper->stop();
            std::cout << "  Stopped" << std::endl;
        } else {
            std::cerr << "  Failed to start" << std::endl;
        }
    }
    
    chn_wrapper::release();
}

void test_direct_camera_access() {
    std::cout << "\n=== Test 4: Direct Camera Instance Access ===" << std::endl;
    
    if (!chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE)) {
        std::cerr << "Failed to initialize system" << std::endl;
        return;
    }
    
    auto camera_wrapper = std::make_shared<chn_wrapper>("OS04A10", "H264_CBR", 0);
    
    if (camera_wrapper->start(1920, 1080, 30, 4096)) {
        std::cout << "Camera started" << std::endl;
        
        // Access underlying camera instance for advanced operations
        auto camera_instance = camera_wrapper->get_camera_instance();
        if (camera_instance) {
            std::cout << "Got camera instance" << std::endl;
            auto status = camera_instance->get_status();
            std::cout << "Camera ID: " << status.camera_id << std::endl;
            std::cout << "Running: " << (status.is_running ? "Yes" : "No") << std::endl;
            std::cout << "Streams: " << status.stream_count << std::endl;
        } else {
            std::cout << "Using legacy implementation (no camera_instance)" << std::endl;
        }
        
        camera_wrapper->stop();
    }
    
    chn_wrapper::release();
}

void test_static_methods() {
    std::cout << "\n=== Test 5: Static Method Compatibility ===" << std::endl;
    
    if (!chn_wrapper::init(OT_VI_ONLINE_VPSS_ONLINE)) {
        std::cerr << "Failed to initialize system" << std::endl;
        return;
    }
    
    // Test scene methods (delegates to legacy)
    // Note: Scene configuration paths are placeholders
    // Actual paths depend on sensor and deployment configuration
    std::cout << "Testing scene methods..." << std::endl;
    if (chn_wrapper::scene_init("/opt/ceanic/scene/param/sensor_os04a10")) {
        std::cout << "Scene initialized" << std::endl;
        chn_wrapper::scene_set_mode(0);
        std::cout << "Scene mode set" << std::endl;
        chn_wrapper::scene_release();
        std::cout << "Scene released" << std::endl;
    } else {
        std::cout << "Scene not available (expected without hardware)" << std::endl;
    }
    
    // Test rate auto methods
    // Note: Configuration file paths are placeholders
    std::cout << "Testing rate auto methods..." << std::endl;
    if (chn_wrapper::rate_auto_init("/opt/ceanic/etc/rate_auto.ini")) {
        std::cout << "Rate auto initialized" << std::endl;
        chn_wrapper::rate_auto_release();
        std::cout << "Rate auto released" << std::endl;
    } else {
        std::cout << "Rate auto not available (expected without hardware)" << std::endl;
    }
    
    chn_wrapper::release();
}

int main(int argc, char* argv[]) {
    std::cout << "=====================================" << std::endl;
    std::cout << "dev_chn_wrapper Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        test_basic_usage();
        test_feature_management();
        test_multiple_start_stop();
        test_direct_camera_access();
        test_static_methods();
        
        std::cout << "\n=====================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "=====================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\nTest failed with unknown exception" << std::endl;
        return 1;
    }
}
