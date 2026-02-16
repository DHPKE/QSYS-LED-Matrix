# Contributing to Olimex LED Matrix

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## How to Contribute

### Reporting Bugs

1. **Check existing issues** - Search to see if the bug has already been reported
2. **Create a new issue** with:
   - Clear, descriptive title
   - Detailed description of the problem
   - Steps to reproduce
   - Expected vs actual behavior
   - Hardware details (ESP32 variant, matrix size, etc.)
   - Firmware version
   - Serial monitor output (if applicable)

### Suggesting Features

1. **Check existing feature requests** - Someone may have already suggested it
2. **Create a new issue** with:
   - Clear description of the feature
   - Use case and benefits
   - Proposed implementation (optional)
   - Examples of similar features in other projects

### Contributing Code

1. **Fork the repository**
2. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make your changes**:
   - Follow the coding style (see below)
   - Add comments for complex logic
   - Update documentation if needed
4. **Test your changes**:
   - Build and upload firmware
   - Test all affected features
   - Verify no regressions
5. **Commit your changes**:
   ```bash
   git commit -m "Add feature: brief description"
   ```
6. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```
7. **Create a Pull Request**:
   - Clear title and description
   - Link any related issues
   - Include screenshots/videos if UI changes

## Coding Style

### C/C++ Code

- **Indentation**: 4 spaces (no tabs)
- **Naming**:
  - Classes: `PascalCase` (e.g., `TextRenderer`)
  - Functions: `camelCase` (e.g., `updateSegmentText`)
  - Variables: `snake_case` (e.g., `segment_id`)
  - Constants: `UPPER_CASE` (e.g., `MAX_SEGMENTS`)
- **Braces**: Opening brace on same line
  ```cpp
  if (condition) {
      // code
  }
  ```
- **Comments**: Use `//` for single line, `/* */` for multi-line
- **Headers**: Include guards using `#ifndef`/`#define`

### Example

```cpp
// Good
class SegmentManager {
private:
    int segment_count;
    
public:
    void updateSegment(int id, const char* text) {
        if (id < MAX_SEGMENTS) {
            // Update logic here
        }
    }
};

// Avoid
class segment_manager {
  int SegmentCount;
  void UpdateSegment( int ID,const char *Text ){
    if( ID<MAX_SEGMENTS )
    {
      // Update logic here
    }
  }
};
```

### JavaScript Code

- **Indentation**: 2 spaces
- **Naming**: `camelCase` for functions and variables
- **Semicolons**: Use them
- **Quotes**: Single quotes for strings
- **Modern syntax**: Use ES6+ features

### Python Code

- **Style**: Follow PEP 8
- **Indentation**: 4 spaces
- **Naming**: `snake_case` for functions and variables
- **Docstrings**: Use for functions and classes

## Documentation

- Update README.md if adding features
- Update relevant docs/ files
- Add examples if applicable
- Comment complex code sections
- Update CHANGELOG.md

## Testing

### Before Submitting PR

1. **Build test** (Arduino IDE):
   - Open the sketch in Arduino IDE
   - Click Verify (✓) to compile
   - Check for compilation errors

2. **Upload test**:
   - Connect Olimex ESP32 Gateway
   - Upload to hardware
   - Verify serial output shows no errors

3. **Functional test**:
   - Test all modified features
   - Test UDP commands
   - Test web interface
   - Verify no regressions

4. **Documentation test**:
   - Check all links work
   - Verify examples run correctly
   - Ensure instructions are clear

### Test Coverage

Try to test:
- Normal operation
- Edge cases (empty strings, max values, etc.)
- Error conditions (invalid input, network issues, etc.)
- Different configurations (matrix sizes, segments, etc.)

## Pull Request Guidelines

### Good PR

- **Focused**: Addresses one feature/bug
- **Tested**: All changes have been tested
- **Documented**: README and docs updated
- **Clean**: No unrelated changes
- **Descriptive**: Clear title and description

### PR Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation update
- [ ] Refactoring

## Testing
- [ ] Builds successfully
- [ ] Tested on hardware
- [ ] Examples run correctly
- [ ] Documentation updated

## Related Issues
Fixes #(issue number)

## Screenshots (if applicable)
Add screenshots showing UI changes or output

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Comments added for complex code
- [ ] Documentation updated
- [ ] No breaking changes (or documented)
```

## Areas for Contribution

### High Priority

- Additional font support
- More text effects (rainbow, fade)
- Advanced segment layouts
- Performance optimizations
- Battery/power monitoring
- OTA (Over-The-Air) updates

### Medium Priority

- Additional platform plugins (Home Assistant, Node-RED)
- More example scripts (Ruby, Go, etc.)
- Custom font upload via web interface
- Animation sequences
- Image/icon display
- Sound-reactive features

### Low Priority

- Alternative matrix libraries
- Support for different matrix sizes
- Multi-panel support (chaining)
- Persistent display state
- Scheduled messages

## Community Guidelines

- Be respectful and professional
- Help others when possible
- Provide constructive feedback
- Stay on topic in issues/PRs
- Follow the code of conduct

## Getting Help

- **Questions**: Open a GitHub Discussion
- **Bugs**: Open an Issue
- **Feature requests**: Open an Issue with [Feature Request] tag
- **Documentation**: Check docs/ folder first

## Development Setup

### Requirements

- **Arduino IDE** 1.8.19+ or 2.x
- **ESP32 board support** (version 2.0.0+)
- **Required libraries** (see docs/ARDUINO_SETUP.md)
- **Git**
- **Hardware**: Olimex ESP32 Gateway + LED Matrix (for hardware testing)

### Setup Steps

1. **Clone repository**:
   ```bash
   git clone https://github.com/DHPKE/OlimexLED-Matrix.git
   cd OlimexLED-Matrix
   ```

2. **Install Arduino IDE**:
   - Download from [arduino.cc](https://www.arduino.cc/en/software)
   - Install ESP32 board support
   - Install required libraries (see [docs/ARDUINO_SETUP.md](docs/ARDUINO_SETUP.md))

3. **Open project**:
   - Launch Arduino IDE
   - Open `arduino/OlimexLED-Matrix/OlimexLED-Matrix.ino`
   - All header files should appear as tabs

4. **Configure**:
   - Edit `config.h` with your WiFi credentials
   - Select "ESP32 Dev Module" board
   - Configure board settings (see docs/ARDUINO_SETUP.md)

5. **Build and Upload**:
   - Click Verify (✓) to compile
   - Click Upload (→) to flash to hardware
   - Open Serial Monitor (115200 baud) to verify

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Questions?

Open a GitHub Discussion or Issue if you have questions about contributing.

---

Thank you for contributing to Olimex LED Matrix! 🎉
