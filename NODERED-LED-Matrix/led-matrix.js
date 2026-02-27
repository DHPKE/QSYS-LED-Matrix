/**
 * Node-RED LED Matrix Controller
 * Controls 64x32 RGB LED Matrix panels via UDP
 * Compatible with QSYS-LED-Matrix RPi controller
 */

module.exports = function(RED) {
    "use strict";
    const dgram = require('dgram');

    // Main LED Matrix node
    function LEDMatrixNode(config) {
        RED.nodes.createNode(this, config);
        const node = this;

        // Configuration
        node.ip = config.ip || "192.168.1.100";
        node.port = parseInt(config.port) || 21324;
        node.command = config.command || "text";
        node.segment = parseInt(config.segment) || 0;
        
        // Text command parameters
        node.text = config.text || "";
        node.color = config.color || "FFFFFF";
        node.bgcolor = config.bgcolor || "000000";
        node.font = config.font || "arial";
        node.size = config.size || "auto";
        node.align = config.align || "C";
        node.effect = config.effect || "none";
        node.intensity = parseInt(config.intensity) || 255;
        
        // Layout/brightness parameters
        node.preset = parseInt(config.preset) || 1;
        node.brightness = parseInt(config.brightness) || 128;
        node.orientation = config.orientation || "landscape";
        
        // Group routing (1-8, or 0 for "all")
        node.group = parseInt(config.group) || 0;

        // UDP client
        let udpClient = null;

        function getClient() {
            if (!udpClient) {
                udpClient = dgram.createSocket('udp4');
                
                udpClient.on('error', (err) => {
                    node.error(`UDP error: ${err.message}`);
                    node.status({fill:"red", shape:"ring", text:"UDP error"});
                });
            }
            return udpClient;
        }

        function sendCommand(cmdObj, ip, port) {
            const client = getClient();
            const message = JSON.stringify(cmdObj);
            const buffer = Buffer.from(message);
            
            const targetIp = ip || node.ip;
            const targetPort = port || node.port;

            client.send(buffer, 0, buffer.length, targetPort, targetIp, (err) => {
                if (err) {
                    node.error(`Failed to send: ${err.message}`);
                    node.status({fill:"red", shape:"ring", text:"send failed"});
                } else {
                    node.status({fill:"green", shape:"dot", text:`sent to ${targetIp}:${targetPort}`});
                    node.log(`Sent: ${message} to ${targetIp}:${targetPort}`);
                }
            });
        }

        function buildCommand(cmd, msg) {
            let cmdObj = {};
            
            // Allow msg properties to override config
            const ip = msg.ip || node.ip;
            const port = msg.port || node.port;
            const command = msg.command || cmd || node.command;
            
            switch(command) {
                case "text":
                    cmdObj = {
                        cmd: "text",
                        seg: msg.segment !== undefined ? msg.segment : node.segment,
                        text: msg.payload !== undefined ? String(msg.payload) : (msg.text || node.text),
                        color: (msg.color || node.color).replace('#', ''),
                        bgcolor: (msg.bgcolor || node.bgcolor).replace('#', ''),
                        font: msg.font || node.font,
                        size: msg.size || node.size,
                        align: msg.align || node.align,
                        effect: msg.effect || node.effect,
                        intensity: msg.intensity !== undefined ? msg.intensity : node.intensity
                    };
                    break;

                case "clear":
                    cmdObj = {
                        cmd: "clear",
                        seg: msg.segment !== undefined ? msg.segment : node.segment
                    };
                    break;

                case "clear_all":
                    cmdObj = { cmd: "clear_all" };
                    break;

                case "brightness":
                    cmdObj = {
                        cmd: "brightness",
                        value: msg.brightness !== undefined ? msg.brightness : 
                               (msg.payload !== undefined ? msg.payload : node.brightness)
                    };
                    break;

                case "layout":
                    cmdObj = {
                        cmd: "layout",
                        preset: msg.preset !== undefined ? msg.preset : 
                                (msg.payload !== undefined ? msg.payload : node.preset)
                    };
                    break;

                case "orientation":
                    cmdObj = {
                        cmd: "orientation",
                        value: msg.orientation || (msg.payload !== undefined ? msg.payload : node.orientation)
                    };
                    break;

                case "group":
                    cmdObj = {
                        cmd: "group",
                        value: msg.group !== undefined ? msg.group : 
                               (msg.payload !== undefined ? msg.payload : node.group)
                    };
                    break;

                case "config":
                    // Manual segment configuration
                    cmdObj = {
                        cmd: "config",
                        seg: msg.segment !== undefined ? msg.segment : node.segment,
                        x: msg.x !== undefined ? msg.x : 0,
                        y: msg.y !== undefined ? msg.y : 0,
                        w: msg.w !== undefined ? msg.w : 64,
                        h: msg.h !== undefined ? msg.h : 32
                    };
                    break;

                default:
                    // Pass through custom command if msg contains full command object
                    if (msg.cmd) {
                        cmdObj = msg;
                    } else {
                        node.error(`Unknown command: ${command}`);
                        return null;
                    }
            }

            return { cmdObj, ip, port };
        }

        // Handle incoming messages
        node.on('input', function(msg) {
            const result = buildCommand(node.command, msg);
            
            if (result) {
                sendCommand(result.cmdObj, result.ip, result.port);
                
                // Pass through with command info
                msg.ledmatrix = {
                    command: result.cmdObj,
                    ip: result.ip,
                    port: result.port
                };
                node.send(msg);
            }
        });

        // Cleanup on close
        node.on('close', function() {
            if (udpClient) {
                udpClient.close();
                udpClient = null;
            }
            node.status({});
        });

        // Initial status
        node.status({fill:"yellow", shape:"ring", text:"ready"});
    }

    RED.nodes.registerType("led-matrix", LEDMatrixNode);
};
