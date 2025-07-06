#!/bin/bash

# Video to GXF Conversion Script for Urology Inference
# Converts standard video files (MP4, AVI, MOV) to Holoscan GXF format

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
INPUT_DIR="$PROJECT_DIR/data/videos"
OUTPUT_DIR="$PROJECT_DIR/data/inputs"
WIDTH=1920
HEIGHT=1080
CHANNELS=3
BASENAME="tensor"
HOLOSCAN_CONTAINER="nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu"
CONVERSION_SCRIPT="/opt/nvidia/holoscan/bin/convert_video_to_gxf_entities.py"

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS] [VIDEO_FILE]"
    echo ""
    echo "Convert video files to Holoscan GXF format for the Urology Inference application."
    echo ""
    echo "Options:"
    echo "  -i, --input-dir DIR     Input directory containing videos (default: data/videos)"
    echo "  -o, --output-dir DIR    Output directory for GXF files (default: data/inputs)"
    echo "  -w, --width WIDTH       Video width (default: 1920)"
    echo "  -h, --height HEIGHT     Video height (default: 1080)"
    echo "  -c, --channels CHANNELS Number of color channels (default: 3)"
    echo "  -b, --basename NAME     Output filename prefix (default: tensor)"
    echo "  --container IMAGE       Holoscan container image (default: latest)"
    echo "  --batch                 Convert all videos in input directory"
    echo "  --help                  Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 my_video.mp4                    # Convert single video"
    echo "  $0 --batch                         # Convert all videos in data/videos/"
    echo "  $0 -w 1280 -h 720 video.mp4       # Convert with custom resolution"
    echo "  $0 -b my_tensor video.mp4          # Convert with custom basename"
    echo ""
    echo "Supported input formats: MP4, AVI, MOV, MKV"
    echo "Output format: GXF entities (.gxf_entities, .gxf_index)"
}

# Function to check dependencies
check_dependencies() {
    print_info "Checking dependencies..."
    
    # Check if Docker is available
    if ! command -v docker &> /dev/null; then
        print_error "Docker is not installed or not in PATH"
        exit 1
    fi
    
    # Check if Docker daemon is running
    if ! docker info &> /dev/null; then
        print_error "Docker daemon is not running"
        exit 1
    fi
    
    print_success "Dependencies check passed"
}

# Function to validate video file
validate_video() {
    local video_file="$1"
    
    if [ ! -f "$video_file" ]; then
        print_error "Video file not found: $video_file"
        return 1
    fi
    
    # Check file extension
    local ext="${video_file##*.}"
    ext=$(echo "$ext" | tr '[:upper:]' '[:lower:]')
    
    case "$ext" in
        mp4|avi|mov|mkv)
            print_info "Video format: $ext (supported)"
            return 0
            ;;
        *)
            print_warning "Video format: $ext (may not be supported)"
            return 0
            ;;
    esac
}

# Function to get video properties
get_video_info() {
    local video_file="$1"
    
    print_info "Getting video information..."
    
    # Use ffprobe to get video properties
    if command -v ffprobe &> /dev/null; then
        echo "Video properties:"
        ffprobe -v quiet -select_streams v:0 -show_entries stream=width,height,r_frame_rate,duration -of csv=p=0 "$video_file" | \
        while IFS=',' read -r width height fps duration; do
            echo "  Resolution: ${width}x${height}"
            echo "  Frame rate: $fps"
            echo "  Duration: $duration seconds"
        done
    else
        print_warning "ffprobe not available, skipping video analysis"
    fi
}

# Function to convert single video
convert_video() {
    local video_file="$1"
    local output_basename="$2"
    
    print_info "Converting video: $video_file"
    print_info "Output basename: $output_basename"
    print_info "Output directory: $OUTPUT_DIR"
    print_info "Resolution: ${WIDTH}x${HEIGHT}"
    print_info "Channels: $CHANNELS"
    
    # Create output directory
    mkdir -p "$OUTPUT_DIR"
    
    # Get absolute paths for Docker mounting
    local abs_video_file=$(realpath "$video_file")
    local abs_output_dir=$(realpath "$OUTPUT_DIR")
    local video_dir=$(dirname "$abs_video_file")
    local video_name=$(basename "$abs_video_file")
    
    # Run conversion in Docker container
    print_info "Starting Docker container for conversion..."
    
    docker run -it --rm \
        -v "$video_dir":/workspace/input:ro \
        -v "$abs_output_dir":/workspace/output \
        "$HOLOSCAN_CONTAINER" \
        bash -c "
            set -e
            cd /workspace
            echo 'Installing ffmpeg...'
            apt-get update && apt-get install -y ffmpeg
            
            echo 'Converting video with ffmpeg and GXF conversion script...'
            ffmpeg -i input/$video_name -pix_fmt rgb24 -f rawvideo pipe:1 | \
            python3 $CONVERSION_SCRIPT \
                --width $WIDTH \
                --height $HEIGHT \
                --channels $CHANNELS \
                --directory output \
                --basename $output_basename
            
            echo 'Conversion completed successfully!'
            echo 'Generated files:'
            ls -la output/${output_basename}.*
        "
    
    if [ $? -eq 0 ]; then
        print_success "Video converted successfully!"
        print_info "Generated files:"
        ls -la "$OUTPUT_DIR/${output_basename}".* 2>/dev/null || print_warning "Output files not found"
    else
        print_error "Video conversion failed"
        return 1
    fi
}

# Function to convert all videos in directory
convert_batch() {
    print_info "Converting all videos in: $INPUT_DIR"
    
    if [ ! -d "$INPUT_DIR" ]; then
        print_error "Input directory not found: $INPUT_DIR"
        exit 1
    fi
    
    local video_count=0
    local success_count=0
    
    # Process all video files
    for video_file in "$INPUT_DIR"/*.{mp4,avi,mov,mkv,MP4,AVI,MOV,MKV}; do
        if [ -f "$video_file" ]; then
            video_count=$((video_count + 1))
            
            # Generate basename from filename
            local basename=$(basename "$video_file" | sed 's/\.[^.]*$//')
            
            print_info "Processing video $video_count: $video_file"
            
            if validate_video "$video_file" && convert_video "$video_file" "$basename"; then
                success_count=$((success_count + 1))
            else
                print_error "Failed to convert: $video_file"
            fi
            
            echo "----------------------------------------"
        fi
    done
    
    if [ $video_count -eq 0 ]; then
        print_warning "No video files found in: $INPUT_DIR"
        print_info "Supported formats: MP4, AVI, MOV, MKV"
    else
        print_success "Batch conversion completed: $success_count/$video_count videos converted"
    fi
}

# Main script logic
main() {
    local batch_mode=false
    local video_file=""
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -i|--input-dir)
                INPUT_DIR="$2"
                shift 2
                ;;
            -o|--output-dir)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            -w|--width)
                WIDTH="$2"
                shift 2
                ;;
            -h|--height)
                HEIGHT="$2"
                shift 2
                ;;
            -c|--channels)
                CHANNELS="$2"
                shift 2
                ;;
            -b|--basename)
                BASENAME="$2"
                shift 2
                ;;
            --container)
                HOLOSCAN_CONTAINER="$2"
                shift 2
                ;;
            --batch)
                batch_mode=true
                shift
                ;;
            --help)
                show_usage
                exit 0
                ;;
            -*)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
            *)
                if [ -z "$video_file" ]; then
                    video_file="$1"
                else
                    print_error "Multiple video files specified. Use --batch for batch processing."
                    exit 1
                fi
                shift
                ;;
        esac
    done
    
    # Print header
    echo -e "${BLUE}============================================${NC}"
    echo -e "${BLUE}  Video to GXF Conversion Tool${NC}"
    echo -e "${BLUE}  Urology Inference Holoscan Application${NC}"
    echo -e "${BLUE}============================================${NC}"
    echo ""
    
    # Check dependencies
    check_dependencies
    
    # Execute based on mode
    if [ "$batch_mode" = true ]; then
        convert_batch
    elif [ -n "$video_file" ]; then
        if validate_video "$video_file"; then
            get_video_info "$video_file"
            convert_video "$video_file" "$BASENAME"
        else
            exit 1
        fi
    else
        print_error "No video file specified and batch mode not enabled"
        echo ""
        show_usage
        exit 1
    fi
}

# Run main function
main "$@" 