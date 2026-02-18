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
        self.request = None
        self.gpio_pins = {}
        self.chip_path = chip_path
        
        # Display state
        self.running = False
        self.refresh_thread = None
        self.brightness = 255  # 0-255
        self.refresh_rate = 0  # Hz, calculated
        
        print(f"Initializing HUB75 driver for {width}×{height} matrix")
        
    def initialize_gpio(self):
        """Initialize GPIO lines using gpiod 2.x API"""
        try:
            # Pin mapping
            self.gpio_pins = {
                'R1': GPIO_R1, 'G1': GPIO_G1, 'B1': GPIO_B1,
                'R2': GPIO_R2, 'G2': GPIO_G2, 'B2': GPIO_B2,
                'A': GPIO_A, 'B': GPIO_B, 'C': GPIO_C, 'D': GPIO_D,
                'CLK': GPIO_CLK, 'LAT': GPIO_LAT, 'OE': GPIO_OE
            }
            
            # Create line settings for all pins (outputs, initially low)
            line_config = {}
            for name, pin_num in self.gpio_pins.items():
                # For gpiod 2.x, we need LineSettings objects
                try:
                    from gpiod.line import Direction, Value
                    line_config[pin_num] = gpiod.LineSettings(
                        direction=Direction.OUTPUT,
                        output_value=Value.INACTIVE
                    )
                except (ImportError, AttributeError):
                    # Fallback: try simple dict config for older versions
                    line_config[pin_num] = gpiod.LineSettings(
                        direction=gpiod.line.Direction.OUTPUT,
                        output_value=gpiod.line.Value.INACTIVE
                    )
            
            # Request all lines at once
            self.request = gpiod.request_lines(
                self.chip_path,
                consumer="hub75_driver",
                config=line_config
            )
            
            print(f"Opened GPIO chip: {self.chip_path}")
            print(f"✓ Initialized {len(self.gpio_pins)} GPIO lines")
            
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
        pin_num = self.gpio_pins[name]
        try:
            from gpiod.line import Value
            self.request.set_value(pin_num, Value.ACTIVE if value else Value.INACTIVE)
        except (ImportError, AttributeError):
            # Fallback for different API versions
            self.request.set_value(pin_num, gpiod.line.Value.ACTIVE if value else gpiod.line.Value.INACTIVE)
    
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
        self._set_pin('A', row & 0x01)
        self._set_pin('B', (row >> 1) & 0x01)
        self._set_pin('C', (row >> 2) & 0x01)
        self._set_pin('D', (row >> 3) & 0x01)
    
    def shift_out_row(self, row: int):
        """
        Shift out one row of data for both panels
        Args:
            row: Row number (0-15)
        """
        # Calculate which rows to display (top and bottom panel)
        top_row = row
        bottom_row = row + self.rows
        
        # Shift out all columns for this row
        for x in range(self.width):
            # Get pixel colors
            r1, g1, b1 = self.front_buffer[top_row][x]
            r2, g2, b2 = self.front_buffer[bottom_row][x]
            
            # Apply brightness
            r1 = (r1 * self.brightness) >> 8
            g1 = (g1 * self.brightness) >> 8
            b1 = (b1 * self.brightness) >> 8
            r2 = (r2 * self.brightness) >> 8
            g2 = (g2 * self.brightness) >> 8
            b2 = (b2 * self.brightness) >> 8
            
            # Set data lines (1-bit for now, can add PWM later)
            self._set_pin('R1', 1 if r1 > 127 else 0)
            self._set_pin('G1', 1 if g1 > 127 else 0)
            self._set_pin('B1', 1 if b1 > 127 else 0)
            self._set_pin('R2', 1 if r2 > 127 else 0)
            self._set_pin('G2', 1 if g2 > 127 else 0)
            self._set_pin('B2', 1 if b2 > 127 else 0)
            
            # Clock pulse
            self._set_pin('CLK', 1)
            self._set_pin('CLK', 0)
    
    def display_row(self, row: int):
        """Display one row on the matrix"""
        # Disable output while updating
        self._set_pin('OE', 1)
        
        # Set row address
        self.set_address(row)
        
        # Shift out data
        self.shift_out_row(row)
        
        # Latch data
        self._set_pin('LAT', 1)
        self._set_pin('LAT', 0)
        
        # Enable output
        self._set_pin('OE', 0)
    
    def refresh_loop(self):
        """Main refresh loop - scans all rows continuously"""
        print("Starting refresh loop...")
        frame_count = 0
        last_fps_time = time.time()
        
        while self.running:
            # Scan all rows
            for row in range(self.rows):
                if not self.running:
                    break
                self.display_row(row)
                # Small delay for brightness control (adjust as needed)
                time.sleep(0.0001)  # 100 microseconds per row = ~625 Hz refresh
            
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
        if self.request:
            try:
                self._set_pin('OE', 1)
            except:
                pass
        
        # Release GPIO
        if self.request:
            self.request.release()
            self.request = None
            self.gpio_pins = {}
        
        print("✓ Display stopped")
    
    def set_brightness(self, brightness: int):
        """Set brightness (0-255)"""
        self.brightness = max(0, min(255, brightness))
    
    def get_status(self) -> dict:
        """Get driver status information"""
        return {
            'running': self.running,
            'refresh_rate': round(self.refresh_rate, 1),
            'brightness': self.brightness,
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
