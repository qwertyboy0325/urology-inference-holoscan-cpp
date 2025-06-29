#include <holoscan/holoscan.hpp>

namespace urology {

class DummyReceiverOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(DummyReceiverOp)

    DummyReceiverOp() = default;

    void setup(holoscan::OperatorSpec& spec) override {
        spec.input("in");
    }

    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext& context) override {
        // Simply receive and discard the input
        auto in_message = op_input.receive("in");
        // Do nothing with the message
    }
};

} // namespace urology 