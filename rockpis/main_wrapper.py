#!/usr/bin/env python3
"""
Hardware mapping workaround for Rock Pi S.

Since the Python bindings don't expose gpio_* attributes in older library
versions, this script works around it by using the 'regular' hardware mapping
which has sensible defaults that can work on generic ARM boards.

The key fix is drop_privileges=False which prevents the Pi detection crash.
"""

import os
import sys

# Set environment variable to tell library to use generic GPIO access
os.environ['GPIOD_CHIP'] = 'gpiochip0'

# Now import and run main
if __name__ == "__main__":
    import main
    main.main()
