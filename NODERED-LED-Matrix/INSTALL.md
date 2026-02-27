# Installation Guide

## Prerequisites

- Node-RED installed and running
- Access to LED Matrix controller (RPi with QSYS-LED-Matrix)
- Network connectivity between Node-RED and the controller

---

## Installation Methods

### Method 1: Install from npm (Recommended - when published)

```bash
cd ~/.node-red
npm install node-red-contrib-led-matrix
```

Restart Node-RED:
```bash
node-red-restart
```

---

### Method 2: Install from Local Directory

If you have cloned the QSYS-LED-Matrix repository:

```bash
cd ~/.node-red
npm install /path/to/QSYS-LED-Matrix/NODERED-LED-Matrix
```

Example:
```bash
cd ~/.node-red
npm install ~/projects/QSYS-LED-Matrix/NODERED-LED-Matrix
```

Restart Node-RED:
```bash
node-red-restart
```

---

### Method 3: Manual Symlink (Development)

For development or if you want to keep the source in a specific location:

```bash
cd ~/.node-red/node_modules
ln -s /path/to/QSYS-LED-Matrix/NODERED-LED-Matrix node-red-contrib-led-matrix
```

Restart Node-RED:
```bash
node-red-restart
```

---

### Method 4: Install from GitHub

Direct installation from the repository:

```bash
cd ~/.node-red
npm install github:DHPKE/QSYS-LED-Matrix#main
```

Note: This installs the entire repository. The node files are in the `NODERED-LED-Matrix` subdirectory.

---

## Verification

After installation and restart:

1. Open Node-RED in your browser (usually http://localhost:1880)
2. Check the left panel (node palette)
3. Look for "led-matrix" in the **output** category
4. The node should have a blue color with a desktop icon

### Check Installation via Command Line

```bash
cd ~/.node-red
npm list node-red-contrib-led-matrix
```

Expected output:
```
~/.node-red
└── node-red-contrib-led-matrix@1.0.0
```

---

## Configuration

### First-Time Setup

1. **Drag** the "led-matrix" node onto the canvas
2. **Double-click** to open the configuration panel
3. **Set** the IP address of your LED Matrix controller (e.g., `10.1.1.24`)
4. **Set** the UDP port (default: `21324`)
5. **Select** a command type (e.g., "Text")
6. **Configure** any default parameters
7. **Click** "Done"

### Network Configuration

Ensure the LED Matrix controller is reachable:

```bash
ping 10.1.1.24
```

Check the UDP port is accessible (no firewall blocking):

```bash
nc -u -v 10.1.1.24 21324
```

---

## Import Example Flows

The package includes example flows in `examples.json`.

### Import via Node-RED UI:

1. Open Node-RED
2. Click the menu (☰) → Import
3. Select "Clipboard"
4. Paste the contents of `examples.json`
5. Click "Import"

### Manual Import:

Copy `examples.json` contents and paste into Node-RED import dialog.

---

## Testing

### Quick Test

1. Import the examples or create a simple flow:
   - **Inject node** → set payload to "Hello"
   - **LED Matrix node** → configure IP and port, command = "text"
   - **Deploy** the flow
   - **Click** the inject node button

2. Check the LED Matrix display for the text "Hello"

3. Check Node-RED debug panel for any errors

### Troubleshooting Test Issues

**No output on display:**
- Verify IP address and port
- Check network connectivity: `ping <ip>`
- Verify LED Matrix service is running on the controller:
  ```bash
  ssh node@10.1.1.24
  sudo systemctl status led-matrix
  ```

**Node-RED shows UDP errors:**
- Check firewall settings
- Verify the controller is listening on the configured port:
  ```bash
  ssh node@10.1.1.24
  sudo netstat -ulnp | grep 21324
  ```

**Node not appearing in palette:**
- Check installation: `npm list node-red-contrib-led-matrix`
- Check Node-RED logs: `~/.node-red/node-red.log`
- Restart Node-RED: `node-red-restart`

---

## Upgrading

### From npm:
```bash
cd ~/.node-red
npm update node-red-contrib-led-matrix
node-red-restart
```

### From local/GitHub:
```bash
cd ~/.node-red
npm uninstall node-red-contrib-led-matrix
npm install /path/to/QSYS-LED-Matrix/NODERED-LED-Matrix
node-red-restart
```

---

## Uninstalling

```bash
cd ~/.node-red
npm uninstall node-red-contrib-led-matrix
node-red-restart
```

---

## Docker / Container Installations

If running Node-RED in Docker:

### Docker Compose

Add a volume mount for the node:

```yaml
services:
  node-red:
    image: nodered/node-red:latest
    volumes:
      - ./NODERED-LED-Matrix:/data/node_modules/node-red-contrib-led-matrix
    ports:
      - "1880:1880"
```

### Dockerfile

```dockerfile
FROM nodered/node-red:latest
WORKDIR /data
RUN npm install node-red-contrib-led-matrix
```

---

## Platform-Specific Notes

### Raspberry Pi

If running Node-RED on the same Raspberry Pi as the LED Matrix controller:

- Use `localhost` or `127.0.0.1` as the IP address
- No network configuration needed
- Lower latency for commands

### Windows

- Use backslashes for paths when installing from local directory:
  ```cmd
  npm install C:\path\to\QSYS-LED-Matrix\NODERED-LED-Matrix
  ```

### macOS

- Standard instructions work as-is
- May need sudo for global Node-RED installations:
  ```bash
  sudo npm install -g node-red-contrib-led-matrix
  ```

---

## Development Setup

For contributors or developers:

```bash
# Clone the repository
git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
cd QSYS-LED-Matrix/NODERED-LED-Matrix

# Link to Node-RED
npm link

# In Node-RED directory
cd ~/.node-red
npm link node-red-contrib-led-matrix

# Restart Node-RED
node-red-restart
```

Changes to the source files will be reflected immediately (after restart).

---

## Support

- **GitHub Issues:** https://github.com/DHPKE/QSYS-LED-Matrix/issues
- **Documentation:** See README.md in the same directory
- **Examples:** See examples.json for sample flows

---

**Ready to go! Start building amazing LED Matrix displays with Node-RED! 🚀**
