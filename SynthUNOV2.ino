//Stepper Arduino
#include <TimerOne.h>
#include "pitches.h"
#define CLR(x,y) (x&=(~_BV(y)))
#define SET(x,y) (x|=(_BV(y)))
#define timerResolution 80000 //vibrato

unsigned long prevMicros[] = {0, 0, 0, 0, 0};
unsigned int period[] = {0, 0, 0, 0, 0};
byte originalPitch[] = {0, 0, 0, 0, 0};
byte bendSens[] = {2, 2, 2, 2, 2};
float bendFactor[] = {1, 1, 1, 1, 1};
float originalBend[] = {1, 1, 1, 1, 1};
float maxVibrato[] = {0, 0, 0, 0, 0};
bool enableVibrato[] = {0, 0, 0, 0, 0};
bool vibratoState[] = {0, 0, 0, 0, 0};
byte controlVal1 = -1;
byte controlVal2 = -1;

void setup() {
  Timer1.initialize(timerResolution);
  Timer1.attachInterrupt(vibrato);
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  Serial.begin(115200);
}

void loop() {
  readMIDI();
  stepperMotors(1);
  stepperMotors(2);
  stepperMotors(3);
  stepperMotors(4);
}

void vibrato() {
  for (int i = 1; i < 5; i++) {
    if (enableVibrato[i]) {
      if (vibratoState[i]) {
        bendFactor[i] = originalBend[i] + maxVibrato[i];
      }
      else {
        bendFactor[i] = originalBend[i] - maxVibrato[i];
      }
      calculatePeriod(i);
      vibratoState[i] = !vibratoState[i];
    }
  }
}

void stepperMotors(byte channel) {
  if (period[channel] > 0 && micros() - prevMicros[channel] >= period[channel]) {
    if (channel == 1) {
      SET(PORTD, 3);
      CLR(PORTD, 3);
    }
    if (channel == 2) {
      SET(PORTD, 5);
      CLR(PORTD, 5);
    }
    if (channel == 3) {
      SET(PORTD, 7);
      CLR(PORTD, 7);
    }
    if (channel == 4) {
      SET(PORTB, 1);
      CLR(PORTB, 1);
    }
    prevMicros[channel] = micros();
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (velocity == 0) {
    handleNoteOff(channel, note, velocity);
  }
  else {
    originalPitch[channel] = note;
    prevMicros[channel] = micros();
    calculatePeriod(channel);
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (note == originalPitch[channel]) {
    period[channel] = 0;
    originalPitch[channel] = 0;
  }
}

void handlePitchBend(byte channel, int bend) {
  int mapped = map(bend, -8192, 8191, 0, 63);
  if (bendSens[channel] == 2) {
    bendFactor[channel] = bendVal2[mapped];
  }
  else if (bendSens[channel] == 12) {
    bendFactor[channel] = bendVal12[mapped];
  }
  originalBend[channel] = bendFactor[channel];
  calculatePeriod(channel);
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  if (firstByte == 1) { //modulation wheel
    if (secondByte > 0) {
      float valF = secondByte;
      enableVibrato[channel] = true;
      if (secondByte > 64) {
        maxVibrato[channel] = 0.03;
      }
      else {
        maxVibrato[channel] = 0.02;
      }
    }
    else {
      enableVibrato[channel] = false;
      bendFactor[channel] = originalBend[channel];
      calculatePeriod(channel);
    }
  }
  if (firstByte == 101) {
    controlVal1 = secondByte;
  }
  if (firstByte == 100) {
    controlVal2 = secondByte;
  }
  if (firstByte == 6 && controlVal1 == 0 && controlVal2 == 0) { //pitch bend sensitivity
    bendSens[channel] = secondByte;
    controlVal1 = -1;
    controlVal2 = -1;
  }
  if (firstByte == 121) { //reset all controllers
    resetAll();
  }
  if (firstByte == 120 || firstByte == 123) { //mute all sounds
    mute();
  }
}

void resetAll() {
  for (int i = 0; i < 5; i++) {
    originalPitch[i] = 0;
    period[i] = 0;
    enableVibrato[i] = 0;
    bendFactor[i] = 1;
    bendSens[i] = 2;
  }
}

void mute() {
  for (int i = 0; i < 5; i++) {
    originalPitch[i] = 0;
    period[i] = 0;
  }
}

void calculatePeriod(byte channel) {
  if (originalPitch[channel] > 0) {
    period[channel] = pitchVals[originalPitch[channel]] * bendFactor[channel];
  }
}

void readMIDI() {
  if (Serial.available() > 3) {
    byte statusByte = Serial.read();
    byte channel = Serial.read();
    byte firstByte = Serial.read();
    byte secondByte = Serial.read();
    if (statusByte == '1') { //note on
      handleNoteOn(channel, firstByte, secondByte);
    }
    else if (statusByte == '2') { //note off
      handleNoteOff(channel, firstByte, secondByte);
    }
    else if (statusByte == '3') { //pitch bend
      int midiBendVal = (secondByte << 7) | firstByte;
      handlePitchBend(channel, (midiBendVal - 8192) * -1);
    }
    else if (statusByte == '4') { //control change
      handleControlChange(channel, firstByte, secondByte);
    }
  }
}
