#include "holoscan_fix.hpp"

namespace urology {

class PassthroughOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(PassthroughOp)

    PassthroughOp() = default;

    void setup(holoscan::OperatorSpec& spec) override {
        spec.input<holoscan::Tensor>("in");
        spec.output<holoscan::Tensor>("out");
        spec.param(enabled_, "enabled", "Enable passthrough");
    }

    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext&) override {
        if (!enabled_) return;
        
        auto in_message = op_input.receive<holoscan::Tensor>("in");
        if (in_message) {
            op_output.emit(in_message.value(), "out");
        }
    }

    void set_enabled(bool enabled) {
        enabled_ = enabled;
    }

private:
    holoscan::Parameter<bool> enabled_;
};

} // namespace urology 