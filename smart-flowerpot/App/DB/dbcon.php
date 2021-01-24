<?php

$servername = "localhost";
$username = "kkimlee";
$dbname = "kkimlee";
$password = "kmt1107!";

header("Content-Type: text/html; charset=UTF-8");
session_start();

$conn = new mysqli ($servername,$username,$password,$dbname);

if( mysqli_connect_error() )
{
    exit('Connect Error ('.mysqli_connect_errno().')'.mysqli_connect_error());
}

?>