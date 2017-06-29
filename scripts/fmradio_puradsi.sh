rm ~/mplayer-control
mkfifo ~/mplayer-control
mplayer -ao alsa:device=hw=2.0 -af format=s16le -cache 16384 -cache-min 2 -slave -volume 10 -input file=/home/pi/mplayer-control http://puradsifm.net:9994/;stream.mp3 &

