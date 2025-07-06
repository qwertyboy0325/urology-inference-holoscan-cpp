#!/bin/bash

# Run script for Urology Inference Holoscan C++
# Usage: ./scripts/run.sh [options]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
DATA_DIR="$PROJECT_DIR/data"

# Configuration
DATA_PATH="${DATA_PATH:-./data}"
SOURCE_TYPE="${SOURCE_TYPE:-replayer}"
CONFIG_FILE="${CONFIG_FILE:-./src/config/app_config.yaml}"
LABELS_FILE="${LABELS_FILE:-./data/config/surgical_tool_labels.yaml}"
LOG_LEVEL="${LOG_LEVEL:-INFO}"
HEADLESS_MODE="${HEADLESS_MODE:-false}"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --data)
            DATA_PATH="$2"
            shift 2
            ;;
        --source)
            SOURCE_TYPE="$2"
            shift 2
            ;;
        --config)
            CONFIG_FILE="$2"
            shift 2
            ;;
        --labels)
            LABELS_FILE="$2"
            shift 2
            ;;
        --log-level)
            LOG_LEVEL="$2"
            shift 2
            ;;
        --headless)
            HEADLESS_MODE="true"
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --data PATH        Data directory path (default: ./data)"
            echo "  --source TYPE      Source type: replayer|yuan (default: replayer)"
            echo "  --config FILE      Configuration file path (default: ./src/config/app_config.yaml)"
            echo "  --labels FILE      Labels file path (default: ./data/config/surgical_tool_labels.yaml)"
            echo "  --log-level LEVEL  Log level: TRACE|DEBUG|INFO|WARN|ERROR|CRITICAL (default: INFO)"
            echo "  --headless         Run without display window (headless mode)"
            echo "  --help, -h         Show this help message"
            echo ""
            echo "Environment Variables:"
            echo "  HOLOVIZ_HEADLESS   Set to any value to enable headless mode"
            echo "  DATA_PATH          Override data directory path"
            echo "  SOURCE_TYPE        Override source type"
            echo "  LOG_LEVEL          Override log level"
            echo ""
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}Urology Inference Holoscan C++ Runner${NC}"
echo "======================================"

# Check if executable exists
EXECUTABLE="$BUILD_DIR/urology_inference_holoscan_cpp"
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}Error: Executable not found at $EXECUTABLE${NC}"
    echo "Please build the application first:"
    echo "  $SCRIPT_DIR/build.sh"
    exit 1
fi

# Check data directory
if [ ! -d "$DATA_PATH" ]; then
    echo -e "${YELLOW}Warning: Data directory not found: $DATA_PATH${NC}"
    echo "Creating data directory structure..."
    mkdir -p "$DATA_PATH"/{models,inputs,output}
fi

# Check model file
MODEL_FILE="$DATA_PATH/models/Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx"
if [ ! -f "$MODEL_FILE" ]; then
    echo -e "${YELLOW}Warning: Model file not found: $MODEL_FILE${NC}"
    echo "Please ensure the model file is in the correct location"
fi

# Check input data for replayer mode
if [ "$SOURCE_TYPE" = "replayer" ]; then
    INPUT_DIR="$DATA_PATH/inputs"
    if [ ! -d "$INPUT_DIR" ] || [ -z "$(ls -A "$INPUT_DIR" 2>/dev/null)" ]; then
        echo -e "${YELLOW}Warning: No input data found in $INPUT_DIR${NC}"
        echo "For replayer mode, please place video files in the inputs directory"
    fi
fi

# Set environment variables
export HOLOHUB_DATA_PATH="$DATA_PATH"
export HOLOSCAN_LOG_LEVEL="$LOG_LEVEL"

# Set headless mode if requested
if [ "$HEADLESS_MODE" = "true" ]; then
    export HOLOVIZ_HEADLESS=1
    echo -e "${YELLOW}Running in headless mode - no display window will be shown${NC}"
fi

# Display runtime information
echo ""
echo -e "${GREEN}Runtime Configuration:${NC}"
echo "  Executable: $EXECUTABLE"
echo "  Data path: $DATA_PATH"
echo "  Source type: $SOURCE_TYPE"
echo "  Config file: $CONFIG_FILE"
echo "  Labels file: $LABELS_FILE"
echo "  Log level: $LOG_LEVEL"
echo ""

# Check GPU availability
if command -v nvidia-smi &> /dev/null; then
    echo -e "${GREEN}GPU Information:${NC}"
    nvidia-smi --query-gpu=name,memory.total,memory.free --format=csv,noheader,nounits | head -1
    echo ""
else
    echo -e "${YELLOW}Warning: nvidia-smi not found. GPU may not be available.${NC}"
fi

# Run the application
echo -e "${BLUE}Starting application...${NC}"
echo "==============================="

# Add any necessary LD_LIBRARY_PATH
if [ -n "$HOLOSCAN_INSTALL_PATH" ]; then
    export LD_LIBRARY_PATH="$HOLOSCAN_INSTALL_PATH/lib:$LD_LIBRARY_PATH"
fi

# Execute the application
exec "$EXECUTABLE" \
    --data "$DATA_PATH" \
    --source "$SOURCE_TYPE" \
    --labels "$LABELS_FILE" 