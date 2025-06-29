#include <holoscan/holoscan.hpp>

namespace urology {

class PassthroughOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(PassthroughOp)

    PassthroughOp() = default;

    void setup(holoscan::OperatorSpec& spec) override {
        spec.input("in");
        spec.output("out");
        spec.param("enabled", enabled_, "Enable passthrough", 
                   "Whether to pass through data", true);
    }

    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext& context) override {
        auto in_message = op_input.receive("in");
        
        if (enabled_) {
            op_output.emit(in_message, "out");
        }
        // If not enabled, simply discard the message
    }

    void set_enabled(bool enabled) {
        enabled_ = enabled;
    }

private:
    bool enabled_ = true;
};

} // namespace urology 