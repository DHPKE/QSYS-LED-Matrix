const dgram = require('dgram');

/**
 * LED Matrix UDP Client for Node.js
 * Simple class to control Olimex LED Matrix via UDP
 */
class LEDMatrixClient {
    constructor(ip = '192.168.1.100', port = 21324) {
        this.ip = ip;
        this.port = port;
        this.client = dgram.createSocket('udp4');
    }

    /**
     * Send raw command to matrix
     * @param {string} command - Raw UDP command
     * @returns {Promise<void>}
     */
    sendCommand(command) {
        return new Promise((resolve, reject) => {
            const message = Buffer.from(command + '\n');
            this.client.send(message, this.port, this.ip, (err) => {
                if (err) {
                    reject(err);
                } else {
                    console.log(`✓ Sent: ${command}`);
                    resolve();
                }
            });
        });
    }

    /**
     * Display text on a segment
     * @param {number} segment - Segment ID (0-3)
     * @param {string} text - Text to display
     * @param {string} color - Hex color RRGGBB
     * @param {string} font - Font name
     * @param {string} size - Font size or "auto"
     * @param {string} align - Alignment (L/C/R)
     * @param {string} effect - Effect (none/scroll/blink/fade/rainbow)
     * @returns {Promise<void>}
     */
    async sendText(segment = 0, text = '', color = 'FFFFFF', 
                   font = 'roboto12', size = 'auto', align = 'C', effect = 'none') {
        const command = `TEXT|${segment}|${text}|${color}|${font}|${size}|${align}|${effect}`;
        return this.sendCommand(command);
    }

    /**
     * Clear a specific segment
     * @param {number} segment - Segment ID (0-3)
     * @returns {Promise<void>}
     */
    async clearSegment(segment) {
        return this.sendCommand(`CLEAR|${segment}`);
    }

    /**
     * Clear all segments
     * @returns {Promise<void>}
     */
    async clearAll() {
        return this.sendCommand('CLEAR_ALL');
    }

    /**
     * Set display brightness
     * @param {number} brightness - Brightness level (0-255)
     * @returns {Promise<void>}
     */
    async setBrightness(brightness) {
        if (brightness < 0 || brightness > 255) {
            throw new Error('Brightness must be 0-255');
        }
        return this.sendCommand(`BRIGHTNESS|${brightness}`);
    }

    /**
     * Close the UDP client
     */
    close() {
        this.client.close();
    }
}

// Example usage
async function main() {
    const matrix = new LEDMatrixClient('192.168.1.100', 21324);

    try {
        // Display "Hello World" in white
        await matrix.sendText(0, 'Hello World', 'FFFFFF', 'roboto12', 'auto', 'C', 'none');
        
        // Wait 3 seconds
        await new Promise(resolve => setTimeout(resolve, 3000));
        
        // Display red alert
        await matrix.sendText(0, 'ALERT!', 'FF0000', 'roboto24', 'auto', 'C', 'blink');
        
        // Wait 3 seconds
        await new Promise(resolve => setTimeout(resolve, 3000));
        
        // Clear display
        await matrix.clearAll();
        
        // Set brightness to 50%
        await matrix.setBrightness(128);
        
    } catch (error) {
        console.error('Error:', error);
    } finally {
        matrix.close();
    }
}

// Run if executed directly
if (require.main === module) {
    main();
}

// Export for use as module
module.exports = LEDMatrixClient;
