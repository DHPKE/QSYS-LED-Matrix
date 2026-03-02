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
            brightness: config.brightness !== "" ? parseInt(config.brightness) : null,
            frame: config.frame === "true" ? true : (config.frame === "false" ? false : null),
            framecolor: config.framecolor || null,
            framewidth: config.framewidth !== "" ? parseInt(config.framewidth) : null,
            curtain: config.curtain === "true" ? true : (config.curtain === "false" ? false : null),
            curtainColor: config.curtainColor || null,
            curtainEnable: config.curtainEnable === "true" ? true : (config.curtainEnable === "false" ? false : null)
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
            } else if (msg.rotation !== undefined) {
                // ROTATION command (0, 90, 180, 270 degrees)
                cmdObj = {
                    cmd: "rotation",
                    value: msg.rotation,
                    group: msg.group || 0
                };
            } else if (msg.testmode !== undefined) {
                // TEST MODE command (via HTTP POST to web API)
                const http = require('http');
                const postData = JSON.stringify({ enabled: !!msg.testmode });
                const options = {
                    hostname: node.ip,
                    port: 8080,
                    path: '/api/testmode',
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                        'Content-Length': Buffer.byteLength(postData)
                    }
                };

                const req = http.request(options, (res) => {
                    if (res.statusCode === 200) {
                        node.status({fill:"green", shape:"dot", text:`test mode ${msg.testmode ? 'ON' : 'OFF'}`});
                    } else {
                        node.status({fill:"red", shape:"ring", text:`test mode failed: ${res.statusCode}`});
                    }
                });

                req.on('error', (err) => {
                    node.error(`Test mode request failed: ${err.message}`);
                    node.status({fill:"red", shape:"ring", text:"test mode error"});
                });

                req.write(postData);
                req.end();

                msg.ledmatrix = { command: { cmd: "testmode", enabled: !!msg.testmode } };
                node.send(msg);
                return;
            } else if (msg.frame !== undefined || node.defaults.frame !== null) {
                // FRAME command (enable/disable segment border) - use defaults if message doesn't provide
                const frameValue = msg.frame !== undefined ? msg.frame : node.defaults.frame;
                
                if (frameValue !== null && frameValue !== undefined) {
                    const segValue = getValue(msg.segment !== undefined ? msg.segment : msg.seg, node.defaults.segment, 0);
                    cmdObj = {
                        cmd: "frame",
                        seg: segValue,
                        enabled: !!frameValue,
                        color: getValue(msg.framecolor, node.defaults.framecolor, "FFFFFF").replace('#', ''),
                        width: getValue(msg.framewidth, node.defaults.framewidth, 1),
                        group: msg.group || 0
                    };
                }
            } else if (msg.curtainEnable !== undefined || node.defaults.curtainEnable !== null) {
                // CURTAIN CONFIG command - configure curtain for a group
                const enableValue = msg.curtainEnable !== undefined ? msg.curtainEnable : node.defaults.curtainEnable;
                
                if (enableValue !== null && enableValue !== undefined) {
                    cmdObj = {
                        cmd: "curtain",
                        group: msg.group || 1,
                        enabled: !!enableValue,
                        color: getValue(msg.curtainColor, node.defaults.curtainColor, "FFFFFF").replace('#', '')
                    };
                }
            } else if (msg.curtain !== undefined || node.defaults.curtain !== null) {
                // CURTAIN SHOW/HIDE command - toggle visibility
                const curtainValue = msg.curtain !== undefined ? msg.curtain : node.defaults.curtain;
                
                if (curtainValue !== null && curtainValue !== undefined) {
                    cmdObj = {
                        cmd: "curtain",
                        group: msg.group || 1,
                        state: !!curtainValue
                    };
                }
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
