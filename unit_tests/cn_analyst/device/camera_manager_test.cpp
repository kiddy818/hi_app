#include "../../../cn_analyst/device/src/camera_manager.h"
#include "../../../cn_analyst/device/src/resource_manager.h"
#include <cassert>
#include <iostream>

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

// Helper to create a simple camera config
camera_config create_test_config(int32_t camera_id) {
    camera_config config;
    config.camera_id = camera_id;
    config.enabled = true;
    config.sensor = sensor_config("OS04A10", "liner");
    
    // Add a main stream
    stream_config main_stream;
    main_stream.stream_id = 0;
    main_stream.name = "main";
    main_stream.type = encoder_type::H264_CBR;
    main_stream.width = 1920;
    main_stream.height = 1080;
    main_stream.framerate = 30;
    main_stream.bitrate = 4096;
    main_stream.outputs.rtsp_enabled = true;
    main_stream.outputs.rtsp_url_path = "/stream1";
    
    config.streams.push_back(main_stream);
    
    return config;
}

// Test: Initialization
bool test_initialization() {
    // Initialize resource manager first
    resource_limits limits;
    resource_manager::init(limits);
    
    TEST_ASSERT(camera_manager::init(4), "Should initialize successfully");
    TEST_ASSERT(camera_manager::is_initialized(), "Should be initialized");
    TEST_ASSERT(camera_manager::get_max_cameras() == 4, "Max cameras should be 4");
    
    camera_manager::release();
    resource_manager::release();
    
    TEST_ASSERT(!camera_manager::is_initialized(), "Should not be initialized after release");
    
    return true;
}

// Test: Invalid initialization parameters
bool test_invalid_init() {
    TEST_ASSERT(!camera_manager::init(0), "Should fail with 0 cameras");
    TEST_ASSERT(!camera_manager::init(-1), "Should fail with negative cameras");
    TEST_ASSERT(!camera_manager::init(5), "Should fail with more than 4 cameras");
    
    return true;
}

// Test: Create and destroy camera
bool test_create_destroy() {
    resource_limits limits;
    resource_manager::init(limits);
    camera_manager::init(4);
    
    auto config = create_test_config(-1); // Auto-assign ID
    
    // Debug: validate config first
    std::string error_msg;
    if (!camera_manager::validate_config(config, error_msg)) {
        std::cerr << "Config validation failed: " << error_msg << std::endl;
    }
    
    auto camera = camera_manager::create_camera(config);
    
    TEST_ASSERT(camera != nullptr, "Should create camera");
    TEST_ASSERT(camera->camera_id() >= 0, "Camera should have valid ID");
    TEST_ASSERT(camera_manager::camera_exists(camera->camera_id()), 
               "Camera should exist in manager");
    TEST_ASSERT(camera_manager::get_camera_count() == 1, "Count should be 1");
    
    // Destroy camera
    TEST_ASSERT(camera_manager::destroy_camera(camera->camera_id()), 
               "Should destroy camera");
    TEST_ASSERT(!camera_manager::camera_exists(camera->camera_id()), 
               "Camera should not exist after destroy");
    TEST_ASSERT(camera_manager::get_camera_count() == 0, "Count should be 0");
    
    camera_manager::release();
    resource_manager::release();
    return true;
}

// Test: Multiple cameras
bool test_multiple_cameras() {
    resource_limits limits;
    resource_manager::init(limits);
    camera_manager::init(4);
    
    std::vector<std::shared_ptr<camera_instance>> cameras;
    
    // Create 3 cameras
    for (int i = 0; i < 3; ++i) {
        auto config = create_test_config(-1);
        auto camera = camera_manager::create_camera(config);
        TEST_ASSERT(camera != nullptr, "Should create camera");
        cameras.push_back(camera);
    }
    
    TEST_ASSERT(camera_manager::get_camera_count() == 3, "Count should be 3");
    
    // List cameras
    auto ids = camera_manager::list_cameras();
    TEST_ASSERT(ids.size() == 3, "Should list 3 cameras");
    
    // Destroy all
    camera_manager::destroy_all_cameras();
    TEST_ASSERT(camera_manager::get_camera_count() == 0, "Count should be 0 after destroy all");
    
    camera_manager::release();
    resource_manager::release();
    return true;
}

// Test: Camera limit enforcement
bool test_camera_limit() {
    resource_limits limits;
    resource_manager::init(limits);
    camera_manager::init(2); // Limit to 2 cameras
    
    std::vector<std::shared_ptr<camera_instance>> cameras;
    
    // Create 2 cameras (should succeed)
    for (int i = 0; i < 2; ++i) {
        auto config = create_test_config(-1);
        auto camera = camera_manager::create_camera(config);
        TEST_ASSERT(camera != nullptr, "Should create camera within limit");
        cameras.push_back(camera);
    }
    
    // Try to create third camera (should fail)
    auto config = create_test_config(-1);
    auto camera = camera_manager::create_camera(config);
    TEST_ASSERT(camera == nullptr, "Should fail when limit reached");
    
    camera_manager::release();
    resource_manager::release();
    return true;
}

// Test: Get camera by ID
bool test_get_camera() {
    resource_limits limits;
    resource_manager::init(limits);
    camera_manager::init(4);
    
    auto config = create_test_config(-1);
    auto camera1 = camera_manager::create_camera(config);
    TEST_ASSERT(camera1 != nullptr, "Should create camera");
    
    int32_t id = camera1->camera_id();
    auto camera2 = camera_manager::get_camera(id);
    
    TEST_ASSERT(camera2 != nullptr, "Should get camera by ID");
    TEST_ASSERT(camera2 == camera1, "Should be same camera instance");
    
    // Try to get non-existent camera
    auto camera3 = camera_manager::get_camera(9999);
    TEST_ASSERT(camera3 == nullptr, "Should return nullptr for non-existent camera");
    
    camera_manager::release();
    resource_manager::release();
    return true;
}

// Test: Configuration validation
bool test_config_validation() {
    resource_limits limits;
    resource_manager::init(limits);
    camera_manager::init(4);
    
    std::string error_msg;
    
    // Valid config
    auto config = create_test_config(-1);
    TEST_ASSERT(camera_manager::validate_config(config, error_msg), 
               "Valid config should pass");
    
    // Disabled camera
    config.enabled = false;
    TEST_ASSERT(!camera_manager::validate_config(config, error_msg), 
               "Disabled camera should fail");
    config.enabled = true;
    
    // Invalid sensor
    config.sensor.name = "INVALID_SENSOR";
    TEST_ASSERT(!camera_manager::validate_config(config, error_msg), 
               "Invalid sensor should fail");
    config.sensor.name = "OS04A10";
    
    // No streams
    config.streams.clear();
    TEST_ASSERT(!camera_manager::validate_config(config, error_msg), 
               "No streams should fail");
    
    camera_manager::release();
    resource_manager::release();
    return true;
}

// Test: Can create camera (resource check)
bool test_can_create() {
    resource_limits limits;
    limits.max_vi_devices = 2; // Limit to 2 VI devices
    resource_manager::init(limits);
    camera_manager::init(4);
    
    // Should be able to create first camera
    auto config = create_test_config(-1);
    TEST_ASSERT(camera_manager::can_create_camera(config), 
               "Should be able to create first camera");
    
    auto camera1 = camera_manager::create_camera(config);
    TEST_ASSERT(camera1 != nullptr, "Should create first camera");
    camera1->start(); // Start to allocate resources
    
    // Should be able to create second camera
    TEST_ASSERT(camera_manager::can_create_camera(config), 
               "Should be able to create second camera");
    
    auto camera2 = camera_manager::create_camera(config);
    TEST_ASSERT(camera2 != nullptr, "Should create second camera");
    camera2->start(); // Start to allocate resources
    
    // Should NOT be able to create third camera (VI limit reached)
    TEST_ASSERT(!camera_manager::can_create_camera(config), 
               "Should not be able to create third camera (resource limit)");
    
    camera_manager::release();
    resource_manager::release();
    return true;
}

// Test: Camera lifecycle
bool test_camera_lifecycle() {
    resource_limits limits;
    resource_manager::init(limits);
    camera_manager::init(4);
    
    auto config = create_test_config(-1);
    auto camera = camera_manager::create_camera(config);
    TEST_ASSERT(camera != nullptr, "Should create camera");
    
    // Initially not running
    TEST_ASSERT(!camera->is_running(), "Camera should not be running initially");
    
    // Start camera
    TEST_ASSERT(camera->start(), "Should start camera");
    TEST_ASSERT(camera->is_running(), "Camera should be running");
    
    // Stop camera
    camera->stop();
    TEST_ASSERT(!camera->is_running(), "Camera should not be running after stop");
    
    camera_manager::release();
    resource_manager::release();
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;
    
    std::cout << "=== Camera Manager Unit Tests ===" << std::endl;
    
    RUN_TEST(test_initialization);
    RUN_TEST(test_invalid_init);
    RUN_TEST(test_create_destroy);
    RUN_TEST(test_multiple_cameras);
    RUN_TEST(test_camera_limit);
    RUN_TEST(test_get_camera);
    RUN_TEST(test_config_validation);
    RUN_TEST(test_can_create);
    RUN_TEST(test_camera_lifecycle);
    
    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return (failed == 0) ? 0 : 1;
}
