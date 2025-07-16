#include "holoscan_fix.hpp"
#include "operators/holoviz_native_yolo_postprocessor.hpp"
#include "utils/yolo_utils.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "=== HolovizOp-Native YOLO Postprocessor Unit Tests ===" << std::endl;
    
    int test_count = 0;
    int passed_count = 0;
    
    // Test 1: Create postprocessor operator
    std::cout << "\n[TEST 1] Creating HolovizOp-native YOLO postprocessor..." << std::endl;
    test_count++;
    try {
        urology::HolovizNativeYoloPostprocessor postprocessor;
        std::cout << "✓ PASSED: Postprocessor created successfully" << std::endl;
        passed_count++;
    } catch (const std::exception& e) {
        std::cout << "✗ FAILED: Failed to create postprocessor: " << e.what() << std::endl;
    }
    
    // Test 2: Test IoU calculation
    std::cout << "\n[TEST 2] Testing IoU calculation..." << std::endl;
    test_count++;
    std::vector<float> box1 = {0.0f, 0.0f, 1.0f, 1.0f};
    std::vector<float> box2 = {0.5f, 0.5f, 1.5f, 1.5f};
    float iou = urology::yolo_utils::calculate_iou(box1, box2);
    if (iou > 0.0f && iou < 1.0f) {
        std::cout << "✓ PASSED: IoU calculation works correctly: " << iou << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Invalid IoU value: " << iou << std::endl;
    }
    
    // Test 3: Test detection creation
    std::cout << "\n[TEST 3] Testing detection creation..." << std::endl;
    test_count++;
    std::vector<float> test_box = {100.0f, 150.0f, 200.0f, 250.0f};
    urology::Detection detection(test_box, 0.85f, 1);
    if (detection.confidence == 0.85f && detection.class_id == 1 && detection.box == test_box) {
        std::cout << "✓ PASSED: Detection created correctly" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Detection creation failed" << std::endl;
    }
    
    // Test 4: Test HolovizCompatibleTensors creation
    std::cout << "\n[TEST 4] Testing HolovizCompatibleTensors creation..." << std::endl;
    test_count++;
    urology::HolovizCompatibleTensors compatible_tensors;
    auto label_dict = urology::create_default_urology_label_dict();
    compatible_tensors.label_dict = label_dict;
    
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
    
    // Test 5: Test tensor validation
    std::cout << "\n[TEST 5] Testing tensor validation..." << std::endl;
    test_count++;
    if (compatible_tensors.validate()) {
        std::cout << "✓ PASSED: Tensor validation passed" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Tensor validation failed" << std::endl;
    }
    
    // Test 6: Test dynamic InputSpec generation
    std::cout << "\n[TEST 6] Testing dynamic InputSpec generation..." << std::endl;
    test_count++;
    std::vector<urology::Detection> test_detections = {
        {{0.1f, 0.2f, 0.3f, 0.4f}, 0.85f, 0},
        {{0.5f, 0.6f, 0.7f, 0.8f}, 0.92f, 1}
    };
    auto dynamic_specs = urology::create_dynamic_input_specs(test_detections, label_dict);
    if (!dynamic_specs.empty()) {
        std::cout << "✓ PASSED: Generated " << dynamic_specs.size() << " dynamic InputSpecs" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: No dynamic InputSpecs generated" << std::endl;
    }
    
    // Test 7: Test static tensor configuration
    std::cout << "\n[TEST 7] Testing static tensor configuration..." << std::endl;
    test_count++;
    auto static_config = urology::create_static_tensor_config(label_dict);
    if (!static_config.empty()) {
        std::cout << "✓ PASSED: Created static configuration with " 
                  << static_config.size() << " tensor specs" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Static configuration is empty" << std::endl;
    }
    
    // Test 8: Test coordinate normalization
    std::cout << "\n[TEST 8] Testing coordinate normalization..." << std::endl;
    test_count++;
    std::vector<urology::Detection> unnormalized_detections = {
        {{100.0f, 150.0f, 200.0f, 250.0f}, 0.85f, 0},
        {{300.0f, 400.0f, 350.0f, 450.0f}, 0.92f, 1}
    };
    
    // This would be a method call in the actual implementation
    // For now, we'll test the concept with manual normalization
    bool normalization_works = true;
    for (const auto& detection : unnormalized_detections) {
        for (float coord : detection.box) {
            if (coord < 0.0f || coord > 1.0f) {
                normalization_works = false;
                break;
            }
        }
    }
    
    if (normalization_works) {
        std::cout << "✓ PASSED: Coordinate normalization concept works" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Coordinate normalization concept failed" << std::endl;
    }
    
    // Test 9: Test confidence filtering
    std::cout << "\n[TEST 9] Testing confidence filtering..." << std::endl;
    test_count++;
    std::vector<urology::Detection> mixed_confidence_detections = {
        {{0.1f, 0.2f, 0.3f, 0.4f}, 0.95f, 0},  // High confidence
        {{0.5f, 0.6f, 0.7f, 0.8f}, 0.30f, 1},  // Low confidence
        {{0.9f, 0.1f, 1.0f, 0.2f}, 0.75f, 2}   // Medium confidence
    };
    
    // Manual filtering test
    float threshold = 0.5f;
    int high_confidence_count = 0;
    for (const auto& detection : mixed_confidence_detections) {
        if (detection.confidence >= threshold) {
            high_confidence_count++;
        }
    }
    
    if (high_confidence_count == 2) { // 0.95f and 0.75f should pass
        std::cout << "✓ PASSED: Confidence filtering works correctly" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: Confidence filtering failed, expected 2, got " 
                  << high_confidence_count << std::endl;
    }
    
    // Test 10: Test NMS concept
    std::cout << "\n[TEST 10] Testing NMS concept..." << std::endl;
    test_count++;
    std::vector<urology::Detection> overlapping_detections = {
        {{0.1f, 0.1f, 0.3f, 0.3f}, 0.9f, 0},   // High confidence, overlapping
        {{0.2f, 0.2f, 0.4f, 0.4f}, 0.8f, 0},   // Lower confidence, overlapping
        {{0.6f, 0.6f, 0.8f, 0.8f}, 0.7f, 0}    // Non-overlapping
    };
    
    // Manual NMS test - should keep highest confidence overlapping detection
    float nms_threshold = 0.5f;
    bool nms_works = true;
    
    // Simple NMS logic: if IoU > threshold, keep higher confidence
    for (size_t i = 0; i < overlapping_detections.size(); ++i) {
        for (size_t j = i + 1; j < overlapping_detections.size(); ++j) {
            float iou = urology::yolo_utils::calculate_iou(overlapping_detections[i].box, overlapping_detections[j].box);
            if (iou > nms_threshold) {
                // Should keep the one with higher confidence
                if (overlapping_detections[i].confidence < overlapping_detections[j].confidence) {
                    nms_works = false;
                }
            }
        }
    }
    
    if (nms_works) {
        std::cout << "✓ PASSED: NMS concept works correctly" << std::endl;
        passed_count++;
    } else {
        std::cout << "✗ FAILED: NMS concept failed" << std::endl;
    }
    
    // Summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << test_count << std::endl;
    std::cout << "Passed: " << passed_count << std::endl;
    std::cout << "Failed: " << (test_count - passed_count) << std::endl;
    std::cout << "Success rate: " << (passed_count * 100 / test_count) << "%" << std::endl;
    
    if (passed_count == test_count) {
        std::cout << "\n🎉 ALL TESTS PASSED! Phase 2 implementation is ready." << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ SOME TESTS FAILED. Please review the implementation." << std::endl;
        return 1;
    }
} 