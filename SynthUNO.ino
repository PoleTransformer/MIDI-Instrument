//Stepper Arduino
#include <TimerOne.h>
#include "pitches.h"
#define CLR(x,y) (x&=(~_BV(y)))
#define SET(x,y) (x|=(_BV(y)))

#define stepPin_M1 2 //CLR(PORTD,2)
#define stepPin_M2 4 //CLR(PORTD,4)
#define stepPin_M3 6 //CLR(PORTD,6)
#define stepPin_M4 8 //CLR(PORTB,0)
#define dirPin_M1 3 //CLR(PORTD,3)
#define dirPin_M2 5 //CLR(PORTD,5)
#define dirPin_M3 7 //CLR(PORTD,7)
#define dirPin_M4 9 //CLR(PORTB,1)
#define enable1 10 //CLR(PORTB,2)
#define enable2 11 //CLR(PORTB,3)
#define enable3 12 //CLR(PORTB,4)
#define enable4 13 //CLR(PORTB,5)
#define timerResolution 20

unsigned int currentPeriod[] = {0, 0, 0, 0, 0};
unsigned int currentTick[] = {0, 0, 0, 0, 0};
unsigned int originalPeriod[] = {0, 0, 0, 0, 0};
float bendFactor[] = {1, 1, 1, 1, 1};
bool currentState[] = {0, 0, 0, 0, 0};
byte bendSens[] = {2, 2, 2, 2, 2};
byte controlVal1 = -1;
byte controlVal2 = -1;

void setup() {
  Timer1.initialize(timerResolution);
  Timer1.attachInterrupt(tick);
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  Serial.begin(115200); //Allows for serial MIDI communication.
}


void loop() {
  readMIDI();
}

void tick() {
  for (int i = 1; i < 5; i++) {
    if (currentPeriod[i] > 0) {
      currentTick[i]++;
      if (currentTick[i] >= currentPeriod[i]) {
        togglePin(i);
        currentTick[i] = 0;
      }
    }
  }
}

void togglePin(byte i) {
  if (currentState[i]) {
    if (i == 1) {
      SET(PORTD, 3);
    }
    if (i == 2) {
      SET(PORTD, 5);
    }
    if (i == 3) {
      SET(PORTD, 7);
    }
    if (i == 4) {
      SET(PORTB, 1);
    }
  }
  else {
    if (i == 1) {
      CLR(PORTD, 3);
    }
    if (i == 2) {
      CLR(PORTD, 5);
    }
    if (i == 3) {
      CLR(PORTD, 7);
    }
    if (i == 4) {
      CLR(PORTB, 1);
    }
  }
  currentState[i] = !currentState[i];
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

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  if (channel > 0 && channel < 5 && velocity > 0) {
    currentPeriod[channel] = originalPeriod[channel] = pitchVals[pitch] / (timerResolution * 2);
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  if (channel > 0 && channel < 5) {
    if (pitchVals[pitch] / (timerResolution * 2) == originalPeriod[channel]) {
      currentPeriod[channel] = originalPeriod[channel] = 0;
    }
  }
}

void handlePitchBend(byte channel, int bend) {
  if (channel > 0 && channel < 5) {
    float bendF = bend;
    bendF /= 8192;
    bendF *= bendSens[channel];
    bendF /= 12;
    currentPeriod[channel] = originalPeriod[channel] * pow(2, bendF);
  }
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  if (channel > 0 && channel < 5) {
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

void resetAll() {
  for (int i = 0; i < 5; i++) {
    currentPeriod[i] = originalPeriod[i] = 0;
    bendFactor[i] = 1;
    bendSens[i] = 2;
  }
}

void mute() {
  for (int i = 0; i < 5; i++) {
    currentPeriod[i] = originalPeriod[i] = 0;
  }
}
