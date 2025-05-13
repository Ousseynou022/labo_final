// === Alarm.h ===
#pragma once
#include <Arduino.h>

class Alarm {
public:
  Alarm(int redPin, int bluePin, int pinBuzzer, float* pDistance);

  void update();
  void setDistance(float dist);
  void setTimeout(unsigned long time);
  void setVariationTiming(unsigned long t);
  void setDistanceTrigger(float trigger);

  void turnOn();
  void turnOff();
  void activateAlarm();
  void deactivateAlarm();
  float getDistanceTrigger() const;


private:
  enum State { OFF, WATCHING, ON };
  State state;

  int redPin, bluePin, buzzerPin;
  float* distance;
  float distanceTrigger = 20.0;
  unsigned long timeoutDelay = 3000;
  unsigned long lastStateChange = 0;
  bool alarmEnabled = true;

  unsigned long lastBlinkTime = 0;
  unsigned long blinkInterval = 500;
  bool ledState = false;
  bool alarmActive = false;
  unsigned long currentTime = 0;
  unsigned long lastAlarmTime = 0;

  void toggleLEDs();
  void soundAlarm();
  void stopAlarm();
};