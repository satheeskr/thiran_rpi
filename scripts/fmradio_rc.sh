mkfifo ~/mplayer-control
mplayer -ao alsa:device=hw=0.0 -cache 16384 -cache-min 2 -slave -volume 10 -input file=/home/pi/mplayer-control http://prclive1.listenon.in:9948/;stream.mp3 &
