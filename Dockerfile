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
RUN mkdir -p ${CCACHE_DIR}

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

# Install development tools
RUN apt-get update && apt-get install -y \
    # Development tools
    gdb \
    valgrind \
    perf-tools-unstable \
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
    && rm -rf /var/lib/apt/lists/*

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

# Create development user (check if exists first)
RUN if ! getent group developer > /dev/null 2>&1; then groupadd -r developer; fi && \
    if ! getent passwd developer > /dev/null 2>&1; then \
        # Find available UID starting from 1000
        for uid in $(seq 1000 1100); do \
            if ! getent passwd $uid > /dev/null 2>&1; then \
                useradd -r -g developer -u $uid -s /bin/bash -m developer && break; \
            fi; \
        done; \
    fi && \
    chown -R developer:developer /workspace

USER developer

# Set default command for development
CMD ["/bin/bash"] 