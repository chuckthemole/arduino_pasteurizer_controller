#!/bin/bash

# Arduino PlatformIO Helper Script
# Usage: ./pio_helper.sh [command] [options]

# Default values
DEFAULT_BAUD=9600
PORT=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to detect Arduino port automatically
detect_port() {
    echo -e "${BLUE}Detecting Arduino port...${NC}"
    
    # Look for common Arduino port patterns
    PORTS=($(ls /dev/cu.usbmodem* 2>/dev/null))
    
    if [ ${#PORTS[@]} -eq 0 ]; then
        echo -e "${RED}No Arduino ports found!${NC}"
        echo "Make sure your Arduino is connected."
        return 1
    elif [ ${#PORTS[@]} -eq 1 ]; then
        PORT=${PORTS[0]}
        echo -e "${GREEN}Found port: $PORT${NC}"
        return 0
    else
        echo -e "${YELLOW}Multiple ports found:${NC}"
        for i in "${!PORTS[@]}"; do
            echo "  $((i+1)). ${PORTS[i]}"
        done
        read -p "Select port (1-${#PORTS[@]}): " choice
        if [[ $choice =~ ^[0-9]+$ ]] && [ $choice -ge 1 ] && [ $choice -le ${#PORTS[@]} ]; then
            PORT=${PORTS[$((choice-1))]}
            echo -e "${GREEN}Selected port: $PORT${NC}"
            return 0
        else
            echo -e "${RED}Invalid selection${NC}"
            return 1
        fi
    fi
}

# Function to show usage
show_usage() {
    echo "Arduino PlatformIO Helper Script"
    echo ""
    echo "Usage: $0 [command] [options]"
    echo ""
    echo "Commands:"
    echo "  upload, u           - Build and upload code"
    echo "  monitor, m          - Start serial monitor"
    echo "  both, b             - Upload then monitor"
    echo "  clean, c            - Clean build files"
    echo "  build               - Build only (no upload)"
    echo "  list, l             - List available devices"
    echo "  kill, k             - Kill processes using serial ports"
    echo ""
    echo "Options:"
    echo "  -p, --port PORT     - Specify port (e.g., /dev/cu.usbmodemXXXX)"
    echo "  -b, --baud RATE     - Specify baud rate (default: 9600)"
    echo "  -h, --help          - Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 upload"
    echo "  $0 monitor -p /dev/cu.usbmodemF412FA9F37082"
    echo "  $0 both -b 115200"
    echo "  $0 upload -p /dev/cu.usbmodemF412FA9F37082"
}

# Parse command line arguments
COMMAND=""
BAUD=$DEFAULT_BAUD

while [[ $# -gt 0 ]]; do
    case $1 in
        upload|u)
            COMMAND="upload"
            shift
            ;;
        monitor|m)
            COMMAND="monitor"
            shift
            ;;
        both|b)
            COMMAND="both"
            shift
            ;;
        clean|c)
            COMMAND="clean"
            shift
            ;;
        build)
            COMMAND="build"
            shift
            ;;
        list|l)
            COMMAND="list"
            shift
            ;;
        kill|k)
            COMMAND="kill"
            shift
            ;;
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -b|--baud)
            BAUD="$2"
            shift 2
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            if [ -z "$COMMAND" ]; then
                COMMAND="$1"
            else
                echo -e "${RED}Unknown option: $1${NC}"
                show_usage
                exit 1
            fi
            shift
            ;;
    esac
done

# If no command provided, show usage
if [ -z "$COMMAND" ]; then
    show_usage
    exit 1
fi

# Execute commands
case $COMMAND in
    upload)
        echo -e "${BLUE}Building and uploading...${NC}"
        pio run --target upload
        ;;
    
    monitor)
        if [ -z "$PORT" ]; then
            detect_port || exit 1
        fi
        echo -e "${BLUE}Starting serial monitor on $PORT at ${BAUD} baud...${NC}"
        echo -e "${YELLOW}Press Ctrl+C to exit monitor${NC}"
        pio device monitor --port "$PORT" --baud "$BAUD"
        ;;
    
    both)
        echo -e "${BLUE}Building and uploading...${NC}"
        pio run --target upload
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}Upload successful! Starting monitor...${NC}"
            sleep 2  # Give the Arduino time to reset
            if [ -z "$PORT" ]; then
                detect_port || exit 1
            fi
            echo -e "${YELLOW}Press Ctrl+C to exit monitor${NC}"
            pio device monitor --port "$PORT" --baud "$BAUD"
        else
            echo -e "${RED}Upload failed!${NC}"
            exit 1
        fi
        ;;
    
    clean)
        echo -e "${BLUE}Cleaning build files...${NC}"
        pio run --target clean
        ;;
    
    build)
        echo -e "${BLUE}Building project...${NC}"
        pio run
        ;;
    
    list)
        echo -e "${BLUE}Available devices:${NC}"
        pio device list
        ;;
    
    kill)
        echo -e "${BLUE}Killing processes using serial ports...${NC}"
        pkill -f "pio device monitor" 2>/dev/null
        pkill -f "cu.usbmodem" 2>/dev/null
        pkill -f platformio 2>/dev/null
        echo -e "${GREEN}Done! Serial ports should be free now.${NC}"
        ;;
    
    *)
        echo -e "${RED}Unknown command: $COMMAND${NC}"
        show_usage
        exit 1
        ;;
    esac
