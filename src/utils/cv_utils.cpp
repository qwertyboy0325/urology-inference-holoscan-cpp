#include <opencv2/opencv.hpp>
#include <holoscan/core/tensor.hpp>
#include <vector>
#include <iostream>

namespace urology {
namespace cv_utils {

// Convert Holoscan tensor to OpenCV Mat
cv::Mat tensor_to_mat(const std::shared_ptr<holoscan::Tensor>& tensor) {
    if (!tensor) {
        return cv::Mat();
    }
    
    auto shape = tensor->shape();
    const void* data = tensor->data();
    
    // Assuming CHW format for now
    if (shape.size() == 3) {
        int channels = shape[0];
        int height = shape[1];
        int width = shape[2];
        
        // Convert to HWC format
        cv::Mat mat(height, width, CV_32FC3, const_cast<void*>(data));
        return mat;
    } else if (shape.size() == 2) {
        int height = shape[0];
        int width = shape[1];
        
        cv::Mat mat(height, width, CV_32F, const_cast<void*>(data));
        return mat;
    }
    
    return cv::Mat();
}

// Convert OpenCV Mat to Holoscan tensor
std::shared_ptr<holoscan::Tensor> mat_to_tensor(const cv::Mat& mat, const std::string& name) {
    if (mat.empty()) {
        return nullptr;
    }
    
    // Create tensor spec based on Mat properties
    holoscan::TensorSpec spec(name, holoscan::nvidia::gxf::PrimitiveType::kFloat32);
    
    auto tensor = std::make_shared<holoscan::Tensor>(spec);
    
    // TODO: Copy data from Mat to tensor
    // This requires proper memory management
    
    return tensor;
}

// Resize image while maintaining aspect ratio
cv::Mat resize_with_aspect_ratio(const cv::Mat& img, int target_width, int target_height) {
    if (img.empty()) {
        return cv::Mat();
    }
    
    float scale = std::min(
        static_cast<float>(target_width) / img.cols,
        static_cast<float>(target_height) / img.rows
    );
    
    int new_width = static_cast<int>(img.cols * scale);
    int new_height = static_cast<int>(img.rows * scale);
    
    cv::Mat resized;
    cv::resize(img, resized, cv::Size(new_width, new_height));
    
    // Create padded image
    cv::Mat padded = cv::Mat::zeros(target_height, target_width, img.type());
    
    int x_offset = (target_width - new_width) / 2;
    int y_offset = (target_height - new_height) / 2;
    
    resized.copyTo(padded(cv::Rect(x_offset, y_offset, new_width, new_height)));
    
    return padded;
}

// Apply color map to mask
cv::Mat apply_colormap(const cv::Mat& mask, const std::vector<cv::Vec3b>& colors) {
    if (mask.empty()) {
        return cv::Mat();
    }
    
    cv::Mat colored(mask.size(), CV_8UC3);
    
    for (int y = 0; y < mask.rows; ++y) {
        for (int x = 0; x < mask.cols; ++x) {
            int class_id = mask.at<uchar>(y, x);
            if (class_id < colors.size()) {
                colored.at<cv::Vec3b>(y, x) = colors[class_id];
            }
        }
    }
    
    return colored;
}

// Draw bounding boxes on image
void draw_boxes(cv::Mat& img, 
               const std::vector<std::vector<float>>& boxes,
               const std::vector<int>& class_ids,
               const std::vector<float>& scores,
               const std::vector<std::string>& class_names,
               const std::vector<cv::Scalar>& colors) {
    
    for (size_t i = 0; i < boxes.size(); ++i) {
        if (i >= class_ids.size() || i >= scores.size()) break;
        
        const auto& box = boxes[i];
        if (box.size() < 4) continue;
        
        int x1 = static_cast<int>(box[0]);
        int y1 = static_cast<int>(box[1]);
        int x2 = static_cast<int>(box[2]);
        int y2 = static_cast<int>(box[3]);
        
        cv::Scalar color = colors.empty() ? cv::Scalar(0, 255, 0) : 
                          (class_ids[i] < colors.size() ? colors[class_ids[i]] : cv::Scalar(0, 255, 0));
        
        // Draw bounding box
        cv::rectangle(img, cv::Point(x1, y1), cv::Point(x2, y2), color, 2);
        
        // Draw label
        std::string label = class_names.empty() ? 
                           std::to_string(class_ids[i]) : 
                           (class_ids[i] < class_names.size() ? class_names[class_ids[i]] : "Unknown");
        
        label += " " + std::to_string(scores[i]).substr(0, 4);
        
        int baseline;
        cv::Size text_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        
        cv::rectangle(img, 
                     cv::Point(x1, y1 - text_size.height - baseline),
                     cv::Point(x1 + text_size.width, y1),
                     color, -1);
        
        cv::putText(img, label, cv::Point(x1, y1 - baseline),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    }
}

} // namespace cv_utils
} // namespace urology 