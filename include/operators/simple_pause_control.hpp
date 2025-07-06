#pragma once

#include <holoscan/holoscan.hpp>
#include <holoscan/core/condition.hpp>

namespace urology {

/**
 * @brief Simple pause control using native Holoscan BooleanCondition
 * This is much simpler than custom passthrough for pause functionality
 */
class PauseController {
public:
    PauseController() 
        : condition_(std::make_shared<holoscan::BooleanCondition>()), 
          is_paused_(false) {
        condition_->enable_tick(); // Start enabled
    }
    
    void pause() {
        if (!is_paused_) {
            condition_->disable_tick();
            is_paused_ = true;
        }
    }
    
    void resume() {
        if (is_paused_) {
            condition_->enable_tick();
            is_paused_ = false;
        }
    }
    
    void toggle() {
        if (is_paused_) {
            resume();
        } else {
            pause();
        }
    }
    
    bool is_paused() const { return is_paused_; }
    
    std::shared_ptr<holoscan::BooleanCondition> get_condition() { 
        return condition_; 
    }

private:
    std::shared_ptr<holoscan::BooleanCondition> condition_;
    bool is_paused_;
};

} // namespace urology 