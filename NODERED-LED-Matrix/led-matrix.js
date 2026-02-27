/**
 * Node-RED LED Matrix Controller - Simplified Nodes
 * Multiple specialized nodes for easier handling
 */

module.exports = function(RED) {
    "use strict";
    const dgram = require('dgram');

    // Shared UDP client functionality
    const udpClients = new Map();

    function getClient(nodeId) {
        if (!udpClients.has(nodeId)) {
            const client = dgram.createSocket('udp4');
            client.on('error', (err) => {
                console.error(`UDP error for ${nodeId}:`, err.message);
            });
            udpClients.set(nodeId, client);
        }
        return udpClients.get(nodeId);
    }

    function sendCommand(node, cmdObj, ip, port) {
        const client = getClient(node.id);
        const message = JSON.stringify(cmdObj);
        const buffer = Buffer.from(message);

        client.send(buffer, 0, buffer.length, port, ip, (err) => {
            if (err) {
                node.error(`Failed to send: ${err.message}`);
                node.status({fill:"red", shape:"ring", text:"send failed"});
            } else {
                node.status({fill:"green", shape:"dot", text:`sent to ${ip}:${port}`});
            }
        });
    }

    // ========================================================================
    // LED Matrix Text Node (per-segment)
    // ========================================================================
    function LEDMatrixTextNode(config) {
        RED.nodes.createNode(this, config);
        const node = this;

        node.ip = config.ip || "192.168.1.100";
        node.port = parseInt(config.port) || 21324;
        node.segment = parseInt(config.segment) || 0;
        node.color = config.color || "FFFFFF";
        node.bgcolor = config.bgcolor || "000000";
        node.font = config.font || "arial";
        node.size = config.size || "auto";
        node.align = config.align || "C";
        node.effect = config.effect || "none";
        node.intensity = parseInt(config.intensity) || 255;

        node.on('input', function(msg) {
            const cmdObj = {
                cmd: "text",
                seg: msg.segment !== undefined ? msg.segment : node.segment,
                text: msg.payload !== undefined ? String(msg.payload) : (msg.text || ""),
                color: ((msg.color || node.color).replace('#', '')),
                bgcolor: ((msg.bgcolor || node.bgcolor).replace('#', '')),
                font: msg.font || node.font,
                size: msg.size || node.size,
                align: msg.align || node.align,
                effect: msg.effect || node.effect,
                intensity: msg.intensity !== undefined ? msg.intensity : node.intensity
            };

            const targetIp = msg.ip || node.ip;
            const targetPort = msg.port || node.port;

            sendCommand(node, cmdObj, targetIp, targetPort);

            msg.ledmatrix = { command: cmdObj, ip: targetIp, port: targetPort };
            node.send(msg);
        });

        node.on('close', function() {
            if (udpClients.has(node.id)) {
                udpClients.get(node.id).close();
                udpClients.delete(node.id);
            }
            node.status({});
        });

        node.status({fill:"yellow", shape:"ring", text:`seg ${node.segment} ready`});
    }

    // ========================================================================
    // LED Matrix Layout Node
    // ========================================================================
    function LEDMatrixLayoutNode(config) {
        RED.nodes.createNode(this, config);
        const node = this;

        node.ip = config.ip || "192.168.1.100";
        node.port = parseInt(config.port) || 21324;
        node.preset = parseInt(config.preset) || 1;

        node.on('input', function(msg) {
            const preset = msg.layout !== undefined ? msg.layout : 
                          (msg.preset !== undefined ? msg.preset : 
                          (msg.payload !== undefined ? msg.payload : node.preset));

            const cmdObj = {
                cmd: "layout",
                preset: preset
            };

            const targetIp = msg.ip || node.ip;
            const targetPort = msg.port || node.port;

            sendCommand(node, cmdObj, targetIp, targetPort);

            msg.ledmatrix = { command: cmdObj, ip: targetIp, port: targetPort };
            node.send(msg);
        });

        node.on('close', function() {
            if (udpClients.has(node.id)) {
                udpClients.get(node.id).close();
                udpClients.delete(node.id);
            }
            node.status({});
        });

        node.status({fill:"yellow", shape:"ring", text:`layout ${node.preset}`});
    }

    // ========================================================================
    // LED Matrix Brightness Node
    // ========================================================================
    function LEDMatrixBrightnessNode(config) {
        RED.nodes.createNode(this, config);
        const node = this;

        node.ip = config.ip || "192.168.1.100";
        node.port = parseInt(config.port) || 21324;
        node.brightness = parseInt(config.brightness) || 128;

        node.on('input', function(msg) {
            const value = msg.brightness !== undefined ? msg.brightness :
                         (msg.payload !== undefined ? msg.payload : node.brightness);

            const cmdObj = {
                cmd: "brightness",
                value: value
            };

            const targetIp = msg.ip || node.ip;
            const targetPort = msg.port || node.port;

            sendCommand(node, cmdObj, targetIp, targetPort);

            msg.ledmatrix = { command: cmdObj, ip: targetIp, port: targetPort };
            node.send(msg);
        });

        node.on('close', function() {
            if (udpClients.has(node.id)) {
                udpClients.get(node.id).close();
                udpClients.delete(node.id);
            }
            node.status({});
        });

        node.status({fill:"yellow", shape:"ring", text:`${node.brightness}`});
    }

    // ========================================================================
    // LED Matrix Clear Node
    // ========================================================================
    function LEDMatrixClearNode(config) {
        RED.nodes.createNode(this, config);
        const node = this;

        node.ip = config.ip || "192.168.1.100";
        node.port = parseInt(config.port) || 21324;
        node.segment = parseInt(config.segment) || 0;
        node.clearAll = config.clearAll || false;

        node.on('input', function(msg) {
            let cmdObj;
            
            if (msg.clearAll !== undefined ? msg.clearAll : node.clearAll) {
                cmdObj = { cmd: "clear_all" };
            } else {
                cmdObj = {
                    cmd: "clear",
                    seg: msg.segment !== undefined ? msg.segment : node.segment
                };
            }

            const targetIp = msg.ip || node.ip;
            const targetPort = msg.port || node.port;

            sendCommand(node, cmdObj, targetIp, targetPort);

            msg.ledmatrix = { command: cmdObj, ip: targetIp, port: targetPort };
            node.send(msg);
        });

        node.on('close', function() {
            if (udpClients.has(node.id)) {
                udpClients.get(node.id).close();
                udpClients.delete(node.id);
            }
            node.status({});
        });

        const statusText = node.clearAll ? "clear all" : `clear seg ${node.segment}`;
        node.status({fill:"yellow", shape:"ring", text: statusText});
    }

    // ========================================================================
    // LED Matrix Config Node (connection settings)
    // ========================================================================
    function LEDMatrixConfigNode(config) {
        RED.nodes.createNode(this, config);
        this.ip = config.ip;
        this.port = config.port;
    }

    // Register all node types
    RED.nodes.registerType("led-matrix-text", LEDMatrixTextNode);
    RED.nodes.registerType("led-matrix-layout", LEDMatrixLayoutNode);
    RED.nodes.registerType("led-matrix-brightness", LEDMatrixBrightnessNode);
    RED.nodes.registerType("led-matrix-clear", LEDMatrixClearNode);
    RED.nodes.registerType("led-matrix-config", LEDMatrixConfigNode);
};
