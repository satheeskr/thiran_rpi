import socket
import struct
import binascii
import os
# Written by Bob Steinbeiser (https://medium.com/@xtalker)

rawSocket = socket.socket(socket.AF_PACKET, socket.SOCK_RAW,
                          socket.htons(0x0003))
MAC = 'b47c9cfef752'
toggle = True

while True:
    packet = rawSocket.recvfrom(2048)

    ethernet_header = packet[0][0:14]
    ethernet_detailed = struct.unpack('!6s6s2s', ethernet_header)

    arp_header = packet[0][14:42]
    arp_detailed = struct.unpack('2s2s1s1s2s6s4s6s4s', arp_header)

    # skip non-ARP packets
    ethertype = ethernet_detailed[2]
    if ethertype != '\x08\x06':
        continue

    source_mac = binascii.hexlify(arp_detailed[5])
    dest_ip = socket.inet_ntoa(arp_detailed[8])

    if source_mac == MAC:
	os.system("sudo /var/www/html/rf/byron")
	os.system("sudo /var/www/html/rf/nexa 5")
	os.system("sudo /home/pi/doorbell.sh")
	os.system("sudo /var/www/html/rf/nexa 6")

#	if toggle == True:
#		print "Chromecast Audio starting"
#		os.system("python /home/pi/pychromecast/pyradio_on.py")
#		print "Chromecast Audio ON"
#	else:
#		print "Chromecast Audio stopping"
#		os.system("python /home/pi/pychromecast/pyradio_off.py")
#		print "Chromecast Audio OFF"

#	toggle = not toggle

