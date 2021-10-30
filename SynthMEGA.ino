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
bool enFloppyDrum = false; //enable floppy drum on channel 7
#define timerResolution 40

unsigned int currentPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int currentTick[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int originalPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0};
float bendFactor[] = {1, 1, 1, 1, 1, 1, 1, 1};
bool currentState[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool floppyDir[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte stepCount[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte bendSens[] = {2, 2, 2, 2, 2, 2, 2, 2};
unsigned long floppyEnvelope[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long prevDrum[] = {0, 0, 0, 0, 0}; //bass drum, snare drum, high drum 2, high drum
byte controlVal1 = -1;
byte controlVal2 = -1;

byte midiNote;
byte midiBendLsb;
byte midiControlNum;
byte midiAction = 0;
byte offsetChannel;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200); //Allows for serial MIDI communication. Comment this line when using mocaLUFA.
  Serial1.begin(115200); //Sends MIDI data to stepper Arduino
  Timer1.initialize(timerResolution);
  Timer1.attachInterrupt(tick);
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = 22; i < 54; i++) {
    pinMode(i, OUTPUT);
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

void loop() {
  readMIDI();
  drum();
}

void drum() {
  unsigned long currentMicros = micros();
  if (prevDrum[0] > 0) { //bass drum
    if (currentMicros - prevDrum[0] >= 2000) {
      prevDrum[0] = 0;
      CLR(PORTE, 4);
    }
  }
  if (prevDrum[1] > 0) { //snare drum
    if (currentMicros - prevDrum[1] >= 1250) {
      prevDrum[1] = 0;
      CLR(PORTE, 5);
    }
  }
  if (prevDrum[2] > 0) { //high drum 2
    if (currentMicros - prevDrum[2] >= 1500) {
      prevDrum[2] = 0;
      CLR(PORTE, 3);
    }
  }
  if (prevDrum[3] > 0) { //high drum
    if (currentMicros - prevDrum[3] >= 500) {
      prevDrum[3] = 0;
      CLR(PORTG, 5);
    }
  }
  if (prevDrum[4] > 0) {
    if (currentMicros - prevDrum[4] >= 10000) {
      SET(PORTL, 7);
      SET(PORTG, 1);
      SET(PORTD, 7);
      prevDrum[4] = 0;
    }
  }
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (channel >= 5 && channel <= 10 && velocity > 0) {
    unsigned long currentMicros = micros();
    if (channel <= 7 && pitch < 71) { //Floppy Drives
      if (enFloppyDrum && channel == 7) {
        //ignore message
      }
      else {
        currentPeriod[channel] = originalPeriod[channel] = pitchVals[pitch] / (timerResolution * 2);
        floppyEnvelope[channel] = currentMicros;
      }
    }
    else if (channel == 10) {
      if (pitch == 35 || pitch == 36) { //bass drum
        SET(PORTE, 4);
        prevDrum[0] = currentMicros;
      }
      else if (pitch == 38 || pitch == 40) { //snare drum
        SET(PORTE, 5);
        prevDrum[1] = currentMicros;
      }
      else if (pitch == 39 || pitch == 48 || pitch == 50 || pitch == 54 || pitch == 46 || pitch == 51
               || pitch == 59 || pitch == 49 || pitch == 57) { //high drum 2
        SET(PORTE, 3);
        prevDrum[2] = currentMicros;
      }
      else { //high drum
        if (enFloppyDrum) {
          floppyDrum();
        }
        else {
          SET(PORTG, 5);
          prevDrum[3] = currentMicros;
        }
      }
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  if (channel >= 5 && channel <= 7)
  {
    if (pitchVals[pitch] / (timerResolution * 2) == originalPeriod[channel]) {
      currentPeriod[channel] = originalPeriod[channel] = 0;
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

void tick() { //function to pulse floppy drives
  for (int i = 5; i < 8; i++) {
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
  unsigned long currentMicros = micros();
  if (currentState[i]) {
    if (i == 5) {
      SET(PORTA, 4); //floppy 1
      SET(PORTA, 6); //floppy 2
      SET(PORTC, 7); //floppy 3
    }
    if (i == 6) {
      SET(PORTA, 2); //floppy 1
      SET(PORTC, 3); //floppy 2
      SET(PORTC, 1); //floppy 3
    }
    if (i == 7) {
      SET(PORTD, 7); //floppy 1
      SET(PORTG, 1); //floppy 2
      SET(PORTL, 7); //floppy 3
    }
  }
  else {
    if (i == 5) {
      CLR(PORTA, 4); //floppy 1
      if (currentMicros - floppyEnvelope[i] >= 10000 && currentMicros - floppyEnvelope[i] <= 200000) { //envelope simulation
        CLR(PORTA, 6); //floppy 2
      }
      if (currentMicros - floppyEnvelope[i] >= 20000 && currentMicros - floppyEnvelope[i] <= 100000) { //envelope simulation
        CLR(PORTC, 7); //floppy 2
      }
    }
    if (i == 6) {
      CLR(PORTA, 2); //floppy 1
      if (currentMicros - floppyEnvelope[i] >= 10000 && currentMicros - floppyEnvelope[i] <= 200000) { //envelope simulation
        CLR(PORTC, 3); //floppy 2
      }
      if (currentMicros - floppyEnvelope[i] >= 20000 && currentMicros - floppyEnvelope[i] <= 100000) { //envelope simulation
        CLR(PORTC, 1); //floppy 3
      }
    }
    if (i == 7) {
      CLR(PORTD, 7); //floppy 1
      if (currentMicros - floppyEnvelope[i] >= 10000 && currentMicros - floppyEnvelope[i] <= 200000) { //envelope simulation
        CLR(PORTG, 1); //floppy 2
      }
      if (currentMicros - floppyEnvelope[i] >= 20000 && currentMicros - floppyEnvelope[i] <= 100000) { //envelope simulation
        CLR(PORTL, 7); //floppy 3
      }
    }
  }
  if (stepCount[i] == 2) {
    stepCount[i] = 0;
    floppyDir[i] = !floppyDir[i];
    if (floppyDir[i]) {
      if (i == 5) {
        SET(PORTA, 5); //floppy 1
        SET(PORTA, 7); //floppy 2
        SET(PORTC, 6); //floppy 3
      }
      if (i == 6) {
        SET(PORTA, 3); //floppy 1
        SET(PORTC, 2); //floppy 2
        SET(PORTC, 0); //floppy 3
      }
      if (i == 7) {
        SET(PORTG, 2); //floppy 1
        SET(PORTG, 0); //floppy 2
        SET(PORTL, 6); //floppy 3
      }
    }
    else {
      if (i == 5) {
        CLR(PORTA, 5); //floppy 1
        CLR(PORTA, 7); //floppy 2
        CLR(PORTC, 6); //floppy 3
      }
      if (i == 6) {
        CLR(PORTA, 3); //floppy 1
        CLR(PORTC, 2); //floppy 2
        CLR(PORTC, 0); //floppy 3
      }
      if (i == 7) {
        CLR(PORTG, 2); //floppy 1
        CLR(PORTG, 0); //floppy 2
        CLR(PORTL, 6); //floppy 3
      }
    }
  }
  currentState[i] = !currentState[i];
  stepCount[i]++;
}

void floppyDrum() {
  CLR(PORTL, 7);
  CLR(PORTG, 1);
  CLR(PORTD, 7);
  currentState[7] = !currentState[7];
  if (currentState[7]) {
    SET(PORTL, 6);
    SET(PORTG, 2);
    SET(PORTC, 0);
  }
  else {
    CLR(PORTL, 6);
    CLR(PORTG, 2);
    CLR(PORTC, 0);
  }
  prevDrum[4] = micros();
}

void myPitchBend(byte channel, int bend) {
  if (channel >= 5 && channel <= 7) {
    float bendF = bend;
    bendF /= 8192;
    bendF *= bendSens[channel];
    bendF /= 12;
    currentPeriod[channel] = originalPeriod[channel] * pow(2, bendF);
  }
}

void myControlChange(byte channel, byte firstByte, byte secondByte) {
  if (channel >= 5 && channel <= 7) {
    if (firstByte == 121) { //reset all controllers
      resetAll();
    }
    if (firstByte == 120 || firstByte == 123) { //mute all sounds
      mute();
    }
    if (firstByte == 101) {
      controlVal1 = secondByte;
    }
    if (firstByte == 100) {
      controlVal2 = secondByte;
    }
    if (firstByte == 6 && controlVal1 == 0 && controlVal2 == 0) { //pitch bend sensitivity
      if (secondByte > 0 && secondByte < 13) {
        bendSens[channel] = secondByte;
      }
      controlVal1 = -1;
      controlVal2 = -1;
    }
  }
}

void resetAll() {
  for (int i = 0; i < 8; i++) {
    bendSens[i] = 2;
    bendFactor[i] = 1;
    currentPeriod[i] = originalPeriod[i] = 0;
  }
  for (int i = 24; i < 54; i += 2) {
    digitalWrite(i, HIGH);
  }
}

void mute() {
  for (int i = 0; i < 8; i++) {
    currentPeriod[i] = originalPeriod[i] = 0;
  }
  for (int i = 24; i < 54; i += 2) {
    digitalWrite(i, HIGH);
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
          myPitchBend(offsetChannel, (midiBendVal - 8192) * -1);
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
          myControlChange(offsetChannel, midiControlNum, incomingByte);
        }
        midiAction = 0;
      }
    }
  }
}
