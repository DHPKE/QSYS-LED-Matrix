#!/bin/bash
# View live LED matrix service logs
# Usage: ./view-logs.sh

echo "Viewing live logs from LED matrix service on 10.1.1.25..."
echo "Press Ctrl+C to exit"
echo ""

sshpass -p node ssh node@10.1.1.25 "journalctl -u led-matrix -f --no-pager"
