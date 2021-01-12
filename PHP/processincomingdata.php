<?php

//establish a link to the database
$link = mysqli_connect("localhost", "phpmyadmin", "arduino", "co2data");

//grab the values sent in the GET request
$co2 = $_GET['c'];
$tabel = $_GET['t'];

$query = "INSERT INTO $tabel (timestamp, co2_concentration) VALUES (NOW(), '$co2')";

//execute the query/commit to the db
$result = mysqli_query($link, $query);

//close the connection
mysqli_close($link);

?>
