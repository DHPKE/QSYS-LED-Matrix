"""
GPIO Configuration for Rock Pi S + HUB75 64×32 LED Matrix
Uses libgpiod for modern GPIO access
"""

# Rock Pi S GPIO chip (RK3308 has multiple GPIO banks)
GPIO_CHIP = "/dev/gpiochip0"  # May need adjustment, check with 'gpiodetect'

# HUB75 Pin Mapping - All on Header 1
# Format: (gpio_number, physical_pin, description)
GPIO_PINS = {
    # RGB Data - Panel 1 (top half)
    'R1': (16, 13, 'GPIO0_C0'),  # Red data 1
    'G1': (17, 15, 'GPIO0_C1'),  # Green data 1
    'B1': (15, 11, 'GPIO0_B7'),  # Blue data 1
    
    # RGB Data - Panel 2 (bottom half)
    'R2': (68, 7, 'GPIO2_A4'),   # Red data 2
    'G2': (69, 12, 'GPIO2_A5'),  # Green data 2
    'B2': (74, 16, 'GPIO2_B2'),  # Blue data 2
    
    # Row Address Lines (select 1 of 16 rows)
    'A': (11, 3, 'GPIO0_B3'),    # Address bit 0
    'B': (12, 5, 'GPIO0_B4'),    # Address bit 1
    'C': (65, 8, 'GPIO2_A1'),    # Address bit 2 (UART TX - disable console!)
    'D': (64, 10, 'GPIO2_A0'),   # Address bit 3 (UART RX - disable console!)
    
    # Control Signals
    'CLK': (71, 22, 'GPIO2_A7'), # Shift register clock
    'LAT': (55, 19, 'GPIO1_C7'), # Latch (display data)
    'OE': (54, 21, 'GPIO1_C6'),  # Output enable (active low)
}

# Extract just the GPIO numbers for easy access
GPIO_R1 = 16
GPIO_G1 = 17
GPIO_B1 = 15
GPIO_R2 = 68
GPIO_G2 = 69
GPIO_B2 = 74
GPIO_A = 11
GPIO_B = 12
GPIO_C = 65
GPIO_D = 64
GPIO_CLK = 71
GPIO_LAT = 55
GPIO_OE = 54

# All GPIO numbers in order for bulk operations
ALL_GPIOS = [GPIO_R1, GPIO_G1, GPIO_B1, GPIO_R2, GPIO_G2, GPIO_B2,
             GPIO_A, GPIO_B, GPIO_C, GPIO_D, GPIO_CLK, GPIO_LAT, GPIO_OE]

# Matrix dimensions
MATRIX_WIDTH = 64
MATRIX_HEIGHT = 32
MATRIX_ROWS = 16  # Each of the 2 panels shows 16 rows simultaneously

def get_gpio_info():
    """Return formatted GPIO pin mapping for debugging"""
    info = []
    info.append("Rock Pi S GPIO Pin Mapping for HUB75")
    info.append("=" * 50)
    info.append(f"{'Signal':<6} {'GPIO#':<6} {'Pin':<4} {'Description':<12}")
    info.append("-" * 50)
    
    for signal, (gpio, pin, desc) in sorted(GPIO_PINS.items(), key=lambda x: x[1][1]):
        info.append(f"{signal:<6} {gpio:<6} {pin:<4} {desc:<12}")
    
    return "\n".join(info)

if __name__ == "__main__":
    print(get_gpio_info())
