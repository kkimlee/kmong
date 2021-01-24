#include <SoftwareSerial.h>
#include <SimpleTimer.h>

// 핀 연결 설정
#define SENSOR 14 // 센서핀 A0핀으로 설정

SimpleTimer timer;
SoftwareSerial HM10(2, 3); // RX, TX

int buzzer = 7; // 부저핀 7번으로 설정


int tones[] = {261, 392, 523}; // 측정시작 및 압박성공시, 단계 통과시, 기준 미달시.

int count; // 압박 횟수
int level; // 압박 단계 총 5단계
int checkPressed; // 눌린상태
int state; // 측정 상태 01:측정대기, 02:측정중, 03:기준미달 or 중도포기, 04:성공
int ble_timer; // BLE통신 타이머
int start;
int pairing_check;
int pairingState;

float depth;

void setup() {
  //기본 통신속도는 9600입니다.
  Serial.begin(9600);
  HM10.begin(9600);

  initialize(); // 압박횟수, 단계, 눌린상태 초기화

  pinMode(buzzer, OUTPUT);
  ble_timer = timer.setInterval(6000, startBLE); // 6초마다 통신
  start = timer.setInterval(1000, startCPR); // 압박판을 눌러서 state 2로 변경.
  pairing_check = timer.setInterval(100,pairing);
}

void loop() {
  timer.run(); // timer시작

  if (HM10.available()) {
    pairingState = 1;
    //Serial.write(HM10.read());
  }
  if(HM10.available()%2==0) {
    pairingState = 0;
    timer.enable(pairing_check);
  }

  if (state != 2) {
    if (!timer.isEnabled(start)) {
      timer.enable(start);
    }
  }
  else { // 측정중
    countCPR();
  }
  Serial.print(HM10.available());
  Serial.print(state);
  Serial.print("\n");
  delay(150);
}
// 30초를 5구간으로 나누어서 단계별로 진행
void levelUp() {
  if (state == 2) { // 측정중
    if (level > 4 && count > 10) { // 마지막 레벨에서 10회이상 압박시 성공
      state = 4;
      initialize();
      sendData();
      timer.disable(ble_timer);
    }
    else {
      if (count >= 10) { // 10회이상 압박했을시 다음단계로
        tone(buzzer, tones[2]);
        timer.setTimeout(50, turnOffBuzzer);
        level = level + 1;
        count = 0;
        sendData();
      }
      else { // 기준미달 or 중도포기
        timer.disable(ble_timer);
        tone(buzzer, tones[1]);
        timer.setTimeout(1000, turnOffBuzzer);
        state = 3;
        sendData();
        initialize();
        //sendData();
      }
    }
  }
}

// 블루투스 모듈을 이용해 전송할 정보
void sendData()
{
  char Ddata[8];
  String Cstr, Sstr, Zstr, Dstr;
  String Zero = "0";

  Sstr = Zero + String(state);
  if (count >= 10) {
    Cstr = String(count);
  }
  else {
    Cstr = Zero + String(count);
  }

  Dstr = "S" + Sstr + "C" + Cstr;

  Dstr.toCharArray(Ddata, 8);

  HM10.write(Ddata);
  HM10.write("\n");
}

void measureDepth()
{
  float distance_start = 14, distance = 0;
  depth = 0;
  unsigned int vol; // 센서의 전압값 저장변수

  vol = analogRead(SENSOR); // 센서의 전압값을 변수에 저장

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
    if (checkPressed == 1) {
      count = count + 1;
      tone(buzzer, tones[0]);
      timer.setTimeout(50, turnOffBuzzer);
    }
  }
  else {
    checkPressed = 0;
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
  if (depth >= 6)
  {
    checkPressed = checkPressed + 1;
  }
  else
  {
    checkPressed = 0;
  }
  // 2초동안 누르고 있을 경우 state값 02로 변경.
  if (checkPressed >= 2)
  {
    tone(buzzer, tones[1]);
    timer.setTimeout(50, turnOffBuzzer);
    state = 2;
    sendData();
    if (!timer.isEnabled(ble_timer)) {
      timer.enable(ble_timer);
    }
    timer.restartTimer(ble_timer);

    timer.disable(start);
  }
}

// 통신
void startBLE()
{
  if (state == 2) {
    sendData();
    levelUp();
  }
}

// 초기화
void initialize()
{
  level = 1;
  count = 0;
  checkPressed = 0;
  state = 1;
}

void pairing()
{
  if(pairingState == 1) {
    tone(buzzer, tones[0]);
    delay(50);
    tone(buzzer, tones[1]);
    delay(50);
    tone(buzzer, tones[2]);
    delay(50);
    turnOffBuzzer();
    

    timer.disable(pairing_check);
  }
}
