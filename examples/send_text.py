#!/usr/bin/env python3
"""
Example Python script to send text to LED Matrix via UDP
Usage: python3 send_text.py "Your message here"
"""

import socket
import sys
import argparse

def send_command(ip, port, command):
    """Send UDP command to LED Matrix"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    message = (command + "\n").encode('utf-8')
    try:
        sock.sendto(message, (ip, port))
        print(f"✓ Sent: {command}")
        return True
    except Exception as e:
        print(f"✗ Error: {e}")
        return False
    finally:
        sock.close()

def send_text(ip, port, segment, text, color="FFFFFF", font="roboto12", 
              size="auto", align="C", effect="none"):
    """Send TEXT command with parameters"""
    command = f"TEXT|{segment}|{text}|{color}|{font}|{size}|{align}|{effect}"
    return send_command(ip, port, command)

def clear_segment(ip, port, segment):
    """Clear a specific segment"""
    command = f"CLEAR|{segment}"
    return send_command(ip, port, command)

def clear_all(ip, port):
    """Clear all segments"""
    return send_command(ip, port, "CLEAR_ALL")

def set_brightness(ip, port, brightness):
    """Set display brightness (0-255)"""
    command = f"BRIGHTNESS|{brightness}"
    return send_command(ip, port, command)

def main():
    parser = argparse.ArgumentParser(
        description='Send text to Olimex LED Matrix via UDP',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Display "Hello World" in white on segment 0
  %(prog)s "Hello World"
  
  # Display red text on segment 1
  %(prog)s "Alert!" -s 1 -c FF0000
  
  # Display with digital font and scrolling
  %(prog)s "Temperature: 72.5" -f digital12 -e scroll
  
  # Clear all segments
  %(prog)s --clear-all
  
  # Set brightness to 50%%
  %(prog)s --brightness 128
        ''')
    
    parser.add_argument('text', nargs='?', help='Text to display')
    parser.add_argument('-i', '--ip', default='192.168.1.100',
                        help='LED Matrix IP address (default: 192.168.1.100)')
    parser.add_argument('-p', '--port', type=int, default=21324,
                        help='UDP port (default: 21324)')
    parser.add_argument('-s', '--segment', type=int, default=0,
                        help='Segment ID 0-3 (default: 0)')
    parser.add_argument('-c', '--color', default='FFFFFF',
                        help='Hex color RRGGBB (default: FFFFFF)')
    parser.add_argument('-f', '--font', default='roboto12',
                        choices=['roboto6', 'roboto8', 'roboto12', 'roboto16', 
                                'roboto24', 'digital12', 'digital24', 'mono9', 'mono12'],
                        help='Font name (default: roboto12)')
    parser.add_argument('--size', default='auto',
                        help='Font size 6-32 or "auto" (default: auto)')
    parser.add_argument('-a', '--align', default='C', choices=['L', 'C', 'R'],
                        help='Alignment: L=left, C=center, R=right (default: C)')
    parser.add_argument('-e', '--effect', default='none',
                        choices=['none', 'scroll', 'blink', 'fade', 'rainbow'],
                        help='Text effect (default: none)')
    parser.add_argument('--clear', type=int, metavar='SEGMENT',
                        help='Clear specific segment (0-3)')
    parser.add_argument('--clear-all', action='store_true',
                        help='Clear all segments')
    parser.add_argument('-b', '--brightness', type=int, metavar='VALUE',
                        help='Set brightness 0-255')
    
    args = parser.parse_args()
    
    # Handle special commands
    if args.clear_all:
        return 0 if clear_all(args.ip, args.port) else 1
    
    if args.clear is not None:
        return 0 if clear_segment(args.ip, args.port, args.clear) else 1
    
    if args.brightness is not None:
        if not 0 <= args.brightness <= 255:
            print("Error: Brightness must be 0-255")
            return 1
        return 0 if set_brightness(args.ip, args.port, args.brightness) else 1
    
    # Send text command
    if args.text is None:
        parser.print_help()
        return 1
    
    # Remove # from color if present
    color = args.color.lstrip('#')
    
    # Validate color
    if len(color) != 6:
        print("Error: Color must be 6-digit hex (RRGGBB)")
        return 1
    
    # Validate segment
    if not 0 <= args.segment <= 3:
        print("Error: Segment must be 0-3")
        return 1
    
    success = send_text(args.ip, args.port, args.segment, args.text,
                       color, args.font, args.size, args.align, args.effect)
    
    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
