"""
Native HUB75 LED Matrix Driver for Rock Pi S
Pure Python implementation using libgpiod
"""

import time
import threading
from typing import Tuple, Optional
import gpiod
from gpio_config import *

class HUB75Driver:
    """
    HUB75 LED Matrix Driver
    
    Implements the HUB75 protocol for 64×32 RGB LED panels:
    - Row scanning via address lines (A, B, C, D)
    - RGB data shifted via 6 data pins + clock
    - Latch + Output Enable for display control
    """
    
    def __init__(self, width: int = MATRIX_WIDTH, height: int = MATRIX_HEIGHT, 
                 chip_path: str = GPIO_CHIP):
        self.width = width
        self.height = height
        self.rows = height // 2  # HUB75 scans top and bottom half simultaneously
        
        # Frame buffers (double buffering)
        self.front_buffer = [[[0, 0, 0] for _ in range(width)] for _ in range(height)]
        self.back_buffer = [[[0, 0, 0] for _ in range(width)] for _ in range(height)]
        self.buffer_lock = threading.Lock()
        
        # GPIO setup
        self.requests = {}  # Dict of chip_num -> LineRequest
        self.gpio_pins = {}  # Dict of pin_name -> (chip_num, line_offset)
        self.chip_path = chip_path
        
        # Display state
        self.running = False
        self.refresh_thread = None
        self.brightness = 255  # 0-255
        self.refresh_rate = 0  # Hz, calculated
        self.refresh_delay_us = 0  # Microseconds delay per row (0=max speed)
        
        print(f"Initializing HUB75 driver for {width}×{height} matrix")
        
    def initialize_gpio(self):
        """Initialize GPIO lines using gpiod 2.x API across multiple chips"""
        try:
            from gpiod.line import Direction, Value
            
            # Map of signal names to GPIO numbers
            gpio_map = {
                'R1': GPIO_R1, 'G1': GPIO_G1, 'B1': GPIO_B1,
                'R2': GPIO_R2, 'G2': GPIO_G2, 'B2': GPIO_B2,
                'A': GPIO_A, 'B': GPIO_B, 'C': GPIO_C, 'D': GPIO_D,
                'CLK': GPIO_CLK, 'LAT': GPIO_LAT, 'OE': GPIO_OE
            }
            
            # Group GPIOs by chip (RK3308: chip 0 = GPIO 0-31, chip 1 = GPIO 32-63, chip 2 = GPIO 64-95, etc.)
            # Calculate chip number and line offset for each GPIO
            chips_config = {}  # chip_num -> {line_offset: pin_name}
            
            for name, gpio_num in gpio_map.items():
                chip_num = gpio_num // 32
                line_offset = gpio_num % 32
                
                if chip_num not in chips_config:
                    chips_config[chip_num] = {}
                chips_config[chip_num][line_offset] = name
                
                # Store mapping for _set_pin
                self.gpio_pins[name] = (chip_num, line_offset)
            
            print(f"GPIO mapping: {len(gpio_map)} pins across {len(chips_config)} chips")
            
            # Request lines from each chip
            for chip_num, lines in chips_config.items():
                chip_path = f"/dev/gpiochip{chip_num}"
                
                # Create line settings for this chip
                line_config = {}
                for line_offset in lines.keys():
                    line_config[line_offset] = gpiod.LineSettings(
                        direction=Direction.OUTPUT,
                        output_value=Value.INACTIVE
                    )
                
                # Request lines from this chip
                self.requests[chip_num] = gpiod.request_lines(
                    chip_path,
                    consumer="hub75_driver",
                    config=line_config
                )
                
                print(f"✓ Initialized gpiochip{chip_num}: {len(lines)} lines {list(lines.keys())}")
            
            # Set OE high (disable output initially)
            self._set_pin('OE', 1)
            
            return True
            
        except Exception as e:
            print(f"✗ GPIO initialization failed: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def _set_pin(self, name: str, value: int):
        """Set a pin value (0 or 1)"""
        from gpiod.line import Value
        chip_num, line_offset = self.gpio_pins[name]
        request = self.requests[chip_num]
        request.set_value(line_offset, Value.ACTIVE if value else Value.INACTIVE)
    
    def set_pixel(self, x: int, y: int, r: int, g: int, b: int):
        """
        Set a single pixel color in the back buffer
        Args:
            x, y: Coordinates (0-63, 0-31)
            r, g, b: Color values (0-255)
        """
        if 0 <= x < self.width and 0 <= y < self.height:
            with self.buffer_lock:
                self.back_buffer[y][x] = [r, g, b]
    
    def clear(self, r: int = 0, g: int = 0, b: int = 0):
        """Clear the back buffer to specified color"""
        with self.buffer_lock:
            for y in range(self.height):
                for x in range(self.width):
                    self.back_buffer[y][x] = [r, g, b]
    
    def swap_buffers(self):
        """Swap front and back buffers (call after drawing)"""
        with self.buffer_lock:
            self.front_buffer, self.back_buffer = self.back_buffer, self.front_buffer
    
    def set_address(self, row: int):
        """Set row address lines (A, B, C, D) for scanning"""
        # Cache request objects for speed
        from gpiod.line import Value
        a_chip, a_off = self.gpio_pins['A']
        b_chip, b_off = self.gpio_pins['B']
        c_chip, c_off = self.gpio_pins['C']
        d_chip, d_off = self.gpio_pins['D']
        a_req = self.requests[a_chip]
        b_req = self.requests[b_chip]
        c_req = self.requests[c_chip]
        d_req = self.requests[d_chip]
        
        # Set address bits directly
        a_req.set_value(a_off, Value.ACTIVE if row & 0x01 else Value.INACTIVE)
        b_req.set_value(b_off, Value.ACTIVE if (row >> 1) & 0x01 else Value.INACTIVE)
        c_req.set_value(c_off, Value.ACTIVE if (row >> 2) & 0x01 else Value.INACTIVE)
        d_req.set_value(d_off, Value.ACTIVE if (row >> 3) & 0x01 else Value.INACTIVE)
    
    def shift_out_row(self, row: int):
        """
        Shift out one row of data for both panels - OPTIMIZED
        Args:
            row: Row number (0-15)
        """
        from gpiod.line import Value
        
        # Cache GPIO request objects and offsets for inner loop speed
        r1_chip, r1_off = self.gpio_pins['R1']
        g1_chip, g1_off = self.gpio_pins['G1']
        b1_chip, b1_off = self.gpio_pins['B1']
        r2_chip, r2_off = self.gpio_pins['R2']
        g2_chip, g2_off = self.gpio_pins['G2']
        b2_chip, b2_off = self.gpio_pins['B2']
        clk_chip, clk_off = self.gpio_pins['CLK']
        
        r1_req = self.requests[r1_chip]
        g1_req = self.requests[g1_chip]
        b1_req = self.requests[b1_chip]
        r2_req = self.requests[r2_chip]
        g2_req = self.requests[g2_chip]
        b2_req = self.requests[b2_chip]
        clk_req = self.requests[clk_chip]
        
        # Cache values for speed
        brightness = self.brightness
        front_buffer = self.front_buffer
        top_row = row
        bottom_row = row + self.rows
        ACTIVE = Value.ACTIVE
        INACTIVE = Value.INACTIVE
        
        # Shift out all 64 columns for this row
        for x in range(64):  # Hardcode for speed
            # Get pixel colors
            r1, g1, b1 = front_buffer[top_row][x]
            r2, g2, b2 = front_buffer[bottom_row][x]
            
            # Apply brightness and threshold (1-bit color)
            r1 = ACTIVE if (r1 * brightness) > 32512 else INACTIVE  # 127 * 256
            g1 = ACTIVE if (g1 * brightness) > 32512 else INACTIVE
            b1 = ACTIVE if (b1 * brightness) > 32512 else INACTIVE
            r2 = ACTIVE if (r2 * brightness) > 32512 else INACTIVE
            g2 = ACTIVE if (g2 * brightness) > 32512 else INACTIVE
            b2 = ACTIVE if (b2 * brightness) > 32512 else INACTIVE
            
            # Set data lines directly (no function call overhead)
            r1_req.set_value(r1_off, r1)
            g1_req.set_value(g1_off, g1)
            b1_req.set_value(b1_off, b1)
            r2_req.set_value(r2_off, r2)
            g2_req.set_value(g2_off, g2)
            b2_req.set_value(b2_off, b2)
            
            # Clock pulse
            clk_req.set_value(clk_off, ACTIVE)
            clk_req.set_value(clk_off, INACTIVE)
    
    def display_row(self, row: int):
        """Display one row on the matrix - OPTIMIZED"""
        from gpiod.line import Value
        
        # Cache OE and LAT requests
        oe_chip, oe_off = self.gpio_pins['OE']
        lat_chip, lat_off = self.gpio_pins['LAT']
        oe_req = self.requests[oe_chip]
        lat_req = self.requests[lat_chip]
        
        # Disable output while updating
        oe_req.set_value(oe_off, Value.ACTIVE)
        
        # Set row address
        self.set_address(row)
        
        # Shift out data
        self.shift_out_row(row)
        
        # Latch data
        lat_req.set_value(lat_off, Value.ACTIVE)
        lat_req.set_value(lat_off, Value.INACTIVE)
        
        # Enable output
        oe_req.set_value(oe_off, Value.INACTIVE)
    
    def refresh_loop(self):
        """Main refresh loop - scans all rows continuously"""
        print("Starting refresh loop...")
        frame_count = 0
        last_fps_time = time.time()
        
        while self.running:
            # Scan all rows as fast as possible (or with delay if configured)
            for row in range(self.rows):
                if not self.running:
                    break
                self.display_row(row)
                # Optional delay for throttling refresh rate
                if self.refresh_delay_us > 0:
                    time.sleep(self.refresh_delay_us / 1_000_000)
            
            # Calculate refresh rate
            frame_count += 1
            now = time.time()
            if now - last_fps_time >= 1.0:
                self.refresh_rate = frame_count / (now - last_fps_time)
                frame_count = 0
                last_fps_time = now
    
    def start(self):
        """Start the display refresh thread"""
        if self.running:
            print("Display already running")
            return False
        
        if not self.initialize_gpio():
            return False
        
        self.running = True
        self.refresh_thread = threading.Thread(target=self.refresh_loop, daemon=True)
        self.refresh_thread.start()
        print("✓ Display started")
        return True
    
    def stop(self):
        """Stop the display refresh thread"""
        print("Stopping display...")
        self.running = False
        if self.refresh_thread:
            self.refresh_thread.join(timeout=1.0)
        
        # Turn off display
        if self.requests:
            try:
                self._set_pin('OE', 1)
            except:
                pass
        
        # Release GPIO
        for request in self.requests.values():
            try:
                request.release()
            except:
                pass
        self.requests = {}
        self.gpio_pins = {}
        
        print("✓ Display stopped")
    
    def set_brightness(self, brightness: int):
        """Set brightness (0-255)"""
        self.brightness = max(0, min(255, brightness))
    
    def set_refresh_delay(self, delay_us: int):
        """Set refresh delay in microseconds (0=max speed, higher=slower)"""
        self.refresh_delay_us = max(0, min(10000, delay_us))
    
    def get_status(self) -> dict:
        """Get driver status information"""
        return {
            'running': self.running,
            'refresh_rate': round(self.refresh_rate, 1),
            'brightness': self.brightness,
            'refresh_delay_us': self.refresh_delay_us,
            'resolution': f"{self.width}×{self.height}",
            'gpio_chip': self.chip_path
        }

# Test code
if __name__ == "__main__":
    import sys
    
    print("HUB75 Driver Test")
    print("=" * 50)
    
    # Create driver
    driver = HUB75Driver()
    
    try:
        # Start display
        if not driver.start():
            print("Failed to start display")
            sys.exit(1)
        
        print("\nTest 1: Red fill")
        driver.clear(255, 0, 0)
        driver.swap_buffers()
        time.sleep(2)
        
        print("Test 2: Green fill")
        driver.clear(0, 255, 0)
        driver.swap_buffers()
        time.sleep(2)
        
        print("Test 3: Blue fill")
        driver.clear(0, 0, 255)
        driver.swap_buffers()
        time.sleep(2)
        
        print("Test 4: White fill")
        driver.clear(255, 255, 255)
        driver.swap_buffers()
        time.sleep(2)
        
        print("Test 5: Checkerboard pattern")
        for y in range(32):
            for x in range(64):
                if (x + y) % 2 == 0:
                    driver.set_pixel(x, y, 255, 255, 255)
                else:
                    driver.set_pixel(x, y, 0, 0, 0)
        driver.swap_buffers()
        time.sleep(2)
        
        # Show status
        status = driver.get_status()
        print("\nStatus:")
        for key, value in status.items():
            print(f"  {key}: {value}")
        
        print("\nPress Ctrl+C to exit...")
        while True:
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        driver.stop()
