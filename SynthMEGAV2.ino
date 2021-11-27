//Floppy Arduino
#include <TimerOne.h>
#include "pitches.h"
#define CLR(x,y) (x&=(~_BV(y)))
#define SET(x,y) (x|=(_BV(y)))

//Floppy Drives channel_floppy#:
#define floppyStep5_1 26 //CLR(PORTA,4)
#define floppyStep5_2 28 //CLR(PORTA,6)
#define floppyStep5_3 30 //CLR(PORTC,7)
#define floppyDir5_1 27 //CLR(PORTA,5)
#define floppyDir5_2 29 //CLR(PORTA,7)
#define floppyDir5_3 31 //CLR(PORTC,6)

#define floppyStep6_1 32 //CLR(PORTC,5)
#define floppyStep6_2 34 //CLR(PORTC,3)
#define floppyStep6_3 36 //CLR(PORTC,1)
#define floppyDir6_1 33 //CLR(PORTC,4)
#define floppyDir6_2 35 //CLR(PORTC,2)
#define floppyDir6_3 37 //CLR(PORTC,0)

#define floppyStep7_1 38 //CLR(PORTD,7)
#define floppyStep7_2 40 //CLR(PORTG,1)
#define floppyStep7_3 42 //CLR(PORTL,7)
#define floppyDir7_1 39 //CLR(PORTG,2)
#define floppyDir7_2 41 //CLR(PORTG,0)
#define floppyDir7_3 43 //CLR(PORTL,6)

//Hard Drives:
#define bassDrum 2
#define snareDrum 3
#define highDrum 4
#define highDrum2 5

//MIDI Configuration:
#define timerResolution 80000 //vibrato
bool floppyDrum = false; //enable floppy drum channel 7

unsigned int period[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long prevMicros[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long floppyEnvelope[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long prevDrum[] = {0, 0, 0, 0, 0}; //bass drum, snare drum, high drum 2, high drum, floppy drum
int drumDuration[] = { -1, -1, -1, -1, -1};
byte originalPitch[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte bendSens[] = {2, 2, 2, 2, 2, 2, 2, 2};
float bendFactor[] = {1, 1, 1, 1, 1, 1, 1, 1};
float originalBend[] = {1, 1, 1, 1, 1, 1, 1, 1};
float maxVibrato[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool enableVibrato[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool vibratoState[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool floppyDir[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool bendFlag[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte controlVal1 = -1;
byte controlVal2 = -1;

byte midiNote;
byte midiBendLsb;
byte midiControlNum;
byte midiAction = 0;
byte offsetChannel;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  Timer1.initialize(timerResolution);
  Timer1.attachInterrupt(vibrato);
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = 22; i < 54; i++) {
    pinMode(i, OUTPUT);
  }
  resetHead(); //run all floppy head to middle
}

void loop() {
  readMIDI();
  floppyDrives(5);
  floppyDrives(6);
  floppyDrives(7);
  drum();
}

void floppyDrives(byte channel) {
  if (period[channel] > 0 && micros() - prevMicros[channel] >= period[channel]) {
    if (channel == 5) {
      toggleFloppy(0);
      if (bendFlag[5] || micros() - floppyEnvelope[5] >= 10000 && micros() - floppyEnvelope[5] <= 200000) {
        toggleFloppy(1);
      }
      else {
        disableFloppy(1);
      }
      if (micros() - floppyEnvelope[5] >= 20000 && micros() - floppyEnvelope[5] <= 100000) {
        toggleFloppy(2);
      }
      else {
        disableFloppy(2);
      }
    }
    if (channel == 6) {
      toggleFloppy(3);
      if (bendFlag[6] || micros() - floppyEnvelope[6] >= 10000 && micros() - floppyEnvelope[6] <= 200000) {
        toggleFloppy(4);
      }
      else {
        disableFloppy(4);
      }
      if (micros() - floppyEnvelope[6] >= 20000 && micros() - floppyEnvelope[6] <= 100000) {
        toggleFloppy(5);
      }
      else {
        disableFloppy(5);
      }
    }
    if (channel == 7) {
      toggleFloppy(6);
      if (bendFlag[7] || micros() - floppyEnvelope[7] >= 10000 && micros() - floppyEnvelope[7] <= 200000) {
        toggleFloppy(7);
      }
      else {
        disableFloppy(7);
      }
      if (micros() - floppyEnvelope[7] >= 20000 && micros() - floppyEnvelope[7] <= 100000) {
        toggleFloppy(8);
      }
      else {
        disableFloppy(8);
      }
    }
    if (floppyDir[channel]) {
      if (channel == 5) {
        SET(PORTA, 5); //floppy 1
        SET(PORTA, 7); //floppy 2
        SET(PORTC, 6); //floppy 3
      }
      if (channel == 6) {
        SET(PORTA, 3); //floppy 1
        SET(PORTC, 2); //floppy 2
        SET(PORTC, 0); //floppy 3
      }
      if (channel == 7) {
        SET(PORTG, 2); //floppy 1
        SET(PORTG, 0); //floppy 2
        SET(PORTL, 6); //floppy 3
      }
    }
    else {
      if (channel == 5) {
        CLR(PORTA, 5); //floppy 1
        CLR(PORTA, 7); //floppy 2
        CLR(PORTC, 6); //floppy 3
      }
      if (channel == 6) {
        CLR(PORTA, 3); //floppy 1
        CLR(PORTC, 2); //floppy 2
        CLR(PORTC, 0); //floppy 3
      }
      if (channel == 7) {
        CLR(PORTG, 2); //floppy 1
        CLR(PORTG, 0); //floppy 2
        CLR(PORTL, 6); //floppy 3
      }
    }
    floppyDir[channel] = !floppyDir[channel];
    prevMicros[channel] = micros();
  }
}

void toggleFloppy(byte index) {
  if (index == 0) {
    SET(PORTA, 4); //floppy 1 channel 5
    CLR(PORTA, 4); //floppy 1 channel 5
  }
  if (index == 1) {
    SET(PORTA, 6); //floppy 2 channel 5
    CLR(PORTA, 6); //floppy 2 channel 5
  }
  if (index == 2) {
    SET(PORTC, 7); //floppy 3 channel 5
    CLR(PORTC, 7); //floppy 3 channel 5
  }
  if (index == 3) {
    SET(PORTA, 2); //floppy 1 channel 6
    CLR(PORTA, 2); //floppy 1 channel 6
  }
  if (index == 4) {
    SET(PORTC, 3); //floppy 2 channel 6
    CLR(PORTC, 3); //floppy 2 channel 6
  }
  if (index == 5) {
    SET(PORTC, 1); //floppy 3 channel 6
    CLR(PORTC, 1); //floppy 3 channel 6
  }
  if (index == 6) {
    SET(PORTD, 7); //floppy 1 channel 7
    CLR(PORTD, 7); //floppy 1 channel 7
  }
  if (index == 7) {
    SET(PORTG, 1); //floppy 2 channel 7
    CLR(PORTG, 1); //floppy 2 channel 7
  }
  if (index == 8) {
    SET(PORTL, 7); //floppy 3 channel 7
    CLR(PORTL, 7); //floppy 3 channel 7
  }
}

void disableFloppy(byte index) {
  if (index == 0) {
    SET(PORTA, 4); //floppy 1 channel 5
  }
  if (index == 1) {
    SET(PORTA, 6); //floppy 2 channel 5
  }
  if (index == 2) {
    SET(PORTC, 7); //floppy 3 channel 5
  }
  if (index == 3) {
    SET(PORTA, 2); //floppy 1 channel 6
  }
  if (index == 4) {
    SET(PORTC, 3); //floppy 2 channel 6
  }
  if (index == 5) {
    SET(PORTC, 1); //floppy 3 channel 6
  }
  if (index == 6) {
    SET(PORTD, 7); //floppy 1 channel 7
  }
  if (index == 7) {
    SET(PORTG, 1); //floppy 2 channel 7
  }
  if (index == 8) {
    SET(PORTL, 7); //floppy 3 channel 7
  }
}

void drum() {
  if (drumDuration[0] != -1 && micros() - prevDrum[0] >= drumDuration[0]) {
    drumDuration[0] = -1;
    CLR(PORTE, 4);
  }
  if (drumDuration[1] != -1 && micros() - prevDrum[1] >= drumDuration[1]) {
    drumDuration[1] = -1;
    CLR(PORTE, 5);
  }
  if (drumDuration[2] != -1 && micros() - prevDrum[2] >= drumDuration[2]) {
    drumDuration[2] = -1;
    CLR(PORTE, 3);
  }
  if (drumDuration[3] != -1 && micros() - prevDrum[3] >= drumDuration[3]) {
    drumDuration[3] = -1;
    CLR(PORTG, 5);
  }
  if (drumDuration[4] != -1 && micros() - prevDrum[4] >= drumDuration[4]) {
    drumDuration[4] = -1;
    SET(PORTD, 7);
    SET(PORTG, 1);
    SET(PORTL, 7);
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (channel > 4 && channel < 8) {
    if(velocity == 0) {
      handleNoteOff(channel, note, velocity);
    }
    else if (note < 71) {
      if (floppyDrum && channel == 7) {
        //ignore message
      }
      else {
        originalPitch[channel] = note;
        prevMicros[channel] = micros();
        floppyEnvelope[channel] = prevMicros[channel];
        calculatePeriod(channel);
      }
    }
  }
  else if (channel == 10 && velocity > 0) {
    if (note == 35 || note == 36) { //bass drum
      SET(PORTE, 4);
      if (velocity > 119) {
        drumDuration[0] = 2500;
      }
      else {
        drumDuration[0] = 2000;
      }
      prevDrum[0] = micros();
    }
    else if (note == 38 || note == 40) { //snare drum
      SET(PORTE, 5);
      if (velocity > 119) {
        drumDuration[1] = 6000;
      }
      else {
        drumDuration[1] = 2000;
      }
      prevDrum[1] = micros();
    }
    else if (note == 39 || note == 48 || note == 50 || note == 54 || note == 46 || note == 51
             || note == 59 || note == 49 || note == 57) { //high toms
      SET(PORTE, 3);
      drumDuration[2] = 1500;
      prevDrum[2] = micros();
    }
    else { //low toms
      if (floppyDrum) {
        CLR(PORTD, 7);
        CLR(PORTG, 1);
        CLR(PORTL, 7);
        if (floppyDir[7]) {
          SET(PORTG, 2); //floppy 1
          SET(PORTG, 0); //floppy 2
          SET(PORTL, 6); //floppy 3
        }
        else {
          CLR(PORTG, 2); //floppy 1
          CLR(PORTG, 0); //floppy 2
          CLR(PORTL, 6); //floppy 3
        }
        floppyDir[7] = !floppyDir[7];
        drumDuration[4] = 10000;
        prevDrum[4] = micros();
      }
      else {
        SET(PORTG, 5);
        drumDuration[3] = 300;
        prevDrum[3] = micros();
      }
    }
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (channel > 4 && channel < 8) {
    if (note == originalPitch[channel]) {
      period[channel] = 0;
      originalPitch[channel] = 0;
      if (channel == 5) {
        SET(PORTA, 4);
        SET(PORTA, 6);
        SET(PORTC, 7);
      }
      else if (channel == 6) {
        SET(PORTA, 2);
        SET(PORTC, 3);
        SET(PORTC, 1);
      }
      else if (channel == 7) {
        SET(PORTD, 7);
        SET(PORTG, 1);
        SET(PORTL, 7);
      }
    }
  }
}

void handlePitchBend(byte channel, int bend) {
  if (channel > 4 && channel < 8) {
    int mapped = map(bend, -8192, 8191, 0, 63);
    if (bendSens[channel] == 2) {
      bendFactor[channel] = bendVal2[mapped];
    }
    else if (bendSens[channel] == 12) {
      bendFactor[channel] = bendVal12[mapped];
    }
    if(bend > 2048 && bend < 6144 || bend < -2048 && bend > -6144) {
      bendFlag[channel] = true;
    }
    else {
      bendFlag[channel] = false;
    }
    originalBend[channel] = bendFactor[channel];
    calculatePeriod(channel);
  }
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  if (channel > 4 && channel < 8) {
    if (firstByte == 1) { //modulation wheel
      if (secondByte > 0) {
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
}

void resetHead() {
  for (int i = 24; i < 54; i++) {
    digitalWrite(i, LOW);
  }
  for (int i = 0; i < 80; i++) {
    SET(PORTA, 4);
    SET(PORTA, 6);
    SET(PORTC, 7);
    SET(PORTA, 2);
    SET(PORTC, 3);
    SET(PORTC, 1);
    SET(PORTD, 7);
    SET(PORTG, 1);
    SET(PORTL, 7);
    CLR(PORTA, 4);
    CLR(PORTA, 6);
    CLR(PORTC, 7);
    CLR(PORTA, 2);
    CLR(PORTC, 3);
    CLR(PORTC, 1);
    CLR(PORTD, 7);
    CLR(PORTG, 1);
    CLR(PORTL, 7);
    delay(5);
  }
  for (int i = 24; i < 54; i++) {
    digitalWrite(i, HIGH);
  }
  for (int i = 0; i < 40; i++) {
    SET(PORTA, 4);
    SET(PORTA, 6);
    SET(PORTC, 7);
    SET(PORTA, 2);
    SET(PORTC, 3);
    SET(PORTC, 1);
    SET(PORTD, 7);
    SET(PORTG, 1);
    SET(PORTL, 7);
    CLR(PORTA, 4);
    CLR(PORTA, 6);
    CLR(PORTC, 7);
    CLR(PORTA, 2);
    CLR(PORTC, 3);
    CLR(PORTC, 1);
    CLR(PORTD, 7);
    CLR(PORTG, 1);
    CLR(PORTL, 7);
    delay(5);
  }
  for (int i = 24; i < 54; i += 2) {
    digitalWrite(i, HIGH);
  }
}

void vibrato() {
  for (int i = 5; i < 8; i++) {
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

void resetAll() {
  for (int i = 5; i < 8; i++) {
    originalPitch[i] = 0;
    period[i] = 0;
    enableVibrato[i] = 0;
    bendFlag[i] = 0;
    bendFactor[i] = 1;
    bendSens[i] = 2;
  }
  resetHead();
}

void mute() {
  for (int i = 5; i < 8; i++) {
    originalPitch[i] = 0;
    period[i] = 0;
  }
  for (int i = 24; i < 54; i += 2) {
    digitalWrite(i, HIGH);
  }
}

void calculatePeriod(byte channel) {
  if (originalPitch[channel] > 0) {
    period[channel] = pitchVals[originalPitch[channel]] * bendFactor[channel];
  }
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
        if (offsetChannel > 0 && offsetChannel < 5) {
          Serial1.write('1');
          Serial1.write(offsetChannel);
          Serial1.write(midiNote);
          Serial1.write(incomingByte);
        }
        else {
          handleNoteOn(offsetChannel, midiNote, incomingByte);
        }
        midiAction = 0;
      }
      else if (midiAction == 3) {
        midiNote = incomingByte;
        midiAction = 4;
      }
      else if (midiAction == 4) {
        if (offsetChannel > 0 && offsetChannel < 5) {
          Serial1.write('2');
          Serial1.write(offsetChannel);
          Serial1.write(midiNote);
          Serial1.write(incomingByte);
        }
        else {
          handleNoteOff(offsetChannel, midiNote, incomingByte);
        }
        midiAction = 0;
      }
      else if (midiAction == 5) { //receive bendmsb
        midiBendLsb = incomingByte;
        midiAction = 6;
      }
      else if (midiAction == 6) {
        if (offsetChannel > 0 && offsetChannel < 5) {
          Serial1.write('3');
          Serial1.write(offsetChannel);
          Serial1.write(midiBendLsb);
          Serial1.write(incomingByte);
        }
        else {
          int midiBendVal = (incomingByte << 7) | midiBendLsb;
          handlePitchBend(offsetChannel, (midiBendVal - 8192) * -1);
        }
        midiAction = 0;
      }
      else if (midiAction == 7) {
        midiControlNum = incomingByte;
        midiAction = 8;
      }
      else if (midiAction == 8) {
        if (offsetChannel > 0 && offsetChannel < 5) {
          Serial1.write('4');
          Serial1.write(offsetChannel);
          Serial1.write(midiControlNum);
          Serial1.write(incomingByte);
        }
        else {
          handleControlChange(offsetChannel, midiControlNum, incomingByte);
        }
        midiAction = 0;
      }
    }
  }
}
