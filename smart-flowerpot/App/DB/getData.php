<?php

include('dbcon.php');

// DB data 불러오는 쿼리
$sqlD = "SELECT * FROM data ORDER BY id DESC LIMIT 1";

// 쿼리문 실행 후 res에 저장
$resD = mysqli_query($conn, $sqlD);

// 불러온 결과를 각 변수에 대입
if($rowD = mysqli_fetch_array($resD))
{
    $temp = $rowD[1];
    $humid = $rowD[2];
	$soil_humid = $rowD[3];
	$uv = $rowD[4];
	$measureTime = $rowD[5];
}
 
// 배열 생성
$result = array ();

array_push($result, array(
    'temp' => $temp,
    'humid' => $humid,
	'soil_humid' => $soil_humid,
	'uv' => $uv,
	'measureTime' => $measureTime
));

// JSON Data로 인코딩
$output = json_encode ( array ("result" => $result) );

// JSON Data 출력
echo urldecode ( $output );
 
// 연결 해제
$conn->close ();
?>