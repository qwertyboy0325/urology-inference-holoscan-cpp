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

# Default parameters
SOURCE_TYPE="replayer"
DATA_PATH="$DATA_DIR"
CONFIG_FILE="$PROJECT_DIR/src/config/app_config.yaml"
LABELS_FILE="$PROJECT_DIR/src/config/labels.yaml"
LOG_LEVEL="INFO"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--data)
            DATA_PATH="$2"
            shift 2
            ;;
        -s|--source)
            SOURCE_TYPE="$2"
            shift 2
            ;;
        -c|--config)
            CONFIG_FILE="$2"
            shift 2
            ;;
        -l|--labels)
            LABELS_FILE="$2"
            shift 2
            ;;
        --log-level)
            LOG_LEVEL="$2"
            shift 2
            ;;
        --build)
            echo -e "${YELLOW}Building first...${NC}"
            "$SCRIPT_DIR/build.sh"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  -d, --data PATH       Data directory path (default: $DATA_DIR)"
            echo "  -s, --source TYPE     Source type: replayer/yuan (default: replayer)"
            echo "  -c, --config FILE     Configuration file (default: app_config.yaml)"
            echo "  -l, --labels FILE     Labels file (default: labels.yaml)"
            echo "  --log-level LEVEL     Log level: TRACE/DEBUG/INFO/WARN/ERROR (default: INFO)"
            echo "  --build               Build before running"
            echo "  -h, --help            Show this help"
            echo ""
            echo "Examples:"
            echo "  $0                                    # Run with defaults"
            echo "  $0 -d /path/to/data -s yuan          # Use YUAN capture card"
            echo "  $0 --build --log-level DEBUG         # Build and run with debug logs"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use -h or --help for usage information"
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