<?php
include('dbcon.php');

    // 현재 시각
    $currentTime=date("Y-m-d H:i:s");

    $sql1 = "SELECT * FROM data ORDER BY id DESC LIMIT 1";
    $resData = mysqli_query($conn, $sql1);
    $rowD = mysqli_fetch_array($resData);

 
    $temp=$_POST['temp'];
    $humid=$_POST['humid'];
	$soil_humid=$_POST['soil_humid'];
	$uv=$_POST['uv'];
       
    try {
        mysqli_query($conn, "INSERT INTO data (id, temp, humid, soil_humid, uv, measureTime) VALUES ($rowD[0]+1, $temp, $humid, $soil_humid, $uv, '$currentTime')");
        echo "1";
    }
    catch(Exception $ex) {    
        echo "1010";
    }

// 연결 해제
$conn->close ();
?>