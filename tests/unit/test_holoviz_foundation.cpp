#include "holoscan_fix.hpp"
#include "operators/holoviz_compatible_tensors.hpp"
#include "operators/holoviz_tensor_factory.hpp"
#include "operators/holoviz_static_config.hpp"
#include "utils/yolo_utils.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "=== HolovizOp Foundation Unit Tests ===" << std::endl;
    
    int test_count = 0;
    int passed_count = 0;
    
    // Test 1: Create default label dictionary
    std::cout << "\n[TEST 1] Creating default urology label dictionary..." << std::endl;
    test_count++;
    auto label_dict = urology::create_default_urology_label_dict();
    if (!label_dict.empty()) {
        std::cout << "✓ PASSED: Created label dictionary with " << label_dict.size() << " classes" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Label dictionary is empty" << std::endl;
    }
    
    // Test 2: Create static tensor configuration
    std::cout << "\n[TEST 2] Creating static tensor configuration..." << std::endl;
    test_count++;
    auto static_config = urology::create_static_tensor_config(label_dict);
    if (!static_config.empty()) {
        std::cout << "✓ PASSED: Created static configuration with " << static_config.size() << " tensor specs" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Static configuration is empty" << std::endl;
    }
    
    // Test 3: Validate static configuration
    std::cout << "\n[TEST 3] Validating static configuration..." << std::endl;
    test_count++;
    if (urology::validate_static_config(static_config)) {
        std::cout << "✓ PASSED: Static configuration validation passed" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Static configuration validation failed" << std::endl;
    }
    
    // Test 4: Create rectangle tensor
    std::cout << "\n[TEST 4] Creating rectangle tensor..." << std::endl;
    test_count++;
    std::vector<std::vector<float>> test_boxes = {
        {100.0f, 150.0f, 200.0f, 250.0f},
        {300.0f, 400.0f, 350.0f, 450.0f}
    };
    // Use HolovizCompatibleTensors instead of non-existent factory
    urology::HolovizCompatibleTensors test_tensors;
    test_tensors.add_rectangle_tensor("test_rect", test_boxes);
    auto rect_tensor = test_tensors.get_tensor("test_rect");
    if (rect_tensor) {
        std::cout << "✓ PASSED: Rectangle tensor created successfully" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Failed to create rectangle tensor" << std::endl;
    }
    
    // Test 5: Create text tensor
    std::cout << "\n[TEST 5] Creating text tensor..." << std::endl;
    test_count++;
    std::vector<std::vector<float>> test_positions = {
        {100.0f, 150.0f, 0.04f},
        {200.0f, 150.0f, 0.04f}
    };
    test_tensors.add_text_tensor("test_text", test_positions);
    auto text_tensor = test_tensors.get_tensor("test_text");
    if (text_tensor) {
        std::cout << "✓ PASSED: Text tensor created successfully" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Failed to create text tensor" << std::endl;
    }
    
    // Test 6: Create color LUT tensor
    std::cout << "\n[TEST 6] Creating color LUT tensor..." << std::endl;
    test_count++;
    std::vector<std::vector<float>> test_masks = {
        {0.1f, 0.2f, 0.3f, 0.4f},  // 2x2 mask
        {0.5f, 0.6f, 0.7f, 0.8f}   // 2x2 mask
    };
    // Create a dummy tensor for mask testing
    std::vector<float> mask_data = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
    std::vector<int64_t> mask_shape = {2, 2, 2}; // 2 masks, 2x2 each
    auto mask_tensor = urology::yolo_utils::make_holoscan_tensor_simple(mask_data, mask_shape);
    test_tensors.add_mask_tensor("test_mask", mask_tensor);
    auto color_lut_tensor = test_tensors.get_tensor("test_mask");
    if (color_lut_tensor) {
        std::cout << "✓ PASSED: Color LUT tensor created successfully" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Failed to create color LUT tensor" << std::endl;
    }
    
    // Test 7: Create HolovizCompatibleTensors structure
    std::cout << "\n[TEST 7] Creating HolovizCompatibleTensors structure..." << std::endl;
    test_count++;
    urology::HolovizCompatibleTensors compatible_tensors;
    compatible_tensors.label_dict = label_dict;
    
    // Add test tensors
    compatible_tensors.add_rectangle_tensor("boxes1", test_boxes);
    compatible_tensors.add_text_tensor("label1", test_positions);
    
    // Use the existing mask tensor from test 6
    compatible_tensors.add_mask_tensor("masks", mask_tensor);
    
    if (compatible_tensors.tensor_count() > 0) {
        std::cout << "✓ PASSED: HolovizCompatibleTensors created with " 
                  << compatible_tensors.tensor_count() << " tensors" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: HolovizCompatibleTensors has no tensors" << std::endl;
    }
    
    // Test 8: Validate HolovizCompatibleTensors
    std::cout << "\n[TEST 8] Validating HolovizCompatibleTensors..." << std::endl;
    test_count++;
    if (compatible_tensors.validate()) {
        std::cout << "✓ PASSED: HolovizCompatibleTensors validation passed" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: HolovizCompatibleTensors validation failed" << std::endl;
    }
    
    // Test 9: Create dynamic InputSpecs
    std::cout << "\n[TEST 9] Creating dynamic InputSpecs..." << std::endl;
    test_count++;
    std::vector<urology::Detection> test_detections = {
        {{100.0f, 150.0f, 200.0f, 250.0f}, 0.85f, 0},
        {{300.0f, 400.0f, 350.0f, 450.0f}, 0.92f, 1}
    };
    auto dynamic_specs = urology::create_dynamic_input_specs(test_detections, label_dict);
    if (!dynamic_specs.empty()) {
        std::cout << "✓ PASSED: Created " << dynamic_specs.size() << " dynamic InputSpecs" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: No dynamic InputSpecs created" << std::endl;
    }
    
    // Test 10: Test tensor summary
    std::cout << "\n[TEST 10] Testing tensor summary..." << std::endl;
    test_count++;
    std::string summary = compatible_tensors.get_summary();
    if (!summary.empty()) {
        std::cout << "✓ PASSED: Tensor summary generated successfully" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Tensor summary generation failed" << std::endl;
    }
    
    // Summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << test_count << std::endl;
    std::cout << "Passed: " << passed_count << std::endl;
    std::cout << "Failed: " << (test_count - passed_count) << std::endl;
    std::cout << "Success rate: " << (passed_count * 100 / test_count) << "%" << std::endl;
    
    if (passed_count == test_count) {
        std::cout << "\n🎉 ALL TESTS PASSED! Phase 1 foundation is ready." << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ SOME TESTS FAILED. Please review the implementation." << std::endl;
        return 1;
    }
} 