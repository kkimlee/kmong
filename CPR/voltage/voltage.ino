void setup() {

  Serial.begin(9600);

}



void loop() {

  int sensorValue = analogRead(A0);    // read the input on analog pin 0

  

  float voltage = sensorValue * (5.0 / 1023.0);    // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V)

  

  Serial.println(voltage);    // print out the value you read

}
[출처] 아두이노에 입력되는 전압 측정하기|작성자 해바우
