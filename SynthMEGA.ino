//Seng's MIDI Instrument Arduino Mega 2560
#include <digitalWriteFast.h>
#include <TimerOne.h>
#include "pitches.h"
//#define CLR(x,y) (x&=(~_BV(y)))
//#define SET(x,y) (x|=(_BV(y)))

//Floppy Drives channel_floppy#:
//Each channel is assigned to 4 rows
//Even number for step pin and direction pin is always +1 corresponding step pin
#define floppyStep5_1 22
#define floppyStep5_2 24
#define floppyStep5_3 26
//#define floppyStep5_4 28
#define floppyDir5_1 23
#define floppyDir5_2 25
#define floppyDir5_3 27
//#define floppyDir5_4 29

#define floppyStep6_1 30
#define floppyStep6_2 32
#define floppyStep6_3 34
//#define floppyStep6_4 36
#define floppyDir6_1 31
#define floppyDir6_2 33
#define floppyDir6_3 35
//#define floppyDir6_4 37

#define floppyStep7_1 38
#define floppyStep7_2 40
#define floppyStep7_3 42
//#define floppyStep7_4 44
#define floppyDir7_1 39
#define floppyDir7_2 41
#define floppyDir7_3 43
//#define floppyDir7_4 45

//Stepper Motors
#define stepPin1 6
#define stepPin2 7
#define stepPin3 8
#define stepPin4 9

//Hard Drives:
#define bassDrum 2
#define snareDrum 3
#define highDrum 4
#define highDrum2 5

//MIDI Configuration:
#define timerResolution 80000 //vibrato (ms)

unsigned int period[17];
unsigned long prevMicros[17];
unsigned long floppyEnvelope[17];
unsigned long prevDrum[17];
int drumDuration[17];
byte originalPitch[17];
byte acceleration[17];
byte bendSens[17];
float bendFactor[17];
float originalBend[17];
float maxVibrato[17];
bool vibratoState[17];
bool floppyDir[17];
bool bendFlag[17];
byte controlVal1 = -1;
byte controlVal2 = -1;

byte midiNote;
byte midiBendLsb;
byte midiControlNum;
byte midiAction = 0;
byte offsetChannel;

void setup() {
  Serial.begin(115200);
  Timer1.initialize(timerResolution);
  Timer1.attachInterrupt(vibrato);
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = 22; i < 54; i++) {
    pinMode(i, OUTPUT);
  }
  mute(); //initialize all variables
  resetHead(); //run all floppy head to middle
}

void loop() {
  readMIDI();
  stepperMotors(1);
  stepperMotors(2);
  stepperMotors(3);
  stepperMotors(4);
  floppyDrives(5);
  floppyDrives(6);
  floppyDrives(7);
  drum();
}

void stepperMotors(byte channel) {
  if (period[channel] > 0 && micros() - prevMicros[channel] >= period[channel]) {
    if (channel == 1) {
      digitalWriteFast(stepPin1, HIGH);
      digitalWriteFast(stepPin1, LOW);
    }
    if (channel == 2) {
      digitalWriteFast(stepPin2, HIGH);
      digitalWriteFast(stepPin2, LOW);
    }
    if (channel == 3) {
      digitalWriteFast(stepPin3, HIGH);
      digitalWriteFast(stepPin3, LOW);
    }
    if (channel == 4) {
      digitalWriteFast(stepPin4, HIGH);
      digitalWriteFast(stepPin4, LOW);
    }
    if (originalPitch[channel] < acceleration[channel]) {
      originalPitch[channel]++;
      calculatePeriod(channel);
    }
    prevMicros[channel] = micros();
  }
}

void floppyDrives(byte channel) {
  if (period[channel] > 0 && micros() - prevMicros[channel] >= period[channel]) {
    if (channel == 5) {
      digitalWriteFast(floppyStep5_1, HIGH);
      digitalWriteFast(floppyStep5_1, LOW);
      if (bendFlag[5] || micros() - floppyEnvelope[5] >= 10000 && micros() - floppyEnvelope[5] <= 200000) {
        digitalWriteFast(floppyStep5_2, HIGH);
        digitalWriteFast(floppyStep5_2, LOW);
      }
      else {
        digitalWriteFast(floppyStep5_2, HIGH);
      }
      if (micros() - floppyEnvelope[5] >= 20000 && micros() - floppyEnvelope[5] <= 100000) {
        digitalWriteFast(floppyStep5_3, HIGH);
        digitalWriteFast(floppyStep5_3, LOW);
      }
      else {
        digitalWriteFast(floppyStep5_3, HIGH);
      }
    }
    if (channel == 6) {
      digitalWriteFast(floppyStep6_1, HIGH);
      digitalWriteFast(floppyStep6_1, LOW);
      if (bendFlag[6] || micros() - floppyEnvelope[6] >= 10000 && micros() - floppyEnvelope[6] <= 200000) {
        digitalWriteFast(floppyStep6_2, HIGH);
        digitalWriteFast(floppyStep6_2, LOW);
      }
      else {
        digitalWriteFast(floppyStep6_2, HIGH);
      }
      if (micros() - floppyEnvelope[6] >= 20000 && micros() - floppyEnvelope[6] <= 100000) {
        digitalWriteFast(floppyStep6_3, HIGH);
        digitalWriteFast(floppyStep6_3, LOW);
      }
      else {
        digitalWriteFast(floppyStep6_3, HIGH);
      }
    }
    if (channel == 7) {
      digitalWriteFast(floppyStep7_1, HIGH);
      digitalWriteFast(floppyStep7_1, LOW);
      if (bendFlag[7] || micros() - floppyEnvelope[7] >= 10000 && micros() - floppyEnvelope[7] <= 200000) {
        digitalWriteFast(floppyStep7_2, HIGH);
        digitalWriteFast(floppyStep7_2, LOW);
      }
      else {
        digitalWriteFast(floppyStep7_2, HIGH);
      }
      if (micros() - floppyEnvelope[7] >= 20000 && micros() - floppyEnvelope[7] <= 100000) {
        digitalWriteFast(floppyStep7_3, HIGH);
        digitalWriteFast(floppyStep7_3, LOW);
      }
      else {
        digitalWriteFast(floppyStep7_3, HIGH);
      }
    }
    if (floppyDir[channel]) {
      if (channel == 5) {
        digitalWriteFast(floppyDir5_1, HIGH);
        digitalWriteFast(floppyDir5_2, HIGH);
        digitalWriteFast(floppyDir5_3, HIGH);
      }
      if (channel == 6) {
        digitalWriteFast(floppyDir6_1, HIGH);
        digitalWriteFast(floppyDir6_2, HIGH);
        digitalWriteFast(floppyDir6_3, HIGH);
      }
      if (channel == 7) {
        digitalWriteFast(floppyDir7_1, HIGH);
        digitalWriteFast(floppyDir7_2, HIGH);
        digitalWriteFast(floppyDir7_3, HIGH);
      }
    }
    else {
      if (channel == 5) {
        digitalWriteFast(floppyDir5_1, LOW);
        digitalWriteFast(floppyDir5_2, LOW);
        digitalWriteFast(floppyDir5_3, LOW);
      }
      if (channel == 6) {
        digitalWriteFast(floppyDir6_1, LOW);
        digitalWriteFast(floppyDir6_2, LOW);
        digitalWriteFast(floppyDir6_3, LOW);
      }
      if (channel == 7) {
        digitalWriteFast(floppyDir7_1, LOW);
        digitalWriteFast(floppyDir7_2, LOW);
        digitalWriteFast(floppyDir7_3, LOW);
      }
    }
    floppyDir[channel] = !floppyDir[channel];
    prevMicros[channel] = micros();
  }
}

void drum() {
  if (drumDuration[0] != -1 && micros() - prevDrum[0] >= drumDuration[0]) {
    drumDuration[0] = -1;
    digitalWriteFast(bassDrum, LOW);
  }
  if (drumDuration[1] != -1 && micros() - prevDrum[1] >= drumDuration[1]) {
    drumDuration[1] = -1;
    digitalWriteFast(snareDrum, LOW);
  }
  if (drumDuration[2] != -1 && micros() - prevDrum[2] >= drumDuration[2]) {
    drumDuration[2] = -1;
    digitalWriteFast(highDrum2, LOW);
  }
  if (drumDuration[3] != -1 && micros() - prevDrum[3] >= drumDuration[3]) {
    drumDuration[3] = -1;
    digitalWriteFast(highDrum, LOW);
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (velocity == 0) {
    handleNoteOff(channel, note, velocity);
  }
  else if (channel == 10) {
    if (note == 35 || note == 36) { //bass drum
      digitalWriteFast(bassDrum, HIGH);
      drumDuration[0] = 2000;
      prevDrum[0] = micros();
    }
    else if (note == 38 || note == 40) { //snare drum
      digitalWriteFast(snareDrum, HIGH);
      if (velocity > 119) {
        drumDuration[1] = 6000;
      }
      else {
        drumDuration[1] = 2000;
      }
      prevDrum[1] = micros();
    }
    else if (note == 39 || note == 48 || note == 50 || note == 54 || note == 46 || note == 51
             || note == 59 || note == 49 || note == 57 || note == 37) { //high toms
      digitalWriteFast(highDrum2, HIGH);
      drumDuration[2] = 1500;
      prevDrum[2] = micros();
    }
    else { //low toms
      digitalWriteFast(highDrum, HIGH);
      drumDuration[3] = 200;
      prevDrum[3] = micros();
    }
  }
  else {
    if (note > 78) {
      acceleration[channel] = note;
      originalPitch[channel] = 78;
    }
    else {
      acceleration[channel] = note;
      originalPitch[channel] = note;
    }
    floppyEnvelope[channel] = micros();
    calculatePeriod(channel);
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (note == originalPitch[channel]) {
    period[channel] = 0;
    originalPitch[channel] = 0;
    if (channel == 5) {
      digitalWriteFast(floppyStep5_1, HIGH);
      digitalWriteFast(floppyStep5_2, HIGH);
      digitalWriteFast(floppyStep5_3, HIGH);
    }
    else if (channel == 6) {
      digitalWriteFast(floppyStep6_1, HIGH);
      digitalWriteFast(floppyStep6_2, HIGH);
      digitalWriteFast(floppyStep6_3, HIGH);
    }
    else if (channel == 7) {
      digitalWriteFast(floppyStep7_1, HIGH);
      digitalWriteFast(floppyStep7_2, HIGH);
      digitalWriteFast(floppyStep7_3, HIGH);
    }
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
  if (bend > 2048 && bend < 6144 || bend < -2048 && bend > -6144) {
    bendFlag[channel] = true;
  }
  else {
    bendFlag[channel] = false;
  }
  originalBend[channel] = bendFactor[channel];
  calculatePeriod(channel);
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  if (firstByte == 1) { //modulation wheel
    maxVibrato[channel] = mapF(secondByte, 0.0, 127.0, 0.0, 0.03);
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
    mute();
    resetHead();
  }
  if (firstByte == 120 || firstByte == 123) { //mute all sounds
    mute();
  }
}

void vibrato() {
  for (int i = 1; i < 17; i++) {
    if (maxVibrato[i] > 0) {
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

void resetHead() {
  for (int i = 22; i <= 44; i++) {
    digitalWrite(i, LOW);
  }
  for (int i = 0; i < 80; i++) {
    for (int i = 22; i <= 44; i += 2) {
      digitalWrite(i, HIGH);
      digitalWrite(i, LOW);
    }
    delay(5);
  }
  for (int i = 24; i <= 44; i++) {
    digitalWrite(i, HIGH);
  }
  for (int i = 0; i < 80; i++) {
    for (int i = 22; i <= 44; i += 2) {
      digitalWrite(i, HIGH);
      digitalWrite(i, LOW);
    }
    delay(5);
  }
  for (int i = 22; i <= 44; i += 2) {
    digitalWrite(i, HIGH);
  }
}

void mute() {
  for (int i = 0; i < 17; i++) {
    period[i] = 0;
    prevMicros[i] = 0;
    floppyEnvelope[i] = 0;
    prevDrum[i] = 0;
    drumDuration[i] = -1;
    originalPitch[i] = 0;
    bendSens[i] = 2;
    bendFactor[i] = 1;
    originalBend[i] = 0;
    maxVibrato[i] = 0;
    vibratoState[i] = 0;
    floppyDir[i] = 0;
    bendFlag[i] = 0;
    acceleration[i] = 0;
  }
}

void calculatePeriod(byte channel) {
  if (originalPitch[channel] > 0) {
    period[channel] = pitchVals[originalPitch[channel]] * bendFactor[channel];
  }
}

float mapF(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void readMIDI() {
  if (Serial.available() > 0) {
    byte incomingByte = Serial.read();
    byte command = (incomingByte & 0b10000000) >> 7;
    if (command == 1) { //status
      byte midiMsb = (incomingByte & 0b11110000) >> 4;
      offsetChannel = (incomingByte & 0b00001111) + 1;
      if (midiMsb == 9) { //note on
        midiAction = 1;
      }
      else if (midiMsb == 8) { //note off
        midiAction = 3;
      }
      else if (midiMsb == 14) { //pitch bend
        midiAction = 5;
      }
      else if (midiMsb == 11) { //control change
        midiAction = 7;
      }
    }
    else {
      if (midiAction == 1) {
        midiNote = incomingByte;
        midiAction = 2;
      }
      else if (midiAction == 2) { //velocity
        handleNoteOn(offsetChannel, midiNote, incomingByte);
        midiAction = 0;
      }
      else if (midiAction == 3) {
        midiNote = incomingByte;
        midiAction = 4;
      }
      else if (midiAction == 4) {
        handleNoteOff(offsetChannel, midiNote, incomingByte);
        midiAction = 0;
      }
      else if (midiAction == 5) { //receive bendmsb
        midiBendLsb = incomingByte;
        midiAction = 6;
      }
      else if (midiAction == 6) {
        int midiBendVal = (incomingByte << 7) | midiBendLsb;
        handlePitchBend(offsetChannel, (midiBendVal - 8192) * -1);
        midiAction = 0;
      }
      else if (midiAction == 7) {
        midiControlNum = incomingByte;
        midiAction = 8;
      }
      else if (midiAction == 8) {
        handleControlChange(offsetChannel, midiControlNum, incomingByte);
        midiAction = 0;
      }
    }
  }
}
