#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
VST3_OUTPUT="$BUILD_DIR/KndlSynth_artefacts/Debug/VST3/KndlSynth.vst3"
STANDALONE_APP="$BUILD_DIR/KndlSynth_artefacts/Debug/Standalone/KndlSynth.app"
TESTS_BIN="$BUILD_DIR/KndlSynthTests_artefacts/Debug/KndlSynthTests"
VST3_INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/VST3"

APP_PROCESS_NAME="KndlSynth"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Parse args
RUN_STANDALONE=false
INSTALL_VST=false
FORCE_CONFIGURE=false
SKIP_TESTS=false

for arg in "$@"; do
    case $arg in
        -r|--run) RUN_STANDALONE=true ;;
        -i|--install) INSTALL_VST=true ;;
        -c|--configure) FORCE_CONFIGURE=true ;;
        -s|--skip-tests) SKIP_TESTS=true ;;
        -h|--help)
            echo "Usage: ./build.sh [options]"
            echo "  -r, --run         Build and run standalone app"
            echo "  -i, --install     Build and install VST3 to ~/Library/Audio/Plug-Ins/VST3"
            echo "  -c, --configure   Force CMake reconfigure"
            echo "  -s, --skip-tests  Skip running tests"
            echo "  (no args)         Build + test"
            exit 0
            ;;
    esac
done

# ── Step 1: Kill running instances ──────────────────────────────────
if pgrep -x "$APP_PROCESS_NAME" > /dev/null 2>&1; then
    echo -e "${YELLOW}Closing running KndlSynth...${NC}"
    pkill -x "$APP_PROCESS_NAME" 2>/dev/null || true
    # Wait for process to actually exit
    for i in {1..10}; do
        if ! pgrep -x "$APP_PROCESS_NAME" > /dev/null 2>&1; then
            break
        fi
        sleep 0.3
    done
    # Force kill if still alive
    if pgrep -x "$APP_PROCESS_NAME" > /dev/null 2>&1; then
        echo -e "${RED}Force killing KndlSynth...${NC}"
        pkill -9 -x "$APP_PROCESS_NAME" 2>/dev/null || true
        sleep 0.5
    fi
    echo -e "${GREEN}KndlSynth closed.${NC}"
fi

# ── Step 2: Configure (if needed) ──────────────────────────────────
if [ ! -f "$BUILD_DIR/build.ninja" ] || $FORCE_CONFIGURE; then
    echo -e "${YELLOW}Configuring CMake...${NC}"
    cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug
fi

# ── Step 3: Build tests ────────────────────────────────────────────
if ! $SKIP_TESTS; then
    echo -e "${YELLOW}Building tests...${NC}"
    cmake --build "$BUILD_DIR" --target KndlSynthTests
    
    echo -e "${CYAN}Running tests...${NC}"
    if "$TESTS_BIN" 2>&1 | tee /tmp/kndl_test_output.txt | tail -5; then
        # Check exit code from the test binary (not tail)
        TEST_EXIT=${PIPESTATUS[0]}
        if [ "$TEST_EXIT" -ne 0 ]; then
            echo ""
            echo -e "${RED}Tests FAILED (exit code $TEST_EXIT). Fix before building.${NC}"
            echo -e "${RED}Full output: /tmp/kndl_test_output.txt${NC}"
            exit 1
        fi
        echo -e "${GREEN}All tests passed.${NC}"
    else
        echo -e "${RED}Tests FAILED. Fix before building.${NC}"
        exit 1
    fi
    echo ""
fi

# ── Step 4: Build plugin ───────────────────────────────────────────
echo -e "${YELLOW}Building KndlSynth...${NC}"
cmake --build "$BUILD_DIR"
echo -e "${GREEN}Build complete.${NC}"

# ── Step 5: Copy/update factory presets ───────────────────────────
PRESET_SRC="$PROJECT_DIR/resources/presets"
PRESET_DST="$HOME/Documents/KndlSynth/Presets"

if [ -d "$PRESET_SRC" ]; then
    mkdir -p "$PRESET_DST"
    UPDATED=0
    COPIED=0
    for preset in "$PRESET_SRC"/*.kndl; do
        [ -f "$preset" ] || continue
        filename="$(basename "$preset")"
        dst_file="$PRESET_DST/$filename"
        if [ -f "$dst_file" ]; then
            # Update only if source is newer
            if [ "$preset" -nt "$dst_file" ]; then
                cp "$preset" "$dst_file"
                UPDATED=$((UPDATED + 1))
            fi
        else
            cp "$preset" "$dst_file"
            COPIED=$((COPIED + 1))
        fi
    done
    if [ $COPIED -gt 0 ] || [ $UPDATED -gt 0 ]; then
        echo -e "${GREEN}Presets: ${COPIED} new, ${UPDATED} updated → $PRESET_DST${NC}"
    else
        echo -e "${CYAN}Presets up to date.${NC}"
    fi
fi

# ── Step 6: Install VST3 (optional) ──────────────────────────────
if $INSTALL_VST; then
    echo -e "${YELLOW}Installing VST3...${NC}"
    mkdir -p "$VST3_INSTALL_DIR"
    rm -rf "$VST3_INSTALL_DIR/KndlSynth.vst3"
    cp -R "$VST3_OUTPUT" "$VST3_INSTALL_DIR/"
    echo -e "${GREEN}VST3 installed: $VST3_INSTALL_DIR/KndlSynth.vst3${NC}"
fi

# ── Step 7: Launch standalone (optional) ──────────────────────────
if $RUN_STANDALONE; then
    echo -e "${YELLOW}Launching standalone...${NC}"
    open "$STANDALONE_APP"
else
    echo ""
    echo "  Run standalone:  ./build.sh -r"
    echo "  Install VST3:    ./build.sh -i"
    echo "  Skip tests:      ./build.sh -s"
fi
