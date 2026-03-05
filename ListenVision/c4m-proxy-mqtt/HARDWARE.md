# Hardware Requirements for C4M Proxy

## Question: Can a Raspberry Pi handle 40+ C4M devices?

**Short answer:** Use **RPi 4 (4GB+)** minimum, but **Proxmox VM or bare metal server is better**.

## Performance Analysis

### Raspberry Pi Options

| Model | CPU | RAM | Est. Max Devices | Verdict |
|-------|-----|-----|------------------|---------|
| **RPi Zero 2 W** | 1 GHz quad ARM | 512 MB | 5-10 | ❌ Too slow |
| **RPi 3 B+** | 1.4 GHz quad ARM | 1 GB | 10-15 | ⚠️ Marginal |
| **RPi 4 (4GB)** | 1.8 GHz quad ARM | 4 GB | 40-50 | ✅ Adequate |
| **RPi 4 (8GB)** | 1.8 GHz quad ARM | 8 GB | 50-70 | ✅ Good |
| **RPi 5 (8GB)** | 2.4 GHz quad ARM | 8 GB | 100+ | ✅✅ Excellent |

### Server Options

| Platform | Specs | Est. Max Devices | Cost | Verdict |
|----------|-------|------------------|------|---------|
| **Proxmox VM** | 2 vCPU / 4GB RAM | 100-200 | ~$0 (if existing) | ✅✅ Recommended |
| **Bare Metal** | 4 cores / 8GB RAM | 500+ | $200-500 | ✅✅✅ Best |
| **Cloud VM** | 2 vCPU / 4GB RAM | 100-200 | $20-40/mo | ✅ Good for remote |

## Workload Analysis for 40 Devices @ 10Hz

### CPU Requirements

**Per device update:**
1. Canvas rendering: ~2-5ms
2. SDK program compilation: ~5-10ms
3. Network send: ~2-5ms
4. **Total: ~10-20ms per device**

**40 devices:**
- Sequential: 400-800ms per full cycle
- Actual update rate: ~1.25-2.5 Hz full cycle
- **Per device sees:** ~10Hz (due to MQTT batching)

**CPU Load:**
- Canvas rendering: ~30-40% CPU
- MQTT/Node.js: ~10-20% CPU
- SDK calls: ~20-30% CPU
- **Total: 60-90% CPU** on 4-core system

### RAM Requirements

**Memory breakdown:**
- Node.js base: ~50 MB
- MQTT client: ~10 MB
- Canvas (64×32 per device): ~6 KB × 40 = ~240 KB
- Render state: ~10 KB × 40 = ~400 KB
- SDK library: ~20 MB
- OS overhead: ~500 MB
- **Total: ~600-800 MB**

**Recommended: 2-4 GB RAM**

### Network Requirements

**Bandwidth (40 devices @ 10Hz):**
- MQTT traffic: ~20 Mbps sustained
- C4M SDK traffic: ~5-15 Mbps sustained
- **Total: ~25-35 Mbps**

**Recommendation:** Gigabit Ethernet (1000 Mbps)

## Recommended Setups

### Budget Option: Raspberry Pi 4 (8GB)

**Specs:**
- RPi 4 Model B (8GB RAM)
- Good quality SD card (64GB A2 class)
- Official power supply
- Gigabit Ethernet (built-in)

**Cost:** ~$75-100

**Pros:**
✅ Low power consumption  
✅ Compact  
✅ Easy to deploy  
✅ Fanless (with heatsink)  

**Cons:**
❌ SD card can fail  
❌ Limited to ~50 devices max  
❌ No easy backup/snapshot  
❌ Harder to scale up  

**Performance:**
- 40 devices: ✅ Good
- 10Hz updates: ✅ Achievable
- Headroom: Limited

---

### Recommended: Proxmox VM

**Specs:**
- 2 vCPUs (or 4 for headroom)
- 4 GB RAM
- 20 GB disk (thin provisioned)
- Ubuntu 22.04 LTS or Debian 12
- Virtio network driver

**Cost:** $0 (if you have Proxmox already)

**Pros:**
✅✅ Easy snapshots/backups  
✅✅ Can scale CPU/RAM without reinstall  
✅ High availability (migrate VM)  
✅ Better monitoring integration  
✅ No SD card failure risk  
✅ Easier to manage remotely  

**Cons:**
⚠️ Requires existing Proxmox infrastructure

**Performance:**
- 40 devices: ✅✅ Excellent
- 10Hz updates: ✅ Easy
- Headroom: Good
- Can scale to 100+ devices

---

### Enterprise: Bare Metal Server

**Specs:**
- Intel i5/i7 or AMD Ryzen 5/7 (4+ cores)
- 8-16 GB RAM
- 128 GB SSD
- Gigabit or 10G network
- Ubuntu 22.04 LTS Server

**Cost:** $200-500 (mini PC like Intel NUC)

**Pros:**
✅✅✅ Best performance  
✅✅ Can handle 100+ devices  
✅ Room for other services  
✅ No virtualization overhead  
✅ Maximum reliability  

**Cons:**
❌ Higher cost  
❌ More power consumption  
❌ Larger form factor  

**Performance:**
- 40 devices: ✅✅✅ Easy
- 10Hz updates: ✅✅ Smooth
- Headroom: Excellent
- Can scale to 200+ devices

## Storage Considerations

### SD Card (RPi)

**Risks:**
- ❌ Can fail with frequent writes
- ❌ Logs can wear out card
- ❌ Corruption on power loss

**Mitigation:**
- Use high-quality A2-rated cards
- Mount `/var/log` as tmpfs (RAM)
- Regular backups
- Consider USB boot instead

### SSD (VM/Server)

**Advantages:**
- ✅ Much more reliable
- ✅ Faster performance
- ✅ Better for logs/writes
- ✅ Easier backups/snapshots

**Recommendation:** Always use SSD for production

## Network Considerations

### For 40 Devices

**Required:**
- Gigabit switch (1000 Mbps)
- Gigabit uplink to proxy server
- Dedicated VLAN for LED traffic (optional but recommended)

**Bandwidth:**
- Peak: ~35 Mbps
- Average: ~20 Mbps
- Burst: ~50 Mbps

**Don't use:**
- ❌ 100 Mbps ("Fast Ethernet") - too slow
- ❌ WiFi - too unreliable for 40 devices

## Power Considerations

### Raspberry Pi 4
- Idle: ~4W
- Load: ~8W
- 24/7 cost: ~$8/year (at $0.12/kWh)

### Mini PC Server
- Idle: ~15W
- Load: ~30W
- 24/7 cost: ~$32/year

### VM (depends on host)
- Negligible additional power

## Reliability Comparison

| Platform | MTBF | Backup | Recovery | Uptime |
|----------|------|--------|----------|--------|
| **RPi + SD Card** | ~2 years | Manual | Hours | 98% |
| **RPi + USB SSD** | ~5 years | Manual | Hours | 99% |
| **Proxmox VM** | ~5 years | Snapshot | Minutes | 99.5% |
| **Bare Metal + RAID** | ~7 years | Snapshot/rsync | Minutes | 99.9% |

## Our Recommendation for 40 C4M Devices

### Best Overall: **Proxmox VM**

**Why:**
1. ✅ Perfect performance for 40-100 devices
2. ✅ Easy to backup/snapshot before changes
3. ✅ Can add CPU/RAM if you scale up
4. ✅ Can migrate to another Proxmox node
5. ✅ No hardware to buy (if Proxmox exists)
6. ✅ Professional deployment

**Proxmox VM Configuration:**
```
OS: Ubuntu 22.04 LTS Server
CPU: 2 cores (or 4 with headroom)
RAM: 4 GB
Disk: 20 GB (thin provisioned)
Network: VirtIO, bridged to VLAN
```

### Budget Alternative: **Raspberry Pi 4 (8GB)**

**Why:**
1. ✅ Will work for 40 devices
2. ✅ Low cost
3. ✅ Low power
4. ⚠️ But limited scalability

**Important:**
- Use quality A2-rated SD card OR
- Use USB SSD boot (much better!)
- Set up log rotation
- Regular backups

### Don't Use: Raspberry Pi Zero

**Why not:**
- ❌ Only 512 MB RAM
- ❌ Slow CPU
- ❌ Max ~5-10 devices realistically

## Installation Examples

### Proxmox VM Setup

```bash
# Create VM in Proxmox web UI:
# - 2 vCPUs
# - 4 GB RAM
# - 20 GB disk
# - Ubuntu 22.04 ISO

# SSH into VM
sudo apt update && sudo apt upgrade -y
sudo apt install -y nodejs npm mosquitto git

# Install proxy
cd /opt
sudo git clone https://github.com/DHPKE/QSYS-LED-Matrix.git
cd QSYS-LED-Matrix/ListenVision/c4m-proxy-mqtt
sudo npm install

# Configure
sudo cp config.example.json config.json
sudo nano config.json  # Add your 40 devices

# Create systemd service
sudo nano /etc/systemd/system/c4m-proxy.service
```

### Raspberry Pi 4 Setup

```bash
# Flash Ubuntu Server 22.04 ARM64 to SD card
# Boot RPi and SSH in

# Update system
sudo apt update && sudo apt upgrade -y

# Install dependencies
sudo apt install -y nodejs npm mosquitto git

# Same as above...
```

## Performance Testing

Before deploying to production:

1. **Start with 5 devices**
   - Verify rendering works
   - Check latency (<100ms)
   - Monitor CPU/RAM

2. **Scale to 20 devices**
   - Test simultaneous updates
   - Monitor network bandwidth
   - Check for errors

3. **Full 40 devices**
   - Stress test with test client
   - Monitor for 24 hours
   - Check system stability

## Monitoring

**Watch these metrics:**
- CPU usage (should stay <80%)
- RAM usage (should stay <70%)
- Network bandwidth (~20-35 Mbps)
- Update latency (aim for <150ms)
- Error rate (should be near 0)

**Tools:**
- Proxmox built-in graphs
- `htop` for CPU/RAM
- `iftop` for network
- Web UI at :8080 for proxy stats

## Summary

| Your Situation | Recommendation |
|----------------|----------------|
| **Have Proxmox already** | Use VM (best option) |
| **Budget deployment** | RPi 4 8GB (with USB SSD!) |
| **Professional/enterprise** | Bare metal server |
| **Don't have infrastructure** | RPi 4 8GB or cheap mini PC |

**Bottom line for 40 devices:**
- ✅✅ **Proxmox VM** - Best choice if available
- ✅ **RPi 4 (8GB)** - Will work, adequate performance
- ❌ **RPi Zero** - Don't even try

For the MQTT proxy with web UI, I'd go with **Proxmox VM (2 vCPU / 4GB RAM)** if you have it. Otherwise **RPi 4 (8GB) with USB SSD boot**.
