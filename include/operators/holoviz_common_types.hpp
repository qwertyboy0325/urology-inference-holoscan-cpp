#pragma once

#include <string>
#include <vector>
#include <map>

namespace urology {

/**
 * @brief Information about a label/class for visualization
 */
struct LabelInfo {
    std::string text;
    std::vector<float> color;
    float opacity;
    int line_width;
    
    LabelInfo() : opacity(0.7f), line_width(4) {}
    LabelInfo(const std::string& t, const std::vector<float>& c, 
              float o = 0.7f, int lw = 4) 
        : text(t), color(c), opacity(o), line_width(lw) {}
};

/**
 * @brief Detection result with bounding box, confidence score, and class ID
 */
struct Detection {
    std::vector<float> box;  // [x1, y1, x2, y2] normalized coordinates
    float confidence;
    int class_id;
    
    Detection() : confidence(0.0f), class_id(-1) {}
    Detection(const std::vector<float>& b, float conf, int cls_id)
        : box(b), confidence(conf), class_id(cls_id) {}
};

/**
 * @brief Color lookup table for visualization
 */
using ColorLUT = std::vector<std::vector<float>>;

/**
 * @brief Label dictionary mapping class IDs to label information
 */
using LabelDict = std::map<int, LabelInfo>;

} // namespace urology 