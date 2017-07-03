#!/bin/bash
DATE=$(date +"%Y-%m-%d_%H%M")
IP=$(curl http://ipecho.net/plain)
LD_PRELOAD=/usr/lib/arm-linux-gnueabihf/libv4l/v4l2convert.so fswebcam -r 1280x720 /home/pi/webcam/$DATE.jpeg
#dd if=/dev/video0 of=/home/pi/webcam/$DATE.jpeg bs=11M count=1
mpack -s "Doorbell pressed at $DATE. Watch live streaming at http://$IP:3142/stream" /home/pi/webcam/$DATE.jpeg flower.hearts@gmail.com
