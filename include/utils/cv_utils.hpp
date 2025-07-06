#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

// Forward declarations for OpenCV types
namespace cv {
    class Mat;
    template<typename _Tp> class Scalar_;
    typedef Scalar_<double> Scalar;
}

// Forward declarations for GXF types
namespace nvidia {
    namespace gxf {
        class Tensor;
    }
}

namespace urology {
namespace cv_utils {

// OpenCV-based image processing utilities

// Convert between Holoscan tensors and OpenCV Mats
cv::Mat tensor_to_mat(const std::shared_ptr<nvidia::gxf::Tensor>& tensor);
std::shared_ptr<nvidia::gxf::Tensor> mat_to_tensor(const cv::Mat& mat, const std::string& name);

// Advanced image processing with OpenCV
cv::Mat resize_with_aspect_ratio(const cv::Mat& img, int target_width, int target_height);
cv::Mat apply_colormap(const cv::Mat& mask, const std::vector<int>& colors);

// Drawing utilities
void draw_boxes(cv::Mat& image, 
               const std::vector<std::vector<float>>& boxes,
               const std::vector<int>& class_ids,
               const std::vector<float>& scores,
               const std::vector<std::string>& class_names,
               const std::vector<cv::Scalar>& colors);

void draw_segmentation_masks(cv::Mat& image, 
                           const std::vector<std::vector<float>>& masks,
                           const std::vector<int>& class_ids,
                           const std::vector<cv::Scalar>& colors,
                           float alpha = 0.5f);

// Utility functions
std::vector<cv::Scalar> generate_colors(int num_classes);
cv::Mat create_overlay(const cv::Mat& image, const cv::Mat& mask, const cv::Scalar& color, float alpha = 0.5f);

// Image format conversion
cv::Mat bgr_to_rgb(const cv::Mat& bgr_image);
cv::Mat rgb_to_bgr(const cv::Mat& rgb_image);

// Mask overlay function for raw image data
void overlay_mask(std::vector<uint8_t>& image_data, int width, int height,
                 const std::vector<float>& mask, const std::vector<float>& color, float alpha = 0.5f);

// Simple image processing utilities (vector-based)

// Resize image using nearest neighbor interpolation
std::vector<uint8_t> resize_image(const std::vector<uint8_t>& image, 
                                  int src_width, int src_height,
                                  int dst_width, int dst_height);

// Normalize image pixels from [0, 255] to [0, 1]
std::vector<float> normalize_image(const std::vector<uint8_t>& image);

} // namespace cv_utils
} // namespace urology 