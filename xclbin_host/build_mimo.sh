#!/bin/bash
# Build script for generating all MIMO configurations
# This script automates the process of building xclbin files for different MIMO configurations

echo "========================================================================"
echo "MIMO Multi-Configuration Build Script"
echo "========================================================================"
echo ""

# Available configurations
CONFIGS=("4x4_16QAM" "8x8_4QAM" "8x8_16QAM" "8x8_64QAM" "16x16_16QAM" "16x16_64QAM")

# Function to build a single configuration
build_config() {
    local config=$1
    echo ""
    echo "========================================================================"
    echo "Building MIMO Configuration: $config"
    echo "========================================================================"
    
    if make -f Makefile_multiconfig build MIMO_CONFIG=$config; then
        echo "✓ Successfully built $config"
        return 0
    else
        echo "✗ Failed to build $config"
        return 1
    fi
}

# Parse command line arguments
if [ "$1" == "all" ]; then
    echo "Building all MIMO configurations..."
    echo ""
    
    failed_configs=()
    
    for config in "${CONFIGS[@]}"; do
        if ! build_config "$config"; then
            failed_configs+=("$config")
        fi
    done
    
    echo ""
    echo "========================================================================"
    echo "Build Summary"
    echo "========================================================================"
    
    if [ ${#failed_configs[@]} -eq 0 ]; then
        echo "✓ All configurations built successfully!"
    else
        echo "✗ Failed configurations:"
        for config in "${failed_configs[@]}"; do
            echo "  - $config"
        done
        exit 1
    fi
    
elif [ "$1" == "list" ]; then
    echo "Available MIMO configurations:"
    for config in "${CONFIGS[@]}"; do
        echo "  - $config"
    done
    echo ""
    echo "Usage:"
    echo "  ./build_mimo.sh <config>   - Build specific configuration"
    echo "  ./build_mimo.sh all        - Build all configurations"
    echo "  ./build_mimo.sh list       - List available configurations"
    
elif [ -n "$1" ]; then
    # Build specific configuration
    if [[ " ${CONFIGS[@]} " =~ " $1 " ]]; then
        build_config "$1"
    else
        echo "Error: Invalid configuration '$1'"
        echo ""
        echo "Available configurations:"
        for config in "${CONFIGS[@]}"; do
            echo "  - $config"
        done
        exit 1
    fi
    
else
    echo "Usage:"
    echo "  ./build_mimo.sh <config>   - Build specific configuration"
    echo "  ./build_mimo.sh all        - Build all configurations"
    echo "  ./build_mimo.sh list       - List available configurations"
    echo ""
    echo "Examples:"
    echo "  ./build_mimo.sh 8x8_16QAM"
    echo "  ./build_mimo.sh all"
    exit 1
fi
