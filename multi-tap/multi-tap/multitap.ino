#include <SimpleTimer.h>

SimpleTimer timer;
// 적외선 센서 핀번호 설정
int motionSensor = 2;
//릴레이에 5V 신호를 보낼 핀설정
int relay = 10;
// 택트 스위치
int button = 8;
int relayTimer; // 릴레이 타이머
int state = LOW; // 택트스위치 상태값
void setup ()
{
  
  pinMode(motionSensor, INPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode (relay, OUTPUT); // relay를 output으로 설정한다.
  relayTimer = timer.setInterval(30000, relayOff); // 30초마다 릴레이 off -> 꺼지는 시간 조절할때 이부분 수정하시면 됩니다
  Serial.begin(9600);
}
void loop ()
{
  timer.run();
  int motion = digitalRead(motionSensor);
  
  if(digitalRead(button) == LOW) { // 스위치 눌렀을때
    if(state == LOW) {
      state = HIGH;
    }
    else if(state ==HIGH) {
      state = LOW;
    }
    //Serial.print(state);
    //Serial.print("\n");
    delay(1000);
  }
  
  if (state == HIGH) { // 스위치를 눌러서 자리비움 상태시
    timer.disable(relayTimer);
    //Serial.print("Switch ON\n");
    digitalWrite(relay, HIGH); // 릴레이 ON
  }
  else { // 자리비움 상태 아닐시
    //Serial.print("Switch OFF\n");
    if(timer.isEnabled(relayTimer) != true) { // 릴레이 타이머 비활성화시 타이머 활성화
        timer.enable(relayTimer);
      }
    if (motion == HIGH) { // 움직임 감지될시
      timer.restartTimer(relayTimer); // 타이머 초기화

      if(digitalRead(relay) == LOW) {
        digitalWrite(relay,HIGH); // 릴레이 ON
        //Serial.print("ON\n");
      }
    }
  }
}

void relayOff() {
  Serial.print("OFF\n");
  digitalWrite(relay, LOW);
  timer.disable(relayTimer);
}
