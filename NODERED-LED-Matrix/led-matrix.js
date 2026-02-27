/**
 * Node-RED LED Matrix Controller - Single Node with Optional Defaults
 * Node properties provide defaults, message values override
 */

module.exports = function(RED) {
    "use strict";
    const dgram = require('dgram');

    function LEDMatrixNode(config) {
        RED.nodes.createNode(this, config);
        const node = this;

        node.ip = config.ip || "10.1.1.24";
        node.port = parseInt(config.port) || 21324;
        
        // Default values from node config (empty string means "use message value")
        node.defaults = {
            segment: config.segment !== "" ? parseInt(config.segment) : null,
            color: config.color || null,
            bgcolor: config.bgcolor || null,
            font: config.font || null,
            align: config.align || null,
            intensity: config.intensity !== "" ? parseInt(config.intensity) : null,
            layout: config.layout !== "" ? parseInt(config.layout) : null,
            brightness: config.brightness !== "" ? parseInt(config.brightness) : null
        };

        // UDP client
        const client = dgram.createSocket('udp4');

        client.on('error', (err) => {
            node.error(`UDP error: ${err.message}`);
            node.status({fill:"red", shape:"ring", text:"error"});
        });

        function sendCommand(cmdObj) {
            const message = JSON.stringify(cmdObj);
            const buffer = Buffer.from(message);

            client.send(buffer, 0, buffer.length, node.port, node.ip, (err) => {
                if (err) {
                    node.error(`Failed to send: ${err.message}`);
                    node.status({fill:"red", shape:"ring", text:"send failed"});
                } else {
                    node.status({fill:"green", shape:"dot", text:`sent: ${cmdObj.cmd}`});
                }
            });
        }

        // Helper to get value with fallback to node default
        function getValue(msgValue, defaultValue, finalFallback) {
            if (msgValue !== undefined && msgValue !== null && msgValue !== "") {
                return msgValue;
            }
            if (defaultValue !== undefined && defaultValue !== null && defaultValue !== "") {
                return defaultValue;
            }
            return finalFallback;
        }

        node.on('input', function(msg) {
            let cmdObj = null;

            // Detect command type from message properties
            if (msg.text !== undefined || msg.payload !== undefined) {
                // TEXT command - use node defaults if message doesn't provide values
                cmdObj = {
                    cmd: "text",
                    seg: getValue(msg.segment !== undefined ? msg.segment : msg.seg, node.defaults.segment, 0),
                    text: msg.text !== undefined ? String(msg.text) : String(msg.payload || ""),
                    color: getValue(msg.color, node.defaults.color, "FFFFFF").replace('#', ''),
                    bgcolor: getValue(msg.bgcolor, node.defaults.bgcolor, "000000").replace('#', ''),
                    font: getValue(msg.font, node.defaults.font, "arial"),
                    size: msg.size || "auto",
                    align: getValue(msg.align, node.defaults.align, "C"),
                    effect: msg.effect || "none",
                    intensity: getValue(msg.intensity, node.defaults.intensity, 255)
                };
            } else if (msg.layout !== undefined || msg.preset !== undefined) {
                // LAYOUT command - use node default if message doesn't provide
                const layoutValue = msg.layout !== undefined ? msg.layout : 
                                   (msg.preset !== undefined ? msg.preset : node.defaults.layout);
                
                if (layoutValue !== null && layoutValue !== undefined) {
                    cmdObj = {
                        cmd: "layout",
                        preset: layoutValue
                    };
                } else {
                    node.warn("Layout command requires msg.layout or node default");
                    return;
                }
            } else if (msg.brightness !== undefined) {
                // BRIGHTNESS command - use node default if message doesn't provide
                const brightnessValue = msg.brightness !== undefined ? msg.brightness : node.defaults.brightness;
                
                if (brightnessValue !== null && brightnessValue !== undefined) {
                    cmdObj = {
                        cmd: "brightness",
                        value: brightnessValue
                    };
                } else {
                    node.warn("Brightness command requires msg.brightness or node default");
                    return;
                }
            } else if (msg.clear !== undefined) {
                // CLEAR command
                if (msg.clear === "all" || msg.clear === true) {
                    cmdObj = { cmd: "clear_all" };
                } else {
                    cmdObj = {
                        cmd: "clear",
                        seg: msg.clear
                    };
                }
            } else if (msg.orientation !== undefined) {
                // ORIENTATION command
                cmdObj = {
                    cmd: "orientation",
                    value: msg.orientation
                };
            } else if (msg.group !== undefined) {
                // GROUP command
                cmdObj = {
                    cmd: "group",
                    group: msg.group,
                    segments: msg.segments || []
                };
            } else if (msg.config !== undefined) {
                // CONFIG command
                cmdObj = {
                    cmd: "config"
                };
                if (msg.config.network !== undefined) cmdObj.network = msg.config.network;
                if (msg.config.name !== undefined) cmdObj.name = msg.config.name;
                if (msg.config.orientation !== undefined) cmdObj.orientation = msg.config.orientation;
            } else {
                node.warn("No valid command found in message. Use msg.text, msg.layout, msg.brightness, msg.clear, etc.");
                return;
            }

            sendCommand(cmdObj);

            msg.ledmatrix = { command: cmdObj };
            node.send(msg);
        });

        node.on('close', function() {
            client.close();
            node.status({});
        });

        // Show status with defaults info
        let statusText = `ready @ ${node.ip}:${node.port}`;
        if (node.defaults.segment !== null) {
            statusText += ` | seg${node.defaults.segment}`;
        }
        node.status({fill:"yellow", shape:"ring", text: statusText});
    }

    RED.nodes.registerType("led-matrix", LEDMatrixNode);
};
