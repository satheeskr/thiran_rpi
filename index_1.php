<?php 
echo
"<form action='' method='post'> 
<h1>Welcome to Sathees Pi Page</h1>
<p>Press the buttons below to control the devices</p>
<br></br>
<input type='submit' name='arm' value='BURGLAR ALARM ARM' style='width:350px;height:100px;color:white;background-color:red;font-family:sans-serif;font-size:25px'/> 
<input type='submit' name='disarm' value='BURGLAR ALARM DISARM' style='width:350px;height:100px;color:white;background-color:green;font-family:sans-serif;font-size:25px;' /> 
<br></br>
<input type='submit' name='capture' value='IMAGE CAPTURE' style='width:350px;height:100px;color:white;background-color:green;font-family:sans-serif;font-size:25px;' /> 
<br></br>
</form>"; 
if(isset($_POST['arm'])) 
{
	echo($_POST['arm']);
	system('sudo /home/pi/zatty_rpi/Switch/pir &');
} 
if(isset($_POST['disarm'])) 
{
	echo($_POST['disarm']);
	system('sudo  killall pir');
} 
if(isset($_POST['capture'])) 
{
	echo($_POST['capture']);
	system('sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/nexa 3');
	system('sudo -u pi /home/pi/webcam.sh');
	system('sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/nexa 4');
}
?>
