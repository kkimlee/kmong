
#include "DHT.h"       // DHT.h 라이브러리를 포함한다
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <SimpleTimer.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

WiFiServer server(80); // 80번 포트를 이용
WiFiClient client;
HTTPClient http;
SimpleTimer timer;

const char* ssid = "와이파이 이름"; // wifi name
const char* password = "와이파이 비밀번호"; // wifi password
const char* host = "IP 또는 도메인 주소"; // domain or IPAddress

#define DHTPIN 16      // DHT핀을 2번으로 정의한다(DATA핀)
#define DHTTYPE DHT11  // DHT타입을 DHT11로 정의한다
DHT dht(DHTPIN, DHTTYPE);  // DHT설정 - dht (디지털2, dht11)

int sensorIn = A0;n // 토양 습도와 자외선 센서 데이터 받아올 핀

int temp;
int humid;
float uvIntensity;
int soil_humid;

String PostData; // DB에 보낼 데이터를 저장

void setup() {
  pinMode(sensorIn, INPUT);
  pinMode(5, OUTPUT); // UV 센서 전원 핀 D3
  pinMode(4, OUTPUT); // 토양 습도 센서 전원 핀 D4
  
  Serial.begin(115200);    // 115200 속도로 시리얼 통신을 시작한다
  connectWiFi(); // WiFi 연결

  // timer.setInterval(1000, getData); // 0.1 초마다 DB에서 데이터 읽어옴
  timer.setInterval(500, updateDB); // 주기적으로 DB업데이트
}



void loop() {
  timer.run(); // 타이머 시작
}

void request() {
  connectServer(); // 서버에 연결
  dataToDB(); // DB에 보낼 데이터 저장
  Serial.println("request server");
  // This will send the request to the server
  client.println("POST /insertData.php HTTP/1.1"); // 접근할 php파일 경로
  client.println("Host: 도메인 주소"); // 접속할 서버 도메인
  client.println("Connection: close"); // 없을시 timeout오류남
  client.println("Content-Type: application/x-www-form-urlencoded"); // 정보 보내는 방식
  client.print("Content-Length: ");
  client.println(PostData.length()); // 보낼 데이터의 크기
  client.println();
  client.println(PostData); // 데이터 전송

  int timeout = millis() + 5000; // 5초동안 응닶 없을시 연결 해제
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // 서버로 부터 받은 데이터 출력
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
}

// DB로 보낼 데이터

void dataToDB()
{
  measure();
  
  PostData = "";
  PostData += "submit";
  PostData += "&&";
  PostData += "temp=";
  PostData += temp;
  PostData += "&&";
  PostData += "humid=";
  PostData += humid;
  PostData += "&&";
  PostData += "soil_humid=";
  PostData += soil_humid;
  PostData += "&&";
  PostData += "uv=";
  PostData += uvIntensity;
}

// 와이파이 연결
void connectWiFi()
{

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password); // 와이파이 연결
  while (WiFi.status() != WL_CONNECTED) // 연결이 될때까지 실행
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  server.begin();
  Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());
}

// 서버에 연결
void connectServer()
{

  const int httpPort = 80; // 사용할 포트번호
  client.connect(host, httpPort); // 서버에 연결
  if (!client.connected()) {
    return;
  }
}

// DB에서 데이터를 json 형식으로 읽어옴
void getData()
{
  const char* link = "http://도메인 주소/getData.php"; // 데이터를 불러올 경로

  http.begin(link); // 서버에 연결

  int httpCode = http.POST(link); // 정상적으로 연결됬는지 코드 저장 200: OK
  String payload = http.getString();

  if (httpCode == 200) // 정상적으로 연결됬을시
  {
    // 버퍼 할당
    // arduinojson.org/json 버퍼 크기 계산해주는 사이트
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(7) + 138;
    DynamicJsonDocument jsonBuffer(capacity);

    // json 파싱
    auto error = deserializeJson(jsonBuffer, payload);
    if (error) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
      return;
    }

  }
  else
  {
    Serial.println("Error in response");
  }

  http.end();  // 연결 해제
}

// 온도, 습도, 자외선 측정
void measure()
{
  // 자외선량 측정을 위해 D3번 핀 HIGH
  digitalWrite(5, HIGH);
  delay(10000);
  int uvLevel = averageAnalogRead(sensorIn);
  float outputVoltage = 3.3 * uvLevel/1024;

  uvIntensity = mapfloat(outputVoltage, 0.99, 2.9, 0.0, 15.0);
  // 측정 완료 후 다른 센서 이용을 위해 D3번 핀 LOW
  digitalWrite(5, LOW);
  delay(10000);
  // 토양 습도 측정을 위해 D4번 핀 HIGH
  digitalWrite(4, HIGH);
  delay(10000);
  
  soil_humid = analogRead(sensorIn);
  soil_humid = 100 - (soil_humid * 100 / 1024);
  // 측정 완료 후 D4번 핀 LOW
  digitalWrite(4, LOW);
  
  humid = dht.readHumidity();  // 변수 h에 습도 값을 저장
  temp = dht.readTemperature();  // 변수 t에 온도 값을 저장


  Serial.print("Humid: ");  // 문자열 Humidity: 를 출력한다.
  Serial.print(humid);  // 변수 h(습도)를 출력한다.
  Serial.print("%\t");  // %를 출력한다

  Serial.print("Temp: ");  // 이하생략
  Serial.print(temp);
  Serial.println(" C");

  Serial.print(" UV Intensity: ");
  Serial.print(uvIntensity);
  Serial.println(" mW/cm^2");

  Serial.print("Soil Humid: ");
  Serial.print(soil_humid);
  Serial.println("%\t");
}


// 자외선 센서로 측정된 8개의 값의 평균 이용
int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0;
 
  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;
 
  return(runningValue);
 
}
 
 
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//현재 시간을 구글 서버로 부터 가져옴
String getTime() {
  WiFiClient client;
  while (!client.connect("google.com", 80)) {
    Serial.println("connection failed, retrying...");
  }
  client.print("HEAD / HTTP/1.1\r\n\r\n");
  while (!client.available()) {
    yield();
  }
  while (client.available()) {
    if (client.read() == '\n') {
      if (client.read() == 'D') {
        if (client.read() == 'a') {
          if (client.read() == 't') {
            if (client.read() == 'e') {
              if (client.read() == ':') {
                client.read();
                String timeData = client.readStringUntil('\r');
                client.stop();
                return timeData;
              }
            }
          }
        }
      }
    }
  }
}

// 서버로 부터 받아온 시간을 이용하여 타이머 만듬
void setTime() {
  String ct = getTime();

  int mDate = ct.substring(5, 7).toInt();
  String tempMonth = ct.substring(8, 11);
  int mMonth;

  if (tempMonth = "DEC") {
    mMonth = 12;
  } else if (tempMonth = "JAN") {
    mMonth = 1;
  } else if (tempMonth = "FEB") {
    mMonth = 2;
  } else if (tempMonth = "MAR") {
    mMonth = 3;
  } else if (tempMonth = "APR") {
    mMonth = 4;
  } else if (tempMonth = "MAY") {
    mMonth = 5;
  } else if (tempMonth = "JUN") {
    mMonth = 6;
  } else if (tempMonth = "JUL") {
    mMonth = 7;
  } else if (tempMonth = "AUG") {
    mMonth = 8;
  } else if (tempMonth = "SEP") {
    mMonth = 9;
  } else if (tempMonth = "OCT") {
    mMonth = 10;
  } else if (tempMonth = "NOV") {
    mMonth = 11;
  }

  int mYear = ct.substring(12, 16).toInt();
  int mHour = ct.substring(17, 19).toInt() + 9; //구글 타임존과 달라서 9시간을 더해줌
  int mMinute = ct.substring(20, 22).toInt();
  int mSecond = ct.substring(23, 25).toInt();

  setTime(mHour, mMinute, mSecond, mDate, mMonth, mYear); // 재귀로 계속 시간 업데이트
}

// 5분마다 DB로 데이터 전송
void updateDB() {
  if ( minute() % 5 == 0 && second() == 0) {
    request();
  }
}
