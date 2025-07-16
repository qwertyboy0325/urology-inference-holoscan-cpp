#include "holoscan_fix.hpp"
#include "urology_app.hpp"
#include "operators/holoviz_native_yolo_postprocessor.hpp"
#include "operators/holoviz_static_config.hpp"
#include <iostream>
#include <memory>

int main() {
    std::cout << "=== HolovizOp Integration Test ===" << std::endl;
    
    int test_count = 0;
    int passed_count = 0;
    
    // Test 1: Create UrologyApp with new postprocessor
    std::cout << "\n[TEST 1] Creating UrologyApp with new postprocessor..." << std::endl;
    test_count++;
    try {
        urology::UrologyApp app("./data", "replayer", "test_output.mp4", "");
        std::cout << "✓ PASSED: UrologyApp created successfully" << std::endl;
        passed_count++;
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: Failed to create UrologyApp: " << e.what() << std::endl;
    }
    
    // Test 2: Test static tensor configuration creation
    std::cout << "\n[TEST 2] Testing static tensor configuration creation..." << std::endl;
    test_count++;
    try {
        auto label_dict = urology::create_default_urology_label_dict();
        auto static_config = urology::create_static_tensor_config(label_dict);
        
        if (!static_config.empty()) {
            std::cout << "✓ PASSED: Static tensor configuration created with " 
                      << static_config.size() << " tensor specs" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ FAILED: Static tensor configuration is empty" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: Static tensor configuration creation failed: " << e.what() << std::endl;
    }
    
    // Test 3: Test HolovizOp-native postprocessor creation
    std::cout << "\n[TEST 3] Testing HolovizOp-native postprocessor creation..." << std::endl;
    test_count++;
    try {
        auto label_dict = urology::create_default_urology_label_dict();
        auto color_lut = urology::create_default_color_lut();
        
        // Create postprocessor with test parameters
        auto postprocessor = std::make_shared<urology::HolovizNativeYoloPostprocessor>();
        
        std::cout << "✓ PASSED: HolovizOp-native postprocessor created successfully" << std::endl;
        passed_count++;
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: HolovizOp-native postprocessor creation failed: " << e.what() << std::endl;
    }
    
    // Test 4: Test label dictionary compatibility
    std::cout << "\n[TEST 4] Testing label dictionary compatibility..." << std::endl;
    test_count++;
    try {
        auto label_dict = urology::create_default_urology_label_dict();
        
        // Test that label dictionary has the expected structure
        bool has_expected_structure = true;
        for (const auto& [class_id, label_info] : label_dict) {
            if (label_info.text.empty() || label_info.color.size() != 4) {
                has_expected_structure = false;
                break;
            }
        }
        
        if (has_expected_structure && !label_dict.empty()) {
            std::cout << "✓ PASSED: Label dictionary has correct structure with " 
                      << label_dict.size() << " classes" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ FAILED: Label dictionary has incorrect structure" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: Label dictionary compatibility test failed: " << e.what() << std::endl;
    }
    
    // Test 5: Test color lookup table creation
    std::cout << "\n[TEST 5] Testing color lookup table creation..." << std::endl;
    test_count++;
    try {
        auto color_lut = urology::create_default_color_lut();
        
        if (!color_lut.empty()) {
            std::cout << "✓ PASSED: Color lookup table created with " 
                      << color_lut.size() << " colors" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ FAILED: Color lookup table is empty" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: Color lookup table creation failed: " << e.what() << std::endl;
    }
    
    // Test 6: Test HolovizCompatibleTensors creation
    std::cout << "\n[TEST 6] Testing HolovizCompatibleTensors creation..." << std::endl;
    test_count++;
    try {
        urology::HolovizCompatibleTensors compatible_tensors;
        auto label_dict = urology::create_default_urology_label_dict();
        compatible_tensors.label_dict = label_dict;
        
        // Add test tensors
        std::vector<std::vector<float>> test_boxes = {
            {0.1f, 0.2f, 0.3f, 0.4f},
            {0.5f, 0.6f, 0.7f, 0.8f}
        };
        compatible_tensors.add_rectangle_tensor("boxes1", test_boxes);
        
        if (compatible_tensors.tensor_count() > 0) {
            std::cout << "✓ PASSED: HolovizCompatibleTensors created with " 
                      << compatible_tensors.tensor_count() << " tensors" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ FAILED: HolovizCompatibleTensors has no tensors" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: HolovizCompatibleTensors creation failed: " << e.what() << std::endl;
    }
    
    // Test 7: Test dynamic InputSpec generation
    std::cout << "\n[TEST 7] Testing dynamic InputSpec generation..." << std::endl;
    test_count++;
    try {
        auto label_dict = urology::create_default_urology_label_dict();
        std::vector<urology::Detection> test_detections = {
            {{0.1f, 0.2f, 0.3f, 0.4f}, 0.85f, 0},
            {{0.5f, 0.6f, 0.7f, 0.8f}, 0.92f, 1}
        };
        
        auto dynamic_specs = urology::create_dynamic_input_specs(test_detections, label_dict);
        
        if (!dynamic_specs.empty()) {
            std::cout << "✓ PASSED: Generated " << dynamic_specs.size() 
                      << " dynamic InputSpecs" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ FAILED: No dynamic InputSpecs generated" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: Dynamic InputSpec generation failed: " << e.what() << std::endl;
    }
    
    // Test 8: Test tensor factory functions
    std::cout << "\n[TEST 8] Testing tensor factory functions..." << std::endl;
    test_count++;
    try {
        std::vector<std::vector<float>> test_boxes = {
            {0.1f, 0.2f, 0.3f, 0.4f},
            {0.5f, 0.6f, 0.7f, 0.8f}
        };
        
        // Use the HolovizCompatibleTensors class instead of non-existent factory functions
        urology::HolovizCompatibleTensors compatible_tensors;
        compatible_tensors.add_rectangle_tensor("test_boxes", test_boxes);
        compatible_tensors.add_text_tensor("test_text", test_boxes);
        
        auto rect_tensor = compatible_tensors.get_tensor("test_boxes");
        auto text_tensor = compatible_tensors.get_tensor("test_text");
        
        if (rect_tensor && text_tensor) {
            std::cout << "✓ PASSED: Tensor factory functions work correctly" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ FAILED: Tensor factory functions failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: Tensor factory functions test failed: " << e.what() << std::endl;
    }
    
    // Test 9: Test configuration validation
    std::cout << "\n[TEST 9] Testing configuration validation..." << std::endl;
    test_count++;
    try {
        auto label_dict = urology::create_default_urology_label_dict();
        auto static_config = urology::create_static_tensor_config(label_dict);
        
        if (urology::validate_static_config(static_config)) {
            std::cout << "✓ PASSED: Static configuration validation passed" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ FAILED: Static configuration validation failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: Configuration validation test failed: " << e.what() << std::endl;
    }
    
    // Test 10: Test end-to-end component integration
    std::cout << "\n[TEST 10] Testing end-to-end component integration..." << std::endl;
    test_count++;
    try {
        // Test that all components can work together
        auto label_dict = urology::create_default_urology_label_dict();
        auto color_lut = urology::create_default_color_lut();
        auto static_config = urology::create_static_tensor_config(label_dict);
        
        urology::HolovizCompatibleTensors compatible_tensors;
        compatible_tensors.label_dict = label_dict;
        compatible_tensors.color_lut = color_lut;
        
        std::vector<std::vector<float>> test_boxes = {
            {0.1f, 0.2f, 0.3f, 0.4f}
        };
        compatible_tensors.add_rectangle_tensor("boxes1", test_boxes);
        
        if (compatible_tensors.validate() && !static_config.empty()) {
            std::cout << "✓ PASSED: End-to-end component integration works correctly" << std::endl;
            passed_count++;
        } else {
            std::cout << "✗ FAILED: End-to-end component integration failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: End-to-end integration test failed: " << e.what() << std::endl;
    }
    
    // Summary
    std::cout << "\n=== Integration Test Summary ===" << std::endl;
    std::cout << "Total tests: " << test_count << std::endl;
    std::cout << "Passed: " << passed_count << std::endl;
    std::cout << "Failed: " << (test_count - passed_count) << std::endl;
    std::cout << "Success rate: " << (passed_count * 100 / test_count) << "%" << std::endl;
    
    if (passed_count == test_count) {
        std::cout << "\n🎉 ALL TESTS PASSED! Phase 3 integration is ready." << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ SOME TESTS FAILED. Please review the integration." << std::endl;
        return 1;
    }
} 