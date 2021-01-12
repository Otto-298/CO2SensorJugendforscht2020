<?php

$link = mysqli_connect("localhost", "phpmyadmin", "arduino", "co2data");
$query = "SELECT uid, TIME(timestamp) as dbTime, co2_concentration FROM sensor4 ORDER BY uid DESC LIMIT 40";

//query above is executed
$result = mysqli_query($link, $query);

//declare arrays
$timestamps = array();
$co2_concentrations = array();
$pixelHeights = array();

//35 cols @ 10 min intervals = 6 hrs, indexed from 0
$columnRange = 35;

//dump the db results into separate arrays so we can use them later 
while ($db = mysqli_fetch_array($result))
{
	//chop off the seconds portion of the time value
  $pieces = explode(':', $db['dbTime']);
	$formattedTime = $pieces[0] . ':' . $pieces[1];
	
  $timestampsTime[] = $formattedTime;
	$co2_concentrations[] = $db['co2_concentration'];
	$uid[] = $db['uid'];
}

//find the min and max values in the range 
$minco2 = 5000;
$maxco2 = 0;
for ($i = 0; $i <= $columnRange; $i++)
{
	if ($minco2 > $co2_concentrations[$i])
	{
		$minco2 = $co2_concentrations[$i];
	} 
	
	if ($maxco2 < $co2_concentrations[$i])
	{
		$maxco2 = $co2_concentrations[$i];
	}
}

//define the scale of the graph to display
$rangeHigh = 2000;
if ($maxco2 > $rangeHigh & $rangeHigh >= 2000)
{
        $rangeHigh = $maxco2 + 1;
}
$rangeLow = 380;

//calculate the height for each data point
//The bars are then displayed as 10px wide and x% high using a single pixel as a base
//pixel graphics from 1x1px.me
for ($i = 0; $i <= $columnRange; $i++)
{
	$currentco2 = ($co2_concentrations[$i]) - ($rangeLow);
	$pixelHeights[$i] = round($currentco2 / (($rangeHigh) - ($rangeLow)) * 100);
}

//HTML Part (PHP is interspersed as needed) 
?>

 <!DOCTYPE html>

<html>
<head>
<style>
table, th, td, h1 {
  font-family: calibri;
}
</style>
<title>CO2-Sensor 4</title>
</head>

<body>
<h1><u>CO2-Sensor 4</u></h1>

<!--       Summary table            -->
<table>
  <tr>
    <td width='150'><b>CO2-Concentration:</b></td><td><?php echo round($co2_concentrations[0], 1); ?> ppm</td>
  </tr>
    <td valign='bottom' height='5' colspan='2'>Last updated at <?php echo $timestampsTime[0]; ?></td>
  </tr>
</table>

<br>

<!--       CO2 Trend            -->
<table style='border: 1px solid black;'>
  <tr>
    <td colspan='20'><b>CO2 Trend (past 6 hours)</b></td>
  </tr>
  <tr>
    <td colspan='20'>&nbsp;</td>
  </tr>
  <tr>
  <td width='150'>
    <b>Current:</b> <?php echo round($co2_concentrations[0], 1); ?> ppm <br>
	  <b>High:</b> <?php echo round($maxco2, 1); ?> ppm <br>
	  <b>Low:</b> <?php echo round($minco2, 1); ?> ppm
  </td>
  <!-- empty column for spacing -->
  <td width='10'>
    &nbsp;
  </td>
	
  <?php
    
    //Here we loop over the array of co2_concentration values
    //The corresponding values for pixelHeights is at the same index in its array so we stretch our single pixel 
    //graphic to be 10px wide and $pixelHeights[x] high
    
    for ($i = $columnRange; $i >= 0; $i--)
    {
      if ($co2_concentrations[$i] >= 2000)
      {
        echo "<td valign='bottom'><img src='red.png' height='" . $pixelHeights[$i] . "' width='10' alt='pixel'></td> \n";
      }
        elseif ($co2_concentrations[$i] >= 1500)
      {
        echo "<td valign='bottom'><img src='orange.png' height='" . $pixelHeights[$i] . "' width='10' alt='pixel'></td> \n";
      }
        else
      {
        echo "<td valign='bottom'><img src='green.png' height='" . $pixelHeights[$i] . "' width='10' alt='pixel'></td> \n";
      }
    }
  ?>
		
	</tr>
	<tr>
	 <td colspan='2'>&nbsp;</td>
   <td colspan='18'>&#129121;&nbsp;<?php echo $timestampsTime[$columnRange]; ?></td>     <!-- timestamp for the oldest data -->
	 <td colspan='18' align='right'><?php echo $timestampsTime[0]; ?>&nbsp;&#129121;</td>  <!-- timestamp for the latest data -->
	</tr>
</table> 

</body>
</html>