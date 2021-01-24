#include <SoftwareSerial.h>
#include <SimpleTimer.h>

// 핀 연결 설정
#define SENSOR 14 // 센서핀 A0핀으로 설정

SimpleTimer timer;
SoftwareSerial HM10(2, 3); // RX, TX

int buzzer = 7; // 부저핀 7번으로 설정


int tones[] = {261, 392, 523}; // 측정시작 및 압박성공시, 단계 통과시, 기준 미달시.

int count; // 압박 횟수
int countAccuracy; // 정확한 압박 횟수
float depth; // 압박 깊이
int level; // 압박 단계 총 5단계
int checkPressedS; // 기준에맞게 눌린상태
int checkPressed; // 눌린상태
int overlap; // count시 중복확인
int avg[10];
// 측정 상태
// 01:측정대기, 02:측정중
// 03 : 성공
// 04 : 횟수 미달
// 05 : 깊이 미달
// 06 : 깊이 및 횟수 모두 미달
int state;

int ble_timer; // BLE통신 타이머
int start; // 측정 시작에 사용할 타이머
int connecting; // 연결
int pairingState; // 페어링 여부  0: 연결끊김, 1: 연결, 2: 초기상태
int disconnecting; // 연결해제
int pairing; // 페어링시 바뀌는 숫자
String depthstr;
char test[12];


void setup() {
  //기본 통신속도는 9600입니다.
  Serial.begin(9600);
  HM10.begin(9600);
  pairingState = 2;
  initialize(); // 압박횟수, 단계, 눌린상태 초기화

  pinMode(buzzer, OUTPUT);
  ble_timer = timer.setInterval(6000, startBLE); // 6초마다 통신
  start = timer.setInterval(1000, startCPR); // 압박판을 눌러서 state 2로 변경.
  connecting = timer.setInterval(100, connection); // 연결확인
  disconnecting = timer.setInterval(100, disconnection); // 연결해제 확인
}

void loop() {
  timer.run(); // timer시작
  pairing = HM10.available();

  // 연결
  if (pairing % 2 != 0) {
    pairingState = 1;
    timer.enable(disconnecting);
    //Serial.write(HM10.read());
  }

  // 연결해제
  if (pairing % 2 == 0 && pairing != 0) {
    pairingState = 0;
    timer.enable(connecting);
  }

  // 측정시작
  if (state != 2) {
    if (!timer.isEnabled(start)) {
      timer.enable(start);
    }
  }
  else { // 측정중
    countCPR();
  }
  
  //Serial.print(pairing);
  /*depthstr = String(depth);
  depthstr.toCharArray(test, 12);
  HM10.write(test);*/
  delay(100);
}
// 30초를 5구간으로 나누어서 단계별로 진행
void levelUp()
{
  if (state == 2)
  { // 측정중
    /*if (level > 4 && count >= 10)
    { // 마지막 레벨에서 10회이상 압박시 성공
      state = 3;
      initialize();
      sendData();
      timer.disable(ble_timer);
    }*/ // 10~12회
    if (count >= 10 && count <= 12)
    {
      // 정확한 압박 횟수 : 10회 이상
      if (countAccuracy >= 10 && countAccuracy <= 12)
      {
        if (level > 4)
        {
          state = 3;
          sendData();
          initialize();
          timer.disable(ble_timer);
        }
        else
        {
          tone(buzzer, tones[2]);
          timer.setTimeout(50, turnOffBuzzer);
          level = level + 1;
          count = 0;
          countAccuracy = 0;
          overlap = 0;
          sendData();
        }
      }
      // 깊이 미달
      else
      {
        timer.disable(ble_timer);
        tone(buzzer, tones[1]);
        timer.setTimeout(1000, turnOffBuzzer);
        state = 5;
        sendData();
        initialize();
      }
    }
    // 전체 시도 횟수가 10회 미만, 12회 초과 : 횟수 미달로 간주
    else
    {
      timer.disable(ble_timer);
      tone(buzzer, tones[1]);
      timer.setTimeout(1000, turnOffBuzzer);
      state = 4;
      sendData();
      initialize();
    }
  }
}

// 블루투스 모듈을 이용해 전송할 정보
void sendData()
{
  char Ddata[12];
  String Cstr, Sstr, Pstr, Dstr;
  String Zero = "0";

  Sstr = Zero + String(state);
  if (count >= 10)
  {
    Cstr = String(count);
  }
  else
  {
    Cstr = Zero + String(count);
  }
  if (countAccuracy >= 10)
  {
    Pstr = String(countAccuracy);
  }
  else
  {
    Pstr = Zero + String(countAccuracy);
  }
  
  Dstr = "S" + Sstr + "C" + Cstr + "P" + Pstr;

  Dstr.toCharArray(Ddata, 12);

  HM10.write(Ddata);
  HM10.write("\n");
}
// 깊이 측정
void measureDepth()
{
  float distance_start = 14, distance = 0;
  unsigned int vol; // 센서의 전압값 저장변수
  int sum = 0;
  depth = 0;
  
  //float volts = analogRead(SENSOR) * 0.0048828125; 
  //int distance = 13 * pow(volts, -1);
  
  // 센서에서 읽는값 25개의 평균
  for(int i = 0; i<25; i++) {
    sum += analogRead(SENSOR);
  }
  
  vol = sum/25; // 센서의 전압값을 변수에 저장

  // Valid Range of IR Sensor : 4~36cm
  if (vol > 600)                             // 하한값에서 3 cm 출력
  {
    distance = 3;
  }
  else if (vol < 80 )                             // 상한값에서 37 cm 출력
  {
    distance = 37;
  }
  else                                       // 4cm ~ 36cm 의 측정결과를 출력
  {
    distance = (1 / (0.000413153 * vol - 0.0055266887));  // 입력받은 전압값을 거리로 계산
  }

  
  // 처음 거리와 현재 거리의 차를 이용해 압박 깊이 측정
  depth = distance_start - distance;
}

// 압박 횟수
void countCPR()
{
  measureDepth();

  // 압박깊이가 6cm이상일 경우 카운트
   if (depth >= 6) {
    checkPressed = checkPressed + 1;
    checkPressedS = checkPressedS + 1;
    if (checkPressedS == 1) {
      if(overlap != 1) {
        count++;
        overlap = 1;
      }
      countAccuracy++;
      tone(buzzer, tones[0]);
      timer.setTimeout(50, turnOffBuzzer);
      /*Serial.print("깊이: ");
      Serial.print(depth);
      Serial.print("\n");
      Serial.print("압박: ");
      Serial.print(count);
      Serial.print("\n");
      Serial.print("유효압박: ");
      Serial.print(countAccuracy);
      Serial.print("\n");*/
      
    }
  } // 압박을 했으나 기준에 맞지 않을경우
  else if (depth >= 3) {
    checkPressed = checkPressed + 1;
    if (checkPressed == 1 && overlap != 1) {
      count++;
      overlap = 1;
      /*Serial.print("깊이: ");
      Serial.print(depth);
      Serial.print("\n");
      Serial.print("압박만: ");
      Serial.print(count);
      Serial.print("\n");*/
      
    }
  }
  else {
    checkPressed = 0;
    checkPressedS = 0;
    overlap = 0;
    /*Serial.print("깊이: ");
    Serial.print(depth);
    Serial.print("\n");*/
  }
}

// 부저종료
void turnOffBuzzer()
{
  noTone(buzzer);
}

// 측정 시작 -> 앱에서 시작 신호를 받아 시작함
void startCPR()
{
  measureDepth();

  // 압박 깊이가 3cm 보다 클경우 값 증가
  if (depth >= 5 && pairingState == 1)
  {
    checkPressed = checkPressed + 1;
  }
  else
  {
    checkPressed = 0;
  }
  // 1초동안 누르고 있을 경우 state값 02로 변경.
  if (checkPressed >= 2)
  {
    tone(buzzer, tones[1]);
    timer.setTimeout(50, turnOffBuzzer);
    state = 2;
    sendData();
    overlap = 1;
    countAccuracy = countAccuracy - 1;
    if (!timer.isEnabled(ble_timer))
    {
      timer.enable(ble_timer);
    }
    timer.restartTimer(ble_timer);

    timer.disable(start);
  }
}

// 통신
void startBLE()
{
  if (state == 2)
  {
    sendData();
    levelUp();
  }
}

// 초기화
void initialize()
{
  level = 1;
  count = 0;
  countAccuracy = 0;
  checkPressedS = 0;
  checkPressed = 0;
  state = 1;
}
// 연결시 소리
void connection()
{
  if (pairingState == 1)
  {
    tone(buzzer, tones[0]);
    delay(100);
    tone(buzzer, tones[1]);
    delay(100);
    tone(buzzer, tones[2]);
    delay(100);
    turnOffBuzzer();


    timer.disable(connecting);
  }
}
// 연결해제시 소리
void disconnection()
{
  if (pairingState == 0)
  {
    tone(buzzer, tones[2]);
    delay(100);
    tone(buzzer, tones[1]);
    delay(100);
    tone(buzzer, tones[0]);
    delay(100);
    
    turnOffBuzzer();

    timer.disable(disconnecting);
  }
}
