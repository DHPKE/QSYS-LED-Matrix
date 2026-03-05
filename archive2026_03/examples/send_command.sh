#!/bin/bash
# Bash script example for sending commands to LED Matrix
# Usage: ./send_command.sh "Your message"

# Configuration
LED_MATRIX_IP="${LED_MATRIX_IP:-192.168.1.100}"
LED_MATRIX_PORT="${LED_MATRIX_PORT:-21324}"

# Function to send UDP command
send_command() {
    local command="$1"
    echo "$command" | nc -u -w1 "$LED_MATRIX_IP" "$LED_MATRIX_PORT"
    if [ $? -eq 0 ]; then
        echo "✓ Sent: $command"
    else
        echo "✗ Error sending command"
        return 1
    fi
}

# Function to send text
send_text() {
    local segment="${1:-0}"
    local text="$2"
    local color="${3:-FFFFFF}"
    local font="${4:-roboto12}"
    local size="${5:-auto}"
    local align="${6:-C}"
    local effect="${7:-none}"
    
    local command="TEXT|$segment|$text|$color|$font|$size|$align|$effect"
    send_command "$command"
}

# Function to clear segment
clear_segment() {
    local segment="${1:-0}"
    send_command "CLEAR|$segment"
}

# Function to clear all
clear_all() {
    send_command "CLEAR_ALL"
}

# Function to set brightness
set_brightness() {
    local brightness="${1:-128}"
    send_command "BRIGHTNESS|$brightness"
}

# Main script
show_usage() {
    cat << EOF
LED Matrix Control Script
Usage: $0 [OPTIONS] "Text to display"

Options:
  -i IP        LED Matrix IP address (default: $LED_MATRIX_IP)
  -p PORT      UDP port (default: $LED_MATRIX_PORT)
  -s SEGMENT   Segment ID 0-3 (default: 0)
  -c COLOR     Hex color RRGGBB (default: FFFFFF)
  -f FONT      Font name (default: roboto12)
  --clear      Clear segment
  --clear-all  Clear all segments
  -b VALUE     Set brightness 0-255

Examples:
  $0 "Hello World"
  $0 -s 1 -c FF0000 "Alert!"
  $0 -f digital12 "Temperature: 72.5"
  $0 --clear 0
  $0 --clear-all
  $0 -b 128

Available fonts:
  roboto6, roboto8, roboto12, roboto16, roboto24
  digital12, digital24
  mono9, mono12

Environment variables:
  LED_MATRIX_IP    - Matrix IP address
  LED_MATRIX_PORT  - UDP port
EOF
}

# Parse arguments
SEGMENT=0
COLOR="FFFFFF"
FONT="roboto12"
SIZE="auto"
ALIGN="C"
EFFECT="none"
CLEAR_MODE=""
BRIGHTNESS=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -i)
            LED_MATRIX_IP="$2"
            shift 2
            ;;
        -p)
            LED_MATRIX_PORT="$2"
            shift 2
            ;;
        -s)
            SEGMENT="$2"
            shift 2
            ;;
        -c)
            COLOR="$2"
            shift 2
            ;;
        -f)
            FONT="$2"
            shift 2
            ;;
        --clear)
            CLEAR_MODE="segment"
            SEGMENT="${2:-0}"
            shift
            shift
            ;;
        --clear-all)
            CLEAR_MODE="all"
            shift
            ;;
        -b)
            BRIGHTNESS="$2"
            shift 2
            ;;
        *)
            TEXT="$1"
            shift
            ;;
    esac
done

# Check for netcat
if ! command -v nc &> /dev/null; then
    echo "Error: netcat (nc) is required but not installed"
    echo "Install with: sudo apt install netcat  (or brew install netcat on Mac)"
    exit 1
fi

# Execute command
if [ -n "$CLEAR_MODE" ]; then
    if [ "$CLEAR_MODE" = "all" ]; then
        clear_all
    else
        clear_segment "$SEGMENT"
    fi
elif [ -n "$BRIGHTNESS" ]; then
    set_brightness "$BRIGHTNESS"
elif [ -n "$TEXT" ]; then
    send_text "$SEGMENT" "$TEXT" "$COLOR" "$FONT" "$SIZE" "$ALIGN" "$EFFECT"
else
    show_usage
    exit 1
fi
