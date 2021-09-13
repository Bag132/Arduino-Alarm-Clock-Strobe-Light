
#include "RTClib.h"

RTC_PCF8523 rtc;

int RELAY_PIN = 7;
int inputPin = 8; //infrared proximity switch connected to digital pin 2
int buttonPin = 2;

bool led_on = false;
long target_time;

DateTime alarmDate (2021, 9, 11, 7, 30, 0);
DateTime alarmEnd;


bool alarmDeactivated = false;

bool firstAlarm = true;
bool firstDeactivate = true;
bool firstActivate = true;

DateTime alarmStart;

bool blinking = false;

DateTime nextAlarmDatetime;

enum State {
  Activated,
  Deactivated,
  Alarming
};

State currentState;

void showDate(const char* txt, const DateTime& dt) {
  Serial.print(txt);
  Serial.print(' ');
  Serial.print(dt.year(), DEC);
  Serial.print('/');
  Serial.print(dt.month(), DEC);
  Serial.print('/');
  Serial.print(dt.day(), DEC);
  Serial.print(' ');
  Serial.print(dt.hour(), DEC);
  Serial.print(':');
  Serial.print(dt.minute(), DEC);
  Serial.print(':');
  Serial.print(dt.second(), DEC);

//  Serial.print(" = ");
//  Serial.print(dt.unixtime());
//  Serial.print("s / ");
//  Serial.print(dt.unixtime() / 86400L);
//  Serial.print("d since 1970");

  Serial.println();
}

void setLight(bool on) {
  if (on) {
    digitalWrite(RELAY_PIN, HIGH);
//    Serial.println("Switching on");
  } else {
    digitalWrite(RELAY_PIN, LOW);
//    Serial.println("Switching off");
  }
  Serial.flush();
}

bool buttonPressed() {
  return !digitalRead(buttonPin);
}

DateTime getNextAlarmTime() {
  DateTime currentDt = rtc.now();
  int currentHour = currentDt.hour();
  int currentMinute = currentDt.minute();
  int currentSecond = currentDt.second();

  int alarmHour = alarmDate.hour();
  int alarmMinute = alarmDate.minute();

  if (currentHour > alarmHour || (currentHour == alarmHour && currentMinute > alarmMinute)) {
    DateTime tomorrow (currentDt + TimeSpan(1, 0, 0, 0));
    DateTime nextAlarmDate = DateTime(tomorrow.year(), tomorrow.month(), tomorrow.day(), alarmHour, alarmMinute, 0);
    return nextAlarmDate;
  } else if (currentHour <= alarmHour) {
    DateTime nextAlarmDate;
    if (currentMinute >= alarmMinute && currentHour == alarmHour) {
      nextAlarmDate = currentDt + TimeSpan(1, 0, 0, 0);
    } else {
      nextAlarmDate = DateTime(currentDt.year(), currentDt.month(), currentDt.day(), alarmHour, alarmMinute, 0);
    }
    return nextAlarmDate;
  }
}

bool check(const DateTime& d1, const DateTime& d2) {
  return d1.day() == d2.day() && d1.hour() == d2.hour() && d1.minute() == d2.minute();
}

void setup() {
  Serial.begin(57600);
  
#ifndef ESP8266
  while (!Serial);
#endif

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (!rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  rtc.start();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  nextAlarmDatetime = getNextAlarmTime();

  Serial.println("Next alarm time:");
  showDate("Alarm Date:", nextAlarmDatetime);

  currentState = Activated;

  alarmEnd = (nextAlarmDatetime + TimeSpan(360, 0, 0, 0));

  showDate("Current time: ", rtc.now());
}


void loop() {
  DateTime now(rtc.now());

  switch (currentState) {
    case Activated:
      blinking = false;
      firstDeactivate = true;
      firstAlarm = true;

      if (firstActivate) {
        Serial.println("State: Activated");
        setLight(false);
        firstActivate = false;
      }

      if (check(nextAlarmDatetime, now)) {
        Serial.println("Conditions met");
        currentState = Alarming;
      }

      if (buttonPressed()) {
        currentState = Deactivated;
      }
      break;
    case Deactivated:
      blinking = false;
      firstActivate = true;
      firstAlarm = true;

      if (firstDeactivate) {
        Serial.println("State: Deactivated");
        setLight(true);
        firstDeactivate = false;
      }

      if (!buttonPressed()) {
        nextAlarmDatetime = getNextAlarmTime();
        showDate("Current time: ", now);
        showDate("Next alarm time: ", nextAlarmDatetime);
        currentState = Activated;
      }
      break;
    case Alarming:
      firstActivate = true;
      firstDeactivate = true;

      if (firstAlarm) {
        Serial.println("State: Alarming");
        alarmEnd = (now + TimeSpan(0, 0, 15, 0));
        showDate("Alarm end: ", alarmEnd);
        blinking = true;
        firstAlarm = false;
      }

      if (alarmEnd < now) {
        Serial.println("Alarm ended");
        currentState = Deactivated;
      }

      if (buttonPressed()) {
        Serial.println("Button pressed");
        currentState = Deactivated;
      }
      break;
  }

  if (blinking) {
    if (millis() >= target_time) {
      led_on = !led_on;
      setLight(led_on);
      target_time = millis() + 200;
    }
  }
}
