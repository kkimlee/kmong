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

void setup() {
  //기본 통신속도는 9600입니다.
  Serial.begin(9600);
  HM10.begin(9600);

  initialize(); // 압박횟수, 단계, 눌린상태 초기화

  pinMode(buzzer, OUTPUT);
  ble_timer = timer.setInterval(6000, startBLE); // 6초마다 통신
}

void loop() {
  timer.run(); // timer시작
  if (state != 2) {
    startCPR(); // CPR측정 시작
  }
  else { // 측정중
    countCPR();
  }

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
        timer.setTimeout(50, turnOffBuzzer);
        state = 3;
        sendData();
        initialize();
        sendData();
      }
    }
  }
}

// 블루투스 모듈을 이용해 전송할 정보
void sendData()
{
  char Cdata[4], Sdata[4];
  String Cstr, Sstr;

  Sstr = String(state);
  Sstr.toCharArray(Sdata, 4);
  Cstr = String(count);
  Cstr.toCharArray(Cdata, 4);

  HM10.write("S");
  HM10.write(Sdata);
  HM10.write("C");
  HM10.write(Cdata);
  HM10.write("\n");
}

// 압박 횟수
void countCPR()
{
  float distance_start = 14, distance = 0;
  float depth = 0;
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
  char start = ' ';

  start = (char)HM10.read();

  if (start == 's') {
    tone(buzzer, tones[1]);
    timer.setTimeout(50, turnOffBuzzer);
    state = 2;
    sendData();
    if(!timer.isEnabled(ble_timer)) {
      timer.enable(ble_timer);
    }
    timer.restartTimer(ble_timer);
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
