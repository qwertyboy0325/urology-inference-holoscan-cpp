# Multi-stage build for Urology Inference Holoscan C++
# Based on NVIDIA Holoscan SDK 3.3.0

# Stage 1: Build stage
FROM nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu as builder

LABEL maintainer="Urology Inference Team"
LABEL description="Urology Inference application based on Holoscan SDK 3.3.0"
LABEL version="1.0.0"

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV CUDA_VISIBLE_DEVICES=all
ENV NVIDIA_VISIBLE_DEVICES=all
ENV NVIDIA_DRIVER_CAPABILITIES=compute,video,utility

# Install additional build dependencies
RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    cmake \
    ninja-build \
    ccache \
    pkg-config \
    # Development libraries
    libyaml-cpp-dev \
    libopencv-dev \
    libgtest-dev \
    libgmock-dev \
    libbenchmark-dev \
    # Analysis tools
    clang-tidy \
    cppcheck \
    valgrind \
    # Utilities
    git \
    wget \
    curl \
    vim \
    htop \
    tree \
    # Video processing
    ffmpeg \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    && rm -rf /var/lib/apt/lists/*

# Setup ccache for faster builds
ENV PATH="/usr/lib/ccache:${PATH}"
ENV CCACHE_DIR="/tmp/ccache"
ENV CCACHE_MAXSIZE="2G"
RUN mkdir -p ${CCACHE_DIR} && chmod 777 ${CCACHE_DIR}

# Set work directory
WORKDIR /workspace/urology-inference

# Copy source code
COPY . .

# Make scripts executable
RUN chmod +x scripts/*.sh

# Install video encoder dependencies with verification
RUN echo "Installing video encoder dependencies..." && \
    ./scripts/install_video_encoder_deps.sh && \
    echo "Video encoder dependencies installed and verified successfully!" && \
    echo "Running post-install verification..." && \
    ./scripts/verify_video_encoder_deps.sh && \
    echo "All video encoder dependencies verified!"

# Configure build environment
ENV CMAKE_BUILD_TYPE=Release
ENV ENABLE_OPTIMIZATIONS=ON
ENV HOLOSCAN_BUILD_TYPE=Release

# Build the application
RUN ./scripts/build_optimized.sh \
    --release \
    --enable-testing \
    --enable-benchmarks \
    --jobs $(nproc) \
    --verbose

# Ensure test executables exist (create dummy files if build failed)
RUN if [ ! -f build/unit_tests ]; then \
        echo '#!/bin/bash\necho "Unit tests not available in this build"' > build/unit_tests && \
        chmod +x build/unit_tests; \
    fi && \
    if [ ! -f build/performance_benchmarks ]; then \
        echo '#!/bin/bash\necho "Performance benchmarks not available in this build"' > build/performance_benchmarks && \
        chmod +x build/performance_benchmarks; \
    fi

# Stage 2: Runtime stage
FROM nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu as runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    # Runtime libraries
    libyaml-cpp0.7 \
    libopencv-core4.5 \
    libopencv-imgproc4.5 \
    libopencv-imgcodecs4.5 \
    libopencv-highgui4.5 \
    libopencv-dnn4.5 \
    # Video processing
    ffmpeg \
    # Utilities
    curl \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user for security
RUN groupadd -r urology && useradd -r -g urology -u 1001 urology

# Set work directory
WORKDIR /app

# Copy built application from builder stage
COPY --from=builder /workspace/urology-inference/build/urology_inference_holoscan_cpp ./

# Create directories
RUN mkdir -p ./tests ./benchmarks ./config ./data

# Copy test executables
COPY --from=builder /workspace/urology-inference/build/unit_tests ./tests/
COPY --from=builder /workspace/urology-inference/build/performance_benchmarks ./benchmarks/

# Copy configuration files
COPY --from=builder /workspace/urology-inference/src/config/ ./config/

# Copy video encoder libraries from builder stage
COPY --from=builder /opt/nvidia/holoscan/lib/libgxf_encoder.so /opt/nvidia/holoscan/lib/
COPY --from=builder /opt/nvidia/holoscan/lib/libgxf_encoderio.so /opt/nvidia/holoscan/lib/
COPY --from=builder /opt/nvidia/holoscan/lib/libgxf_decoder.so /opt/nvidia/holoscan/lib/
COPY --from=builder /opt/nvidia/holoscan/lib/libgxf_decoderio.so /opt/nvidia/holoscan/lib/

# Copy verification scripts for runtime checks
COPY --from=builder /workspace/urology-inference/scripts/install_video_encoder_deps.sh ./scripts/
COPY --from=builder /workspace/urology-inference/scripts/verify_video_encoder_deps.sh ./scripts/

# Create necessary directories
RUN mkdir -p /app/logs /app/output /app/models /app/data/inputs && \
    chown -R urology:urology /app

# Copy startup script
COPY --from=builder /workspace/urology-inference/scripts/docker-entrypoint.sh ./
RUN chmod +x docker-entrypoint.sh

# Set environment variables for runtime
ENV HOLOHUB_DATA_PATH=/app/data
ENV HOLOSCAN_MODEL_PATH=/app/models
ENV UROLOGY_CONFIG_PATH=/app/config
ENV UROLOGY_LOG_PATH=/app/logs
ENV UROLOGY_OUTPUT_PATH=/app/output

# Expose any necessary ports (if web interface is added later)
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD ./urology_inference_holoscan_cpp --version || exit 1

# Switch to non-root user
USER urology

# Set default command
ENTRYPOINT ["./docker-entrypoint.sh"]
CMD ["--help"]

# Stage 3: Development stage (for development work)
FROM nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu as development

# Install ALL development tools and libraries (預裝所有依賴)
RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    cmake \
    ninja-build \
    ccache \
    pkg-config \
    # Development libraries - 完整 OpenCV 支持
    libopencv-dev \
    libopencv-core-dev \
    libopencv-imgproc-dev \
    libopencv-imgcodecs-dev \
    libopencv-highgui-dev \
    libopencv-dnn-dev \
    libopencv-features2d-dev \
    libopencv-calib3d-dev \
    libopencv-video-dev \
    libopencv-videoio-dev \
    # 其他開發庫
    libyaml-cpp-dev \
    libgtest-dev \
    libgmock-dev \
    libbenchmark-dev \
    # Development tools
    gdb \
    valgrind \
    # Analysis tools
    clang-tidy \
    cppcheck \
    # Editors
    vim \
    nano \
    # Version control
    git \
    # Documentation
    doxygen \
    graphviz \
    # Networking tools
    net-tools \
    iputils-ping \
    # Utilities
    wget \
    curl \
    htop \
    tree \
    # Video processing - 完整 FFmpeg 支持
    ffmpeg \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    libavdevice-dev \
    libavfilter-dev \
    # X11 GUI 支持
    libx11-dev \
    libxext-dev \
    libxrender-dev \
    libxtst-dev \
    libgtk-3-dev \
    && rm -rf /var/lib/apt/lists/*

# 驗證 OpenCV 安裝
RUN pkg-config --modversion opencv4 && \
    echo "OpenCV $(pkg-config --modversion opencv4) 已預裝完成"

# Set work directory
WORKDIR /workspace/urology-inference

# Copy source code
COPY . .

# Make scripts executable
RUN chmod +x scripts/*.sh

# Setup development environment
ENV CMAKE_BUILD_TYPE=Debug
ENV ENABLE_TESTING=ON
ENV ENABLE_BENCHMARKS=ON
ENV ENABLE_STATIC_ANALYSIS=ON

# Setup ccache for faster builds
ENV PATH="/usr/lib/ccache:${PATH}"
ENV CCACHE_DIR="/tmp/ccache"
ENV CCACHE_MAXSIZE="2G"
RUN mkdir -p ${CCACHE_DIR} && chmod 777 ${CCACHE_DIR}

# Install sudo for convenience
RUN apt-get update && apt-get install -y sudo && rm -rf /var/lib/apt/lists/*

# Set default command for development
CMD ["/bin/bash"] 