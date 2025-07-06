#pragma once

#include <holoscan/holoscan.hpp>
#include <holoscan/core/gxf/entity.hpp>
#include <holoscan/core/condition.hpp>

namespace urology {

/**
 * @brief GXF-safe passthrough operator that prevents entity blocking
 * Based on Holoscan SDK documentation Q14 recommendations
 */
class GXFSafePassthroughOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(GXFSafePassthroughOp)

    GXFSafePassthroughOp() = default;

    void setup(holoscan::OperatorSpec& spec) override {
        spec.input<holoscan::gxf::Entity>("input");
        spec.output<holoscan::gxf::Entity>("output");
        
        // 核心參數：控制暫停行為
        spec.param(enabled_, "enabled", "Enable passthrough", true);
        spec.param(pause_mode_, "pause_mode", "Pause mode: drop/queue/stop", std::string("drop"));
        spec.param(max_queue_size_, "max_queue_size", "Max queue size", 10);
        
        // GXF 狀態控制：使用 BooleanCondition 但不停止 compute()
        auto boolean_condition = std::make_shared<holoscan::BooleanCondition>();
        boolean_condition->enable_tick(); // 始終保持 READY 狀態
        spec.condition(boolean_condition);
        boolean_condition_ = boolean_condition;
    }

    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext& context) override {
        
        // 關鍵：ALWAYS 接收輸入，防止上游 entity 進入 WAIT 狀態
        auto input_entity = op_input.receive<holoscan::gxf::Entity>("input");
        if (!input_entity) {
            // 沒有輸入時，Entity 自然進入 WAIT 狀態（正常）
            return;
        }

        if (enabled_) {
            // 正常模式：直接轉發
            handle_normal_mode(input_entity.value(), op_output);
        } else {
            // 暫停模式：根據策略處理
            handle_pause_mode(input_entity.value(), op_output);
        }
        
        // 處理佇列中的 entities
        process_queued_entities(op_output);
    }

    // 外部控制介面
    void set_enabled(bool enabled) {
        enabled_ = enabled;
        if (enabled) {
            HOLOSCAN_LOG_INFO("GXFSafePassthrough: ENABLED - processing resumed");
        } else {
            HOLOSCAN_LOG_INFO("GXFSafePassthrough: DISABLED - pause mode: {}", pause_mode_);
        }
    }
    
    void set_pause_mode(const std::string& mode) {
        pause_mode_ = mode;
        HOLOSCAN_LOG_INFO("GXFSafePassthrough: Pause mode set to {}", mode);
    }
    
    bool is_enabled() const { return enabled_; }
    size_t queue_size() const { return entity_queue_.size(); }

private:
    void handle_normal_mode(const holoscan::gxf::Entity& entity, 
                           holoscan::OutputContext& op_output) {
        // 直接轉發，保持 entity 在 READY->EXECUTE 流程
        op_output.emit(entity, "output");
    }
    
    void handle_pause_mode(const holoscan::gxf::Entity& entity, 
                          holoscan::OutputContext& op_output) {
        if (pause_mode_ == "drop") {
            // 丟棄模式：entity 被消費但不轉發
            // 這樣上游 entity 完成了 EXECUTE，不會堵塞
            HOLOSCAN_LOG_DEBUG("Dropping entity in pause mode");
            
        } else if (pause_mode_ == "queue") {
            // 佇列模式：暫存 entity 供後續處理
            if (entity_queue_.size() < max_queue_size_) {
                entity_queue_.push(entity);
                HOLOSCAN_LOG_DEBUG("Queued entity - total: {}", entity_queue_.size());
            } else {
                HOLOSCAN_LOG_WARN("Queue full, dropping entity");
            }
            
        } else if (pause_mode_ == "freeze") {
            // 凍結模式：重複發送最後一個 entity
            if (!last_entity_) {
                last_entity_ = entity;
            }
            op_output.emit(last_entity_.value(), "output");
        }
    }
    
    void process_queued_entities(holoscan::OutputContext& op_output) {
        // 只在啟用時處理佇列
        if (!enabled_ || entity_queue_.empty()) {
            return;
        }
        
        // 每次只處理一個，避免爆發式輸出
        auto entity = entity_queue_.front();
        entity_queue_.pop();
        op_output.emit(entity, "output");
        
        HOLOSCAN_LOG_DEBUG("Processed queued entity - remaining: {}", entity_queue_.size());
    }

    // 參數
    holoscan::Parameter<bool> enabled_;
    holoscan::Parameter<std::string> pause_mode_;
    holoscan::Parameter<int> max_queue_size_;
    
    // 狀態管理
    std::shared_ptr<holoscan::BooleanCondition> boolean_condition_;
    std::queue<holoscan::gxf::Entity> entity_queue_;
    std::optional<holoscan::gxf::Entity> last_entity_;
};

/**
 * @brief Entity 狀態監控器
 * 用於調試和監控 GXF entity 狀態
 */
class EntityStateMonitorOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(EntityStateMonitorOp)
    
    EntityStateMonitorOp() = default;
    
    void setup(holoscan::OperatorSpec& spec) override {
        spec.input<holoscan::gxf::Entity>("input");
        spec.output<holoscan::gxf::Entity>("output");
        spec.param(log_interval_, "log_interval", "Log interval (entities)", 30);
    }
    
    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext& context) override {
        
        auto start_time = std::chrono::steady_clock::now();
        
        auto input_entity = op_input.receive<holoscan::gxf::Entity>("input");
        if (!input_entity) {
            HOLOSCAN_LOG_DEBUG("EntityStateMonitor: No input - Entity in WAIT state");
            return;
        }
        
        entity_count_++;
        
        // 定期記錄狀態
        if (entity_count_ % log_interval_ == 0) {
            auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            
            HOLOSCAN_LOG_INFO("EntityStateMonitor: Processed {} entities, "
                            "current processing time: {}μs", 
                            entity_count_, processing_time);
        }
        
        // 轉發 entity，保持流程正常
        op_output.emit(input_entity.value(), "output");
    }

private:
    holoscan::Parameter<int> log_interval_;
    uint64_t entity_count_{0};
};

} // namespace urology 