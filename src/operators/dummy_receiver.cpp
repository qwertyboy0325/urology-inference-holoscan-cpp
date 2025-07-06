#include "holoscan_fix.hpp"

namespace urology {

class DummyReceiverOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(DummyReceiverOp)

    DummyReceiverOp() = default;

    void setup(holoscan::OperatorSpec& spec) override {
        spec.input<holoscan::Tensor>("in");
    }

    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext& context) override {
        // Simply receive and discard the input
        auto in_message = op_input.receive<holoscan::Tensor>("in");
        // Do nothing with the message
    }
};

} // namespace urology 