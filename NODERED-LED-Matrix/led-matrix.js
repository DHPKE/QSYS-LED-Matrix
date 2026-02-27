/**
 * Node-RED LED Matrix Controller - Single Node, Simplified Commands
 * Flat message structure: msg.layout, msg.brightness, msg.text, etc.
 */

module.exports = function(RED) {
    "use strict";
    const dgram = require('dgram');

    function LEDMatrixNode(config) {
        RED.nodes.createNode(this, config);
        const node = this;

        node.ip = config.ip || "10.1.1.24";
        node.port = parseInt(config.port) || 21324;

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

        node.on('input', function(msg) {
            let cmdObj = null;

            // Detect command type from message properties
            if (msg.text !== undefined || msg.payload !== undefined) {
                // TEXT command
                cmdObj = {
                    cmd: "text",
                    seg: msg.segment !== undefined ? msg.segment : (msg.seg !== undefined ? msg.seg : 0),
                    text: msg.text !== undefined ? String(msg.text) : String(msg.payload || ""),
                    color: (msg.color || "FFFFFF").replace('#', ''),
                    bgcolor: (msg.bgcolor || "000000").replace('#', ''),
                    font: msg.font || "arial",
                    size: msg.size || "auto",
                    align: msg.align || "C",
                    effect: msg.effect || "none",
                    intensity: msg.intensity !== undefined ? msg.intensity : 255
                };
            } else if (msg.layout !== undefined || msg.preset !== undefined) {
                // LAYOUT command
                cmdObj = {
                    cmd: "layout",
                    preset: msg.layout !== undefined ? msg.layout : msg.preset
                };
            } else if (msg.brightness !== undefined) {
                // BRIGHTNESS command
                cmdObj = {
                    cmd: "brightness",
                    value: msg.brightness
                };
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

        node.status({fill:"yellow", shape:"ring", text:`ready @ ${node.ip}:${node.port}`});
    }

    RED.nodes.registerType("led-matrix", LEDMatrixNode);
};
