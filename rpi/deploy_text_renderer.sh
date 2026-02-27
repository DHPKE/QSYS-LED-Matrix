#!/bin/bash
scp text_renderer.py pi@10.1.1.10:/tmp/
ssh pi@10.1.1.10 << 'ENDSSH'
sudo mv /tmp/text_renderer.py /opt/led-matrix/
sudo systemctl restart led-matrix
sleep 3
sudo systemctl status led-matrix --no-pager
ENDSSH
