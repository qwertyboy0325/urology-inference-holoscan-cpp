#!/bin/bash

# 🎯 Integrated X11 solution for Docker environment
# Uses pre-installed dependencies in the development stage

set -e

echo "🚀 Starting Urology Inference development environment with full X11 support..."

# 1. Check SSH X11 forwarding environment
echo "📋 Checking SSH X11 forwarding environment..."
echo "DISPLAY: $DISPLAY"
echo "SSH_CLIENT: $SSH_CLIENT"
echo "SSH_CONNECTION: $SSH_CONNECTION"

# 2. Set correct DISPLAY variable
if [ -z "$DISPLAY" ]; then
    if [ -n "$SSH_CLIENT" ]; then
        export DISPLAY=localhost:10.0
        echo "🔧 Automatically set DISPLAY for SSH forwarding: $DISPLAY"
    else
        export DISPLAY=:0
        echo "🔧 Set DISPLAY for local display: $DISPLAY"
    fi
else
    echo "✅ Using existing DISPLAY: $DISPLAY"
fi

# 3. Set X11 environment variables
echo "🔑 Setting X11 environment variables..."
export XSOCK=/tmp/.X11-unix
export XAUTH=/tmp/.docker.xauth.urology

# 4. Create SSH X11 authentication file
echo "📝 Creating SSH X11 authentication file..."
sudo rm -f $XAUTH

if [ -n "$SSH_CLIENT" ] && [ -n "$DISPLAY" ]; then
    echo "🔐 SSH environment: creating X11 authentication file..."
    if [ -f "$HOME/.Xauthority" ]; then
        echo "📋 Copying user .Xauthority file..."
        sudo cp "$HOME/.Xauthority" "$XAUTH"
    else
        echo "📋 .Xauthority does not exist, creating new authentication file..."
        if xauth list $DISPLAY 2>/dev/null | head -1 | sudo xauth -f $XAUTH nmerge - 2>/dev/null; then
            echo "✅ Successfully obtained authentication from current DISPLAY"
        else
            echo "⚠️  Unable to obtain authentication, creating empty authentication file"
            sudo touch $XAUTH
            echo "add $DISPLAY . $(mcookie)" | sudo xauth -f $XAUTH source - 2>/dev/null || true
        fi
    fi
else
    echo "🖥️  Local environment: using standard method..."
    xauth nlist $DISPLAY 2>/dev/null | sed -e 's/^..../ffff/' | sudo xauth -f $XAUTH nmerge - 2>/dev/null || sudo touch $XAUTH
fi

sudo chmod 777 $XAUTH
echo "✅ Authentication file created: $XAUTH"

# 5. Find nvidia_icd.json
echo "🔍 Locating nvidia_icd.json..."
nvidia_icd_json=$(find /usr/share /etc -path '*/vulkan/icd.d/nvidia_icd.json' -type f,l -print -quit 2>/dev/null | grep .) || nvidia_icd_json="/usr/share/vulkan/icd.d/nvidia_icd.json"
echo "✅ Using nvidia_icd.json: $nvidia_icd_json"

# 6. Clean up existing containers
echo "🛑 Cleaning up existing containers..."
docker compose down 2>/dev/null || true
docker stop urology-dev 2>/dev/null || true
docker rm urology-dev 2>/dev/null || true

# 7. Build development image (if needed)
echo "🔨 Building development image..."
docker compose build urology-dev

# 8. Start development container with full X11 support

echo "🚀 Starting development container (full X11 support)..."
docker run -it --rm \
  --name urology-dev-x11 \
  --gpus all \
  --net host \
  --ipc host \
  --pid host \
  --memory=8g \
  --memory-swap=16g \
  --shm-size=2g \
  -v $XAUTH:$XAUTH \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v $nvidia_icd_json:$nvidia_icd_json:ro \
  -v "$(pwd):/workspace" \
  -v "./data:/workspace/data" \
  -w /workspace \
  -e DISPLAY=$DISPLAY \
  -e XAUTHORITY=$XAUTH \
  -e NVIDIA_DRIVER_CAPABILITIES=graphics,video,compute,utility,display \
  -e LIBGL_ALWAYS_INDIRECT=1 \
  -e MESA_GL_VERSION_OVERRIDE=3.3 \
  -e CMAKE_C_COMPILER=/usr/bin/gcc \
  -e CMAKE_CXX_COMPILER=/usr/bin/g++ \
  -e CC=/usr/bin/gcc \
  -e CXX=/usr/bin/g++ \
  --cap-add=CAP_SYS_PTRACE \
  --security-opt seccomp=unconfined \
  --ulimit memlock=-1 \
  urology-inference-holoscan-cpp-urology-dev:latest \
  bash -c "
    echo '=== Urology Inference Development Environment (Full X11 Support) ==='
    echo '📋 Environment check:'
    echo 'DISPLAY=' \$DISPLAY
    echo 'XAUTHORITY=' \$XAUTHORITY
    echo 'OpenCV version:' \$(pkg-config --modversion opencv4)
    echo 'Holoscan version: 3.3.0'
    echo ''
    
    # Build project
    echo '🔨 Building project...'
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    if cmake .. && make -j\$(nproc); then
        echo '✅ Project build successful!'
        echo ''
        echo '📁 Available executables:'
        ls -la *urology* *simple* *hello* 2>/dev/null || echo 'No executables found'
        echo ''
        echo '🎯 Running full application (Display mode)...'
                ./urology_inference_holoscan_cpp --data=../data
    else
        echo '❌ Project build failed'
    fi
    echo ''
    echo '🎯 Development environment is ready!'
    /bin/bash
  " 