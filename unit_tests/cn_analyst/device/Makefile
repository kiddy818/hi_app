# Makefile for Device Module Unit Tests

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread -I../../..

# Source files
DEVICE_SRC_DIR := ../../../cn_analyst/device/src
DEVICE_SRCS := $(DEVICE_SRC_DIR)/resource_manager.cpp \
               $(DEVICE_SRC_DIR)/stream_config.cpp \
               $(DEVICE_SRC_DIR)/camera_instance.cpp \
               $(DEVICE_SRC_DIR)/camera_manager.cpp

# Test files
TEST_DIR := .
TEST_SRCS := $(TEST_DIR)/resource_manager_test.cpp \
             $(TEST_DIR)/camera_manager_test.cpp

# Output binaries
TESTS := resource_manager_test camera_manager_test

.PHONY: all clean test

all: $(TESTS)

resource_manager_test: $(TEST_DIR)/resource_manager_test.cpp $(DEVICE_SRC_DIR)/resource_manager.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

camera_manager_test: $(TEST_DIR)/camera_manager_test.cpp \
                     $(DEVICE_SRC_DIR)/camera_manager.cpp \
                     $(DEVICE_SRC_DIR)/camera_instance.cpp \
                     $(DEVICE_SRC_DIR)/stream_config.cpp \
                     $(DEVICE_SRC_DIR)/resource_manager.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TESTS)
	@echo "Running unit tests..."
	@for test in $(TESTS); do \
		echo ""; \
		./$$test || exit 1; \
	done
	@echo ""
	@echo "All tests passed!"

clean:
	rm -f $(TESTS) *.o

help:
	@echo "Available targets:"
	@echo "  all   - Build all tests (default)"
	@echo "  test  - Build and run all tests"
	@echo "  clean - Remove built files"
	@echo "  help  - Show this help message"
