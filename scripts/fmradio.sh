mkfifo ~/mplayer-control
mplayer -ao alsa:device=hw=1.0 -cache 16384 -cache-min 2 -slave -volume 4 -input file=/home/pi/mplayer-control http://s1.vanavilfm.com:9000/;stream.mp3 &
