// === Alarm.cpp ===
#include "Alarm.h"
#include <Arduino.h>

Alarm::Alarm(int redPin, int bluePin, int pinBuzzer, float* pDistance)
  : redPin(redPin), bluePin(bluePin), buzzerPin(pinBuzzer), distance(pDistance) {
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  state = OFF;
  lastStateChange = millis();
}

void Alarm::setDistance(float dist) {
  *distance = dist;
}

void Alarm::setTimeout(unsigned long time) {
  timeoutDelay = time;
}

void Alarm::setVariationTiming(unsigned long t) {
  blinkInterval = t;
}

void Alarm::setDistanceTrigger(float trigger) {
  distanceTrigger = trigger;
}

void Alarm::turnOn() {
  state = WATCHING;
  lastStateChange = millis();
}

void Alarm::turnOff() {
  state = OFF;
  stopAlarm();
  digitalWrite(redPin, LOW);
  digitalWrite(bluePin, LOW);
}

void Alarm::activateAlarm() {
  alarmEnabled = true;
  if (state == OFF) {
    state = WATCHING;
    lastStateChange = millis();
  }
}

void Alarm::deactivateAlarm() {
  alarmEnabled = false;
  stopAlarm();
  digitalWrite(redPin, LOW);
  digitalWrite(bluePin, LOW);
}

void Alarm::update() {
  currentTime = millis();
  float dist = *distance;

  switch (state) {
    case OFF:
      stopAlarm();
      digitalWrite(redPin, LOW);
      digitalWrite(bluePin, LOW);
      break;

    case WATCHING:
      digitalWrite(redPin, LOW);
      digitalWrite(bluePin, LOW);
      if (dist < distanceTrigger) {
        state = ON;
        lastStateChange = currentTime;
      }
      break;

    case ON:
      if (alarmEnabled) {
        soundAlarm();

        if (!alarmActive) {
          lastAlarmTime = currentTime;
          alarmActive = true;
        }

        if (currentTime - lastBlinkTime >= blinkInterval) {
          lastBlinkTime = currentTime;
          toggleLEDs();
        }
      }

      if (dist >= distanceTrigger && currentTime - lastAlarmTime >= timeoutDelay) {
        stopAlarm();
        digitalWrite(redPin, LOW);
        digitalWrite(bluePin, LOW);
        alarmActive = false;
        state = WATCHING;
      }
      break;
  }
}

void Alarm::toggleLEDs() {
  ledState = !ledState;
  digitalWrite(redPin, ledState);
  digitalWrite(bluePin, !ledState);
}

void Alarm::soundAlarm() {
  digitalWrite(buzzerPin, HIGH);
}

void Alarm::stopAlarm() {
  digitalWrite(buzzerPin, LOW);
  digitalWrite(redPin, LOW);
  digitalWrite(bluePin, LOW);
}

  float Alarm::getDistanceTrigger() const {
  return distanceTrigger;
}

