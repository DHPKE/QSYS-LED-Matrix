#!/bin/bash
# Deploy layout persistence changes to Raspberry Pi

RPI_HOST="${1:-led-matrix.local}"
RPI_USER="${2:-user}"
REMOTE_DIR="/home/${RPI_USER}/led-matrix"

echo "Deploying layout persistence updates to ${RPI_USER}@${RPI_HOST}..."

# Copy updated files
scp \
    udp_handler.py \
    main.py \
    ${RPI_USER}@${RPI_HOST}:${REMOTE_DIR}/

if [ $? -eq 0 ]; then
    echo "Files copied successfully. Restarting led-matrix service..."
    ssh ${RPI_USER}@${RPI_HOST} "sudo systemctl restart led-matrix.service"
    
    echo ""
    echo "Deployment complete!"
    echo "The matrix will now preserve your layout choice when switching orientations."
    echo ""
    echo "To view logs: ssh ${RPI_USER}@${RPI_HOST} 'sudo journalctl -u led-matrix -f'"
else
    echo "Error: Failed to copy files"
    exit 1
fi
