#!/bin/bash
DATE=$(date +"%Y-%m-%d_%H%M")

if [ ! -z $5 ]
then
if (($5 == 1)) 
then 
ZONE="Kitchen" 
elif (($5 == 2)) 
then 
ZONE="Hallway 1" 
elif (($5 == 3)) 
then 
ZONE="Lounge" 
elif (($5 == 4)) 
then 
ZONE="Hallway 2" 
elif (($5 == 30)) 
then 
ZONE="Back Door Magnetic Switch" 
elif (($5 == 40)) 
then 
ZONE="Front Door Bell" 
elif (($5 == 50)) 
then 
ZONE="Smoke Alarm" 
elif (($5 == 99)) 
then 
ZONE="Self Check" 
else
ZONE="Unknown"
fi
else
ZONE="Unknown"
fi

IP=$(curl http://ipecho.net/plain)
LD_PRELOAD=/usr/lib/arm-linux-gnueabihf/libv4l/v4l2convert.so fswebcam -r 1280x720 -S 2 /home/pi/webcam/$DATE.jpeg
#dd if=/dev/video0 of=/home/pi/webcam/$DATE.jpeg bs=11M count=1
mpack -s "Intrusion Alert in zone: $ZONE. Watch live streaming at http://$IP:3142/stream" /home/pi/webcam/$DATE.jpeg flower.hearts@gmail.com
#cp /home/pi/webcam/$DATE.jpeg /home/pi/webcam/webcam.jpeg
