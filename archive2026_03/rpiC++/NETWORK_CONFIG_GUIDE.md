# Network Configuration Guide

## Web UI Access

The LED Matrix Controller now includes a built-in web configuration interface!

**Access it at:** `http://<pi-ip-address>:8080`

Example: http://10.1.1.22:8080

## Features

### Network Mode
- **DHCP** (default): Automatic IP assignment from your router
- **Static IP**: Manual IP configuration

### Static IP Configuration
When you select Static mode, you can configure:
- **Static IP Address**: Fixed IP for the Pi (e.g., `10.1.1.22`)
- **Subnet Mask**: Usually `255.255.255.0` for home networks
- **Gateway**: Your router's IP (e.g., `10.1.1.1`)

### UDP Port
- Configure the UDP port for QSYS communication (default: `21324`)
- Useful if you need to run multiple controllers or avoid port conflicts

## How It Works

1. Open http://<pi-ip>:8080 in any web browser
2. Toggle between DHCP / Static
3. Fill in network details (if using Static)
4. Set UDP port if needed
5. Click **Save & Apply**
6. **Reboot the Pi** for network changes to take effect: `sudo reboot`

## Configuration Storage

Settings are saved to: `/var/lib/led-matrix/network-config.json`

Example config:
```json
{
    "mode": "static",
    "staticIP": "10.1.1.22",
    "subnet": "255.255.255.0",
    "gateway": "10.1.1.1",
    "udpPort": 21324
}
```

## On Boot

The `led-matrix-network.service` runs at boot and applies your saved network configuration automatically by modifying `/etc/dhcpcd.conf`.

## Manual Configuration

You can also edit the config file directly and reboot:

```bash
sudo nano /var/lib/led-matrix/network-config.json
sudo reboot
```

## Troubleshooting

### Can't access the web UI?
1. Check the service is running: `sudo systemctl status led-matrix`
2. Check the current IP: `hostname -I`
3. Try the current IP on port 8080

### Changed to static IP and lost connection?
1. Connect a monitor or serial console
2. Check the current IP: `hostname -I`
3. Use the new IP to access the web UI

### Reset to DHCP?
```bash
sudo rm /var/lib/led-matrix/network-config.json
sudo sed -i '/^interface eth0/,/^$/d' /etc/dhcpcd.conf
sudo reboot
```

## Security Note

The web UI has **no authentication** — it's designed for local network use only. Don't expose port 8080 to the internet.

For production deployments, consider:
- Firewall rules to restrict access
- VPN or SSH tunnel for remote config
- Running it only when needed (not as a service)
