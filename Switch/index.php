<?php 
echo
"<form action='' method='post'> 
<h1>Welcome to Sathees Pi Page</h1>
<p>Press the buttons below to control the devices</p>
<input type='submit' name='switch1_on' value='HEATER ON	' style='width:300px;height:100px;color:red;background-color:yellow;font-family:sans-serif;font-size:25px'/> 
<input type='submit' name='switch1_off' value='HEATER OFF' style='width:300px;height:100px;color:red;background-color:green;font-family:sans-serif;font-size:25px;' /> 
<br></br>
<input type='submit' name='switch2_on' value='MIRROR LIGHT ON' style='width:300px;height:100px;color:red;background-color:yellow;font-family:sans-serif;font-size:25px' /> 
<input type='submit' name='switch2_off' value='MIRROR LIGHT OFF' style='width:300px;height:100px;color:red;background-color:green;font-family:sans-serif;font-size:25px'/> 
<br></br>
<input type='submit' name='switch3_on' value='SERIAL LIGHTS ON' style='width:300px;height:100px;color:red;background-color:yellow;font-family:sans-serif;font-size:25px'/> 
<input type='submit' name='switch3_off' value='SERIAL LIGHTS OFF' style='width:300px;height:100px;color:red;background-color:green;font-family:sans-serif;font-size:25px'/> 
<br></br>
<input type='submit' name='switch4_on' value='DING-DONG' style='width:300px;height:100px;color:red;background-color:yellow;font-family:sans-serif;font-size:25px' /> 
<input type='submit' name='switch4_off' value='ALL OFF' style='width:300px;height:100px;color:red;background-color:green;font-family:sans-serif;font-size:25px'/> 
<br></br>
<input type='text' name='minutes' value = '0' style='width:300px;height:100px;color:red;background-color:white;font-family:sans-serif;font-size:25px' />
<input type='submit' name='min_timer' value='MINUTE TIMER' style='width:300px;height:100px;color:red;background-color:white;font-family:sans-serif;font-size:25px' />
</form>";

if(isset($_POST['switch1_on'])) 
{
	echo($_POST['switch1_on']);
	system('sudo rf/nexa 1');
} 
if(isset($_POST['switch1_off'])) 
{
	echo($_POST['switch1_off']);
	system('sudo rf/nexa 2');
} 
if(isset($_POST['switch2_on'])) 
{
	echo($_POST['switch2_on']);
	system('sudo rf/nexa 3');
} 
if(isset($_POST['switch2_off'])) 
{
	echo($_POST['switch2_off']);
	system('sudo rf/nexa 4');
} 
if(isset($_POST['switch3_on'])) 
{
	echo($_POST['switch3_on']);
	system('sudo rf/nexa 5');
} 
if(isset($_POST['switch3_off'])) 
{
	echo($_POST['switch3_off']);
	system('sudo rf/nexa 6');
} 
if(isset($_POST['switch4_on'])) 
{
	echo($_POST['switch4_on']);
	system('sudo rf/bell');
} 
if(isset($_POST['switch4_off'])) 
{
	echo($_POST['switch4_off']);
	system('sudo rf/nexa 0');
} 
if(isset($_POST['min_timer'])) 
{
	$val = $_POST['minutes'];
	echo "$val\t";
	echo($_POST['min_timer']);
	system("sudo rf/bell $val");
} 
?>
