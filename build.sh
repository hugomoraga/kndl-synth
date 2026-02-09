#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
VST3_OUTPUT="$BUILD_DIR/KndlSynth_artefacts/Debug/VST3/KndlSynth.vst3"
STANDALONE_APP="$BUILD_DIR/KndlSynth_artefacts/Debug/Standalone/KndlSynth.app"
VST3_INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/VST3"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Parse args
RUN_STANDALONE=false
INSTALL_VST=false
FORCE_CONFIGURE=false

for arg in "$@"; do
    case $arg in
        -r|--run) RUN_STANDALONE=true ;;
        -i|--install) INSTALL_VST=true ;;
        -c|--configure) FORCE_CONFIGURE=true ;;
        -h|--help)
            echo "Usage: ./build.sh [options]"
            echo "  -r, --run       Build and run standalone app"
            echo "  -i, --install   Build and install VST3 to ~/Library/Audio/Plug-Ins/VST3"
            echo "  -c, --configure Force CMake reconfigure"
            echo "  (no args)       Just build (fast incremental)"
            exit 0
            ;;
    esac
done

# Only configure if build.ninja doesn't exist or -c flag is passed
if [ ! -f "$BUILD_DIR/build.ninja" ] || $FORCE_CONFIGURE; then
    echo -e "${YELLOW}🔧 Configuring...${NC}"
    cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug
fi

echo -e "${YELLOW}🔨 Building...${NC}"
cmake --build "$BUILD_DIR"

if $INSTALL_VST; then
    echo -e "${YELLOW}📦 Installing VST3...${NC}"
    rm -rf "$VST3_INSTALL_DIR/KndlSynth.vst3"
    cp -R "$VST3_OUTPUT" "$VST3_INSTALL_DIR/"
    echo -e "${GREEN}✅ VST3 installed to: $VST3_INSTALL_DIR/KndlSynth.vst3${NC}"
fi

if $RUN_STANDALONE; then
    echo -e "${YELLOW}🚀 Launching standalone...${NC}"
    open "$STANDALONE_APP"
else
    echo -e "${GREEN}✅ Build complete!${NC}"
    echo ""
    echo "  Run standalone:  ./build.sh -r"
    echo "  Install VST3:    ./build.sh -i"
fi
