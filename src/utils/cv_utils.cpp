#include "holoscan_fix.hpp"
#include "utils/cv_utils.hpp"
#include <opencv2/opencv.hpp>
#include <gxf/std/tensor.hpp>
#include <vector>
#include <iostream>

namespace urology {
namespace cv_utils {

// Convert Holoscan tensor to OpenCV Mat
cv::Mat tensor_to_mat(const std::shared_ptr<nvidia::gxf::Tensor>& tensor) {
    if (!tensor) {
        return cv::Mat();
    }
    
    // TODO: Implement tensor to Mat conversion for GXF Tensor
    // This requires understanding the GXF Tensor API
    return cv::Mat();
}

// Convert OpenCV Mat to Holoscan tensor
std::shared_ptr<nvidia::gxf::Tensor> mat_to_tensor(const cv::Mat& mat, const std::string& name) {
    if (mat.empty()) {
        return nullptr;
    }
    
    // TODO: Implement Mat to tensor conversion for GXF Tensor
    // This requires understanding the GXF Tensor API
    return nullptr;
}

// Resize image while maintaining aspect ratio
cv::Mat resize_with_aspect_ratio(const cv::Mat& img, int target_width, int target_height) {
    if (img.empty()) {
        return cv::Mat();
    }
    
    double aspect_ratio = static_cast<double>(img.cols) / img.rows;
    double target_aspect = static_cast<double>(target_width) / target_height;
    
    int new_width, new_height;
    if (aspect_ratio > target_aspect) {
        new_width = target_width;
        new_height = static_cast<int>(target_width / aspect_ratio);
    } else {
        new_width = static_cast<int>(target_height * aspect_ratio);
        new_height = target_height;
    }
    
    cv::Mat resized;
    cv::resize(img, resized, cv::Size(new_width, new_height));
    
    // Add padding if necessary
    cv::Mat result = cv::Mat::zeros(target_height, target_width, img.type());
    int x_offset = (target_width - new_width) / 2;
    int y_offset = (target_height - new_height) / 2;
    
    resized.copyTo(result(cv::Rect(x_offset, y_offset, new_width, new_height)));
    return result;
}

// Apply colormap to mask
cv::Mat apply_colormap(const cv::Mat& mask, const std::vector<int>& colors) {
    cv::Mat colored_mask = cv::Mat::zeros(mask.size(), CV_8UC3);
    
    for (int y = 0; y < mask.rows; ++y) {
        for (int x = 0; x < mask.cols; ++x) {
            int class_id = mask.at<uchar>(y, x);
            if (class_id > 0 && class_id < colors.size()) {
                int color = colors[class_id];
                colored_mask.at<cv::Vec3b>(y, x) = cv::Vec3b(
                    color & 0xFF,
                    (color >> 8) & 0xFF,
                    (color >> 16) & 0xFF
                );
            }
        }
    }
    
    return colored_mask;
}

// Generate colors for different classes
std::vector<cv::Scalar> generate_colors(int num_classes) {
    std::vector<cv::Scalar> colors;
    for (int i = 0; i < num_classes; ++i) {
        int hue = (i * 180 / num_classes) % 180;
        cv::Mat hsv_color(1, 1, CV_8UC3, cv::Scalar(hue, 255, 255));
        cv::Mat bgr_color;
        cv::cvtColor(hsv_color, bgr_color, cv::COLOR_HSV2BGR);
        cv::Vec3b color_vec = bgr_color.at<cv::Vec3b>(0, 0);
        colors.emplace_back(color_vec[0], color_vec[1], color_vec[2]);
    }
    return colors;
}

// Draw bounding boxes
void draw_boxes(cv::Mat& image, 
               const std::vector<std::vector<float>>& boxes,
               const std::vector<int>& class_ids,
               const std::vector<float>& scores,
               const std::vector<std::string>& class_names,
               const std::vector<cv::Scalar>& colors) {
    
    for (size_t i = 0; i < boxes.size(); ++i) {
        const auto& box = boxes[i];
        cv::Rect rect(static_cast<int>(box[0]), static_cast<int>(box[1]),
                     static_cast<int>(box[2] - box[0]), static_cast<int>(box[3] - box[1]));
        
        cv::Scalar color = (class_ids[i] < colors.size() ? colors[class_ids[i]] : cv::Scalar(0, 255, 0));
        cv::rectangle(image, rect, color, 2);
        
        // Draw label
        std::string label = std::to_string(scores[i]) + " " + 
                           (class_ids[i] < class_names.size() ? class_names[class_ids[i]] : "Unknown");
        
        int baseline;
        cv::Size text_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        cv::Point text_origin(rect.x, rect.y - 5);
        
        cv::rectangle(image, text_origin + cv::Point(0, baseline),
                     text_origin + cv::Point(text_size.width, -text_size.height),
                     color, cv::FILLED);
        cv::putText(image, label, text_origin, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar::all(255), 1);
    }
}

// Draw segmentation masks
void draw_segmentation_masks(cv::Mat& image, 
                           const std::vector<std::vector<float>>& masks,
                           const std::vector<int>& class_ids,
                           const std::vector<cv::Scalar>& colors,
                           float alpha) {
    for (size_t i = 0; i < masks.size(); ++i) {
        const auto& mask = masks[i];
        cv::Scalar color = (class_ids[i] < colors.size() ? colors[class_ids[i]] : cv::Scalar(0, 255, 0));
        
        // Apply mask overlay
        cv::Mat mask_mat(image.rows, image.cols, CV_32F, const_cast<float*>(mask.data()));
        cv::Mat colored_mask;
        cv::applyColorMap(mask_mat * 255, colored_mask, cv::COLORMAP_JET);
        
        cv::addWeighted(image, 1.0 - alpha, colored_mask, alpha, 0, image);
    }
}

// Create overlay
cv::Mat create_overlay(const cv::Mat& image, const cv::Mat& mask, const cv::Scalar& color, float alpha) {
    cv::Mat overlay = image.clone();
    cv::Mat colored_mask = cv::Mat::zeros(image.size(), CV_8UC3);
    colored_mask.setTo(color, mask);
    cv::addWeighted(overlay, 1.0 - alpha, colored_mask, alpha, 0, overlay);
    return overlay;
}

// BGR to RGB conversion
cv::Mat bgr_to_rgb(const cv::Mat& bgr_image) {
    cv::Mat rgb_image;
    cv::cvtColor(bgr_image, rgb_image, cv::COLOR_BGR2RGB);
    return rgb_image;
}

// RGB to BGR conversion
cv::Mat rgb_to_bgr(const cv::Mat& rgb_image) {
    cv::Mat bgr_image;
    cv::cvtColor(rgb_image, bgr_image, cv::COLOR_RGB2BGR);
    return bgr_image;
}

// Overlay mask on raw image data
void overlay_mask(std::vector<uint8_t>& image_data, int width, int height,
                 const std::vector<float>& mask, const std::vector<float>& color, float alpha) {
    if (mask.size() != width * height) {
        std::cerr << "Mask size doesn't match image dimensions" << std::endl;
        return;
    }
    
    if (color.size() < 3) {
        std::cerr << "Color vector must have at least 3 components (RGB)" << std::endl;
        return;
    }
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            if (mask[idx] > 0.5f) { // Threshold for mask
                int pixel_idx = idx * 3; // Assuming RGB format
                if (pixel_idx + 2 < image_data.size()) {
                    image_data[pixel_idx] = static_cast<uint8_t>(alpha * color[0] * 255 + (1 - alpha) * image_data[pixel_idx]);
                    image_data[pixel_idx + 1] = static_cast<uint8_t>(alpha * color[1] * 255 + (1 - alpha) * image_data[pixel_idx + 1]);
                    image_data[pixel_idx + 2] = static_cast<uint8_t>(alpha * color[2] * 255 + (1 - alpha) * image_data[pixel_idx + 2]);
                }
            }
        }
    }
}

// Simple image processing utilities without OpenCV dependency
std::vector<uint8_t> resize_image(const std::vector<uint8_t>& image, 
                                  int src_width, int src_height,
                                  int dst_width, int dst_height) {
    std::vector<uint8_t> resized(dst_width * dst_height * 3);
    
    // Simple nearest neighbor interpolation
    float x_ratio = static_cast<float>(src_width) / dst_width;
    float y_ratio = static_cast<float>(src_height) / dst_height;
    
    for (int y = 0; y < dst_height; ++y) {
        for (int x = 0; x < dst_width; ++x) {
            int src_x = static_cast<int>(x * x_ratio);
            int src_y = static_cast<int>(y * y_ratio);
            
            // Clamp to source image bounds
            src_x = std::min(src_x, src_width - 1);
            src_y = std::min(src_y, src_height - 1);
            
            int src_idx = (src_y * src_width + src_x) * 3;
            int dst_idx = (y * dst_width + x) * 3;
            
            resized[dst_idx] = image[src_idx];         // R
            resized[dst_idx + 1] = image[src_idx + 1]; // G
            resized[dst_idx + 2] = image[src_idx + 2]; // B
        }
    }
    
    return resized;
}

// Normalize image pixels
std::vector<float> normalize_image(const std::vector<uint8_t>& image) {
    std::vector<float> normalized(image.size());
    for (size_t i = 0; i < image.size(); ++i) {
        normalized[i] = static_cast<float>(image[i]) / 255.0f;
    }
    return normalized;
}

} // namespace cv_utils
} // namespace urology 