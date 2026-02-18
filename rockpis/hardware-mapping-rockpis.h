/*
 * hardware-mapping-rockpis.h
 * 
 * Custom hardware mapping for RADXA Rock Pi S (RK3308) to use with
 * rpi-rgb-led-matrix library.
 * 
 * GPIO Pin Mapping (Linux GPIO numbers for RK3308, Header 1 only):
 *   R1  = GPIO 16 (GPIO0_C0, Header 1 pin 13)
 *   G1  = GPIO 17 (GPIO0_C1, Header 1 pin 15)
 *   B1  = GPIO 15 (GPIO0_B7, Header 1 pin 11)
 *   R2  = GPIO 68 (GPIO2_A4, Header 1 pin  7)
 *   G2  = GPIO 69 (GPIO2_A5, Header 1 pin 12)
 *   B2  = GPIO 74 (GPIO2_B2, Header 1 pin 16)
 *   A   = GPIO 11 (GPIO0_B3, Header 1 pin  3)
 *   B   = GPIO 12 (GPIO0_B4, Header 1 pin  5)
 *   C   = GPIO 65 (GPIO2_A1, Header 1 pin  8)  ⚠ UART0_TX - Disable console!
 *   D   = GPIO 64 (GPIO2_A0, Header 1 pin 10)  ⚠ UART0_RX - Disable console!
 *   CLK = GPIO 71 (GPIO2_A7, Header 1 pin 22)
 *   LAT = GPIO 55 (GPIO1_C7, Header 1 pin 19)
 *   OE  = GPIO 54 (GPIO1_C6, Header 1 pin 21)
 * 
 * IMPORTANT: Disable UART0 console on pins 8 & 10:
 *   sudo systemctl disable --now serial-getty@ttyS0
 *   Edit /boot/armbianEnv.txt - remove console=ttyS0,1500000
 *   sudo reboot
 * 
 * To use this mapping:
 * 1. Copy this file to: ~/rpi-rgb-led-matrix/include/
 * 2. Edit lib/hardware-mapping.c and add the registration
 * 3. Recompile: cd ~/rpi-rgb-led-matrix && make clean && make -j$(nproc)
 * 4. Reinstall Python bindings: cd bindings/python && sudo make install
 * 5. Set MATRIX_HARDWARE_MAPPING = "rockpis" in config.py
 * 6. Remove NO_DISPLAY override and restart service
 */

#ifdef RGBMATRIX_HARDWARE_MAPPING_C

static struct HardwareMapping rockpis_mapping = {
  .name          = "rockpis",
  
  // Control signals
  .output_enable = 54,  // OE  - GPIO1_C6
  .clock         = 71,  // CLK - GPIO2_A7
  .strobe        = 55,  // LAT - GPIO1_C7
  
  // Row address pins
  .a             = 11,  // GPIO0_B3
  .b             = 12,  // GPIO0_B4
  .c             = 65,  // GPIO2_A1 (UART0_TX)
  .d             = 64,  // GPIO2_A0 (UART0_RX)
  .e             = -1,  // Not used for 1/16 scan (32px height)
  
  // Upper half RGB
  .p0_r1         = 16,  // GPIO0_C0
  .p0_g1         = 17,  // GPIO0_C1
  .p0_b1         = 15,  // GPIO0_B7
  
  // Lower half RGB
  .p0_r2         = 68,  // GPIO2_A4
  .p0_g2         = 69,  // GPIO2_A5
  .p0_b2         = 74,  // GPIO2_B2
};

// Register the mapping (add this to the hardware_mappings array in hardware-mapping.c)
// Example in hardware-mapping.c:
//   static HardwareMapping *hardware_mappings[] = {
//     &adafruit_hat_mapping,
//     &adafruit_hat_pwm_mapping,
//     &regular_pi_mapping,
//     &rockpis_mapping,  // <-- Add this line
//     NULL
//   };

#endif /* RGBMATRIX_HARDWARE_MAPPING_C */
