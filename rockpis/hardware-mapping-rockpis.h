/*
 * hardware-mapping-rockpis.h
 * 
 * Custom hardware mapping for RADXA Rock Pi S (RK3308) to use with
 * rpi-rgb-led-matrix library.
 * 
 * GPIO Pin Mapping (Linux sysfs numbers for RK3308):
 *   R1  = GPIO 16 (GPIO0_C0, physical pin 11)
 *   G1  = GPIO 17 (GPIO0_C1, physical pin 12)
 *   B1  = GPIO 18 (GPIO0_C2, physical pin 13)
 *   R2  = GPIO 19 (GPIO0_C3, physical pin 15)
 *   G2  = GPIO 20 (GPIO0_C4, physical pin 16)
 *   B2  = GPIO 21 (GPIO0_C5, physical pin 18)
 *   A   = GPIO 11 (GPIO0_B3, physical pin  3)
 *   B   = GPIO 12 (GPIO0_B4, physical pin  5)
 *   C   = GPIO 13 (GPIO0_B5, physical pin  8)  ⚠ Disable UART0 console!
 *   D   = GPIO 14 (GPIO0_B6, physical pin  7)
 *   CLK = GPIO 22 (GPIO0_C6, physical pin 22)
 *   LAT = GPIO 23 (GPIO0_C7, physical pin 19)
 *   OE  = GPIO 24 (GPIO0_D0, physical pin 21)
 * 
 * To use this mapping:
 * 1. Copy this file to: rpi-rgb-led-matrix/lib/hardware-mapping.c
 *    (append the content to the existing file)
 * 2. Recompile the library: make clean && make -j$(nproc)
 * 3. Set MATRIX_HARDWARE_MAPPING = "rockpi-s" in config.py
 * 
 * Or use the simpler approach below (see SIMPLE_APPROACH.txt)
 */

#ifdef RGBMATRIX_HARDWARE_MAPPING_C

static struct HardwareMapping rockpi_s_mapping = {
  .name          = "rockpi-s",
  
  // RGB data pins
  .output_enable = 24,  // OE
  .clock         = 22,  // CLK
  .strobe        = 23,  // LAT/STR
  
  // Row address pins
  .a             = 11,
  .b             = 12,
  .c             = 13,
  .d             = 14,
  .e             = -1,  // Not used for 1/16 scan (32px height)
  
  // Upper half RGB
  .p0_r1         = 16,
  .p0_g1         = 17,
  .p0_b1         = 18,
  
  // Lower half RGB
  .p0_r2         = 19,
  .p0_g2         = 20,
  .p0_b2         = 21,
};

// Register the mapping (add this to the hardware_mappings array in hardware-mapping.c)
// &rockpi_s_mapping,

#endif /* RGBMATRIX_HARDWARE_MAPPING_C */
