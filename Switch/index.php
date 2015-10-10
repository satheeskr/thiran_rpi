<?php 
echo 
"<form action='' method='post'> 
<h1>Welcome to Sathees Pi Page</h1>
<p>Press the buttons below to control the devices</p>
<input type='submit' name='switch1_on' value='Switch 1 ON' style='width:200px;height:100px;color:red;background-color:yellow'/> 
<input type='submit' name='switch1_off' value='Switch 1 OFF' style='width:200px;height:100px;color:red;background-color:green;' /> 
<br></br>
<input type='submit' name='switch2_on' value='Switch 2 ON' style='width:200px;height:100px;color:red;background-color:yellow' /> 
<input type='submit' name='switch2_off' value='Switch 2 OFF' style='width:200px;height:100px;color:red;background-color:green'/> 
<br></br>
<input type='submit' name='switch3_on' value='Switch 3 ON' style='width:200px;height:100px;color:red;background-color:yellow'/> 
<input type='submit' name='switch3_off' value='Switch 3 OFF' style='width:200px;height:100px;color:red;background-color:green'/> 
<br></br>
<input type='submit' name='switch4_on' value='Switch 4 ON' style='width:200px;height:100px;color:red;background-color:yellow' /> 
<input type='submit' name='switch4_off' value='Switch 4 OFF' style='width:200px;height:100px;color:red;background-color:green'/> 
</form>";

if(isset($_POST['switch1_on'])) 
{
	system('sudo rf/rpi_switch 1');
} 
if(isset($_POST['switch1_off'])) 
{
	system('sudo rf/rpi_switch 2');
} 
if(isset($_POST['switch2_on'])) 
{
	system('sudo rf/rpi_switch 3');
} 
if(isset($_POST['switch2_off'])) 
{
	system('sudo rf/rpi_switch 4');
} 
if(isset($_POST['switch3_on'])) 
{
	system('sudo rf/rpi_switch 5');
} 
if(isset($_POST['switch3_off'])) 
{
	system('sudo rf/rpi_switch 6');
} 
if(isset($_POST['switch4_on'])) 
{
	system('sudo rf/rpi_switch 7');
} 
if(isset($_POST['switch4_off'])) 
{
	system('sudo rf/rpi_switch 8');
} 

?>
