#include <digitalWriteFast.h>
#include <MIDI.h>
#include "pitches.h"

//Stepper Motors:
#define stepPin_M1 22
#define stepPin_M2 24
#define stepPin_M3 44
#define stepPin_M4 46
#define dirPin_M1 23
#define dirPin_M2 25
#define dirPin_M3 45
#define dirPin_M4 47
#define enable1 8
#define enable2 9
#define enable3 6
#define enable4 7

//Floppy Drives channel_floppy#:
#define floppyStep5_1 26
#define floppyStep5_2 28
#define floppyStep5_3 30
#define floppyDir5_1 27
#define floppyDir5_2 29
#define floppyDir5_3 31

#define floppyStep6_1 32
#define floppyStep6_2 34
#define floppyStep6_3 36
#define floppyDir6_1 33
#define floppyDir6_2 35
#define floppyDir6_3 37

#define floppyStep7_1 38
#define floppyStep7_2 40
#define floppyStep7_3 42
#define floppyDir7_1 39
#define floppyDir7_2 41
#define floppyDir7_3 43

//Hard Drives:
#define bassDrum 2
#define snareDrum 3
#define highDrum 4
#define highDrum2 5

//MIDI Configuration:
bool floppyDrum = true; //enable floppy drum on floppy channel 7
#define maxChannel 7 //excluding channel 10
#define stepperChannels 4 //stepper channels
#define floppyChannels 3 //floppy channels
#define hddChannels 4 //hdd channels
#define attack 10000 //initial attack for floppy envelope in us
#define attackOffset 10000 //offset added to initial in us
#define sustain 200000 //initial sustain for floppy envelope in us
#define sustainOffset 50000 //offset subtracted from initial in us

//Variable definitions:
int pitchCurrent[] = { -1, -1, -1, -1, -1, -1, -1, -1};
float bendFactor[] = {1, 1, 1, 1, 1, 1, 1, 1};
byte origPitch[] = { 0, 0, 0, 0, 0, 0, 0, 0};
byte bendSens[] = { 2, 2, 2, 2, 2, 2, 2, 2};
bool motorDirections[] = {0, 0, 0, 0, 0, 0, 0, 0};
bool motorStallMode[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long prevMicros[] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned long floppyEnvelope[] = {0, 0, 0, 0, 0, 0, 0, 0}; //prevMicros, but doesnt update each step

unsigned long prevDrum[] = { 0, 0, 0, 0, 0};
uint16_t drumDuration[] = { 0, 0, 0, 0, 0};

bool floppyDir[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

byte controlValue1 = -1;
byte controlValue2 = -1;
MIDI_CREATE_DEFAULT_INSTANCE(); //use default MIDI settings

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = 22; i < 54; i++) {
    pinMode(i, OUTPUT);
  }
  resetAll();

  MIDI.begin(MIDI_CHANNEL_OMNI); //listen to all MIDI channels
  MIDI.setHandleNoteOn(handleNoteOn); //execute function when note on message is received
  MIDI.setHandleNoteOff(handleNoteOff); //execute function when note off message is received
  MIDI.setHandlePitchBend(myPitchBend); //execute function when pitch bend message is received
  MIDI.setHandleControlChange(myControlChange); //execute function when control change message is received
  Serial.begin(115200); //Allows for serial MIDI communication. Comment this line when using mocaLUFA.
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{
  MIDI.read();
  unsigned long currentMicros = micros();
  singleStep(1, currentMicros);
  singleStep(2, currentMicros);
  singleStep(3, currentMicros);
  singleStep(4, currentMicros);
  floppySingleStep(5, currentMicros);
  floppySingleStep(6, currentMicros);
  floppySingleStep(7, currentMicros);
  processDrum(currentMicros);
}

void processDrum(unsigned long currentMicros) {
  if (drumDuration[0] > 0 && (currentMicros - prevDrum[0]) >= drumDuration[0]) { //bass drum
    drumDuration[0] = 0;
    digitalWriteFast(bassDrum, LOW);
  }
  else if (drumDuration[1] > 0 && (currentMicros - prevDrum[1]) >= drumDuration[1]) { //snare drum
    drumDuration[1] = 0;
    digitalWriteFast(snareDrum, LOW);
  }
  else if (drumDuration[2] > 0 && (currentMicros - prevDrum[2]) >= drumDuration[2]) { //high drum
    drumDuration[2] = 0;
    digitalWriteFast(highDrum, LOW);
  }
  else if (drumDuration[3] > 0 && (currentMicros - prevDrum[3]) >= drumDuration[3]) { //high drum 2
    drumDuration[3] = 0;
    digitalWriteFast(highDrum2, LOW);
  }
  else if (drumDuration[4] > 0 && (currentMicros - prevDrum[4]) >= drumDuration[4]) { //floppy Drum
    drumDuration[4] = 0;
    digitalWriteFast(floppyStep7_1, HIGH);
    digitalWriteFast(floppyStep7_2, HIGH);
    digitalWriteFast(floppyStep7_3, HIGH);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handleNoteOn(byte channel, byte pitch, byte velocity) //MIDI Note ON Command
{
  if ((channel > 0 && channel <= 10) && velocity > 0)
  {
    unsigned long currentMicros = micros();
    if (channel <= 4) //Stepper Motors
    {
      if (channel == 1) {
        digitalWriteFast(enable1, LOW);
      }
      else if (channel == 2) {
        digitalWriteFast(enable2, LOW);
      }
      else if (channel == 3) {
        digitalWriteFast(enable3, LOW);
      }
      else if (channel == 4) {
        digitalWriteFast(enable4, LOW);
      }
      if (pitch == 50 || pitch == 51 || pitch == 52) {
        motorStallMode[channel] = HIGH;//The motor now intentionally "stalls" itself flag.
      }
      else {
        motorStallMode[channel] = LOW;//No longer stalling itself.
      }
      if (pitch > 81) {
        pitchCurrent[channel] = pitchVals[pitch - 12];
      }
      else {
        pitchCurrent[channel] = pitchVals[pitch];
      }
      origPitch[channel] = pitch;
      prevMicros[channel] = currentMicros;
    }
    else if (channel >= 5 && channel <= 7) { //Floppy Drives
      if (floppyDrum && channel == 7) {
        //ignore message
      }
      else {
        pitchCurrent[channel] = pitchVals[pitch];
        floppyEnvelope[channel] = currentMicros;
        prevMicros[channel] = currentMicros;
        origPitch[channel] = pitch;
      }
    }
    else if (channel == 10) {
      if (pitch == 35 || pitch == 36) { //bass drum
        if (velocity > 119) {
          drumDuration[0] = map(velocity, 120, 127, 2000, 2350);
        }
        else {
          drumDuration[0] = map(velocity, 1, 119, 1750, 2000);
        }
        digitalWriteFast(bassDrum, HIGH);
        prevDrum[0] = currentMicros;
      }
      else if (pitch == 38 || pitch == 40) { //snare drum
        if (velocity > 119) {
          drumDuration[1] = map(velocity, 120, 127, 2250, 3000);
        }
        else {
          drumDuration[1] = map(velocity, 1, 119, 1000, 1500);
        }
        digitalWriteFast(snareDrum, HIGH);
        prevDrum[1] = currentMicros;
      }
      else if (pitch == 39 || pitch == 48 || pitch == 50 || pitch == 54 || pitch == 46 || pitch == 51 || pitch == 59 || pitch == 49 || pitch == 57) { //high drum 2
        drumDuration[3] = 1500;
        digitalWriteFast(highDrum2, HIGH);
        prevDrum[3] = currentMicros;
      }
      else { //high drum
        if (floppyDrum) {
          digitalWriteFast(floppyStep7_1, LOW);
          digitalWriteFast(floppyStep7_2, LOW);
          digitalWriteFast(floppyStep7_3, LOW);
          floppyDir[6] = !floppyDir[6];
          if (floppyDir[6]) {
            digitalWriteFast(floppyDir7_1, HIGH);
            digitalWriteFast(floppyDir7_2, HIGH);
            digitalWriteFast(floppyDir7_3, HIGH);
          }
          else {
            digitalWriteFast(floppyDir7_1, LOW);
            digitalWriteFast(floppyDir7_2, LOW);
            digitalWriteFast(floppyDir7_3, LOW);
          }
          prevDrum[4] = currentMicros;
          drumDuration[4] = 10000;
        }
        else {
          if (velocity > 119) {
            drumDuration[2] = 1500;
          }
          else {
            drumDuration[2] = 500;
          }
          digitalWriteFast(highDrum, HIGH);
          prevDrum[2] = currentMicros;
        }
      }
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) //MIDI Note OFF Command
{
  if (channel > 0 && channel <= maxChannel)
  {
    if (origPitch[channel] == pitch) {
      pitchCurrent[channel] = -1;//Reset to -1
      if (channel == 1)
      {
        digitalWriteFast(enable1, HIGH);
      }

      else if (channel == 2)
      {
        digitalWriteFast(enable2, HIGH);
      }

      else if (channel == 3)
      {
        digitalWriteFast(enable3, HIGH);
      }

      else if (channel == 4)
      {
        digitalWriteFast(enable4, HIGH);
      }

      else if (channel == 5) {
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
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void singleStep(byte motorNum, unsigned long currentMicros)
{
  if (pitchCurrent[motorNum] != -1 && ((currentMicros - prevMicros[motorNum]) <= pitchCurrent[motorNum]))
  {
    if (motorNum == 1)
    {
      digitalWriteFast(stepPin_M1, LOW);
      digitalWriteFast(stepPin_M1, HIGH);
      if (motorStallMode[1]) //If we're in stalled motor mode...
      {
        motorDirections[1] = !motorDirections[1]; //Flip the stored direction of the motor.
        if (motorDirections[1]) {
          digitalWriteFast(dirPin_M1, HIGH);
        }
        else {
          digitalWriteFast(dirPin_M1, LOW);
        }
      }
    }
    if (motorNum == 2)
    {
      digitalWriteFast(stepPin_M2, LOW);
      digitalWriteFast(stepPin_M2, HIGH);
      if (motorStallMode[2]) //If we're in stalled motor mode...
      {
        motorDirections[2] = !motorDirections[2]; //Flip the stored direction of the motor.
        if (motorDirections[2]) {
          digitalWriteFast(dirPin_M2, HIGH);
        }
        else {
          digitalWriteFast(dirPin_M2, LOW);
        }
      }
    }
    if (motorNum == 3)
    {
      digitalWriteFast(stepPin_M3, LOW);
      digitalWriteFast(stepPin_M3, HIGH);
      if (motorStallMode[3]) //If we're in stalled motor mode...
      {
        motorDirections[3] = !motorDirections[3]; //Flip the stored direction of the motor.
        if (motorDirections[3]) {
          digitalWriteFast(dirPin_M3, HIGH);
        }
        else {
          digitalWriteFast(dirPin_M3, LOW);
        }
      }
    }
    if (motorNum == 4)
    {
      digitalWriteFast(stepPin_M4, LOW);
      digitalWriteFast(stepPin_M4, HIGH);
      if (motorStallMode[4]) //If we're in stalled motor mode...
      {
        motorDirections[4] = !motorDirections[4]; //Flip the stored direction of the motor.
        if (motorDirections[4]) {
          digitalWriteFast(dirPin_M4, HIGH);
        }
        else {
          digitalWriteFast(dirPin_M4, LOW);
        }
      }
    }
    prevMicros[motorNum] += pitchCurrent[motorNum] * bendFactor[motorNum]; //Keeps track of the last time a tick occurred for the next time.
  }
}

void floppySingleStep(byte channel, unsigned long currentMicros) {
  if (pitchCurrent[channel] != -1 && ((currentMicros - prevMicros[channel]) <= pitchCurrent[channel])) {
    if (channel == 5) {
      floppy(0);
      if (currentMicros - floppyEnvelope[channel] >= attack && currentMicros - floppyEnvelope[channel] <= sustain) {
        floppy(1);
        if (currentMicros - floppyEnvelope[channel] >= attack + attackOffset && currentMicros - floppyEnvelope[channel] <= sustain - sustainOffset) {
          floppy(2);
        }
        else {
          digitalWriteFast(floppyStep5_3, HIGH);
        }
      } else {
        digitalWriteFast(floppyStep5_2, HIGH);
      }
    }
    else if (channel == 6) {
      floppy(3);
      if (currentMicros - floppyEnvelope[channel] >= attack && currentMicros - floppyEnvelope[channel] <= sustain) {
        floppy(4);
        if (currentMicros - floppyEnvelope[channel] >= attack + attackOffset && currentMicros - floppyEnvelope[channel] <= sustain - sustainOffset) {
          floppy(5);
        }
        else {
          digitalWriteFast(floppyStep6_3, HIGH);
        }
      } else {
        digitalWriteFast(floppyStep6_2, HIGH);
      }
    }
    else if (channel == 7) {
      floppy(6);
      if (currentMicros - floppyEnvelope[channel] >= attack && currentMicros - floppyEnvelope[channel] <= sustain) {
        floppy(7);
        if (currentMicros - floppyEnvelope[channel] >= attack + attackOffset && currentMicros - floppyEnvelope[channel] <= sustain - sustainOffset) {
          floppy(8);
        }
        else {
          digitalWriteFast(floppyStep7_3, HIGH);
        }
      } else {
        digitalWriteFast(floppyStep7_2, HIGH);
      }
    }
    prevMicros[channel] += pitchCurrent[channel] * bendFactor[channel];
  }
}


void floppy(byte index) {
  if (index == 0) { //channel 5
    digitalWriteFast(floppyStep5_1, HIGH);
    digitalWriteFast(floppyStep5_1, LOW);
    floppyDir[0] = !floppyDir[0];
    if (floppyDir[0]) {
      digitalWriteFast(floppyDir5_1, HIGH);
    }
    else {
      digitalWriteFast(floppyDir5_1, LOW);
    }
  }
  else if (index == 1) { //channel 5
    digitalWriteFast(floppyStep5_2, HIGH);
    digitalWriteFast(floppyStep5_2, LOW);
    floppyDir[1] = !floppyDir[1];
    if (floppyDir[1]) {
      digitalWriteFast(floppyDir5_2, HIGH);
    }
    else {
      digitalWriteFast(floppyDir5_2, LOW);
    }
  }
  else if (index == 2) { //channel 5
    digitalWriteFast(floppyStep5_3, HIGH);
    digitalWriteFast(floppyStep5_3, LOW);
    floppyDir[2] = !floppyDir[2];
    if (floppyDir[2]) {
      digitalWriteFast(floppyDir5_3, HIGH);
    }
    else {
      digitalWriteFast(floppyDir5_3, LOW);
    }
  }
  else if (index == 3) { //channel 6
    digitalWriteFast(floppyStep6_1, HIGH);
    digitalWriteFast(floppyStep6_1, LOW);
    floppyDir[3] = !floppyDir[3];
    if (floppyDir[3]) {
      digitalWriteFast(floppyDir6_1, HIGH);
    }
    else {
      digitalWriteFast(floppyDir6_1, LOW);
    }
  }
  else if (index == 4) { //channel 6
    digitalWriteFast(floppyStep6_2, HIGH);
    digitalWriteFast(floppyStep6_2, LOW);
    floppyDir[4] = !floppyDir[4];
    if (floppyDir[4]) {
      digitalWriteFast(floppyDir6_2, HIGH);
    }
    else {
      digitalWriteFast(floppyDir6_2, LOW);
    }
  }
  else if (index == 5) { //channel 6
    digitalWriteFast(floppyStep6_3, HIGH);
    digitalWriteFast(floppyStep6_3, LOW);
    floppyDir[5] = !floppyDir[5];
    if (floppyDir[5]) {
      digitalWriteFast(floppyDir6_3, HIGH);
    }
    else {
      digitalWriteFast(floppyDir6_3, LOW);
    }
  }
  else if (index == 6) { //channel 7
    digitalWriteFast(floppyStep7_1, HIGH);
    digitalWriteFast(floppyStep7_1, LOW);
    floppyDir[6] = !floppyDir[6];
    if (floppyDir[6]) {
      digitalWriteFast(floppyDir7_1, HIGH);
    }
    else {
      digitalWriteFast(floppyDir7_1, LOW);
    }
  }
  else if (index == 7) { //channel 7
    digitalWriteFast(floppyStep7_2, HIGH);
    digitalWriteFast(floppyStep7_2, LOW);
    floppyDir[7] = !floppyDir[7];
    if (floppyDir[7]) {
      digitalWriteFast(floppyDir7_2, HIGH);
    }
    else {
      digitalWriteFast(floppyDir7_2, LOW);
    }
  }
  else if (index == 8) { //channel 7
    digitalWriteFast(floppyStep7_3, HIGH);
    digitalWriteFast(floppyStep7_3, LOW);
    floppyDir[8] = !floppyDir[8];
    if (floppyDir[8]) {
      digitalWriteFast(floppyDir7_3, HIGH);
    }
    else {
      digitalWriteFast(floppyDir7_3, LOW);
    }
  }
}

void myPitchBend(byte channel, int val) {
  if (channel > 0 && channel <= maxChannel) {
    if (val == 0) {
      bendFactor[channel] = 1;
    }
    else {
      int bendMap = map(val * -1, -8192, 8191, 0, 126);
      byte bendRange = bendSens[channel];
      if (bendRange == 2) {
        bendFactor[channel] = bendVal2[bendMap];
      }
      else if (bendRange == 12) {
        bendFactor[channel] = bendVal12[bendMap];
      }
      else if (bendRange == 11) {
        bendFactor[channel] = bendVal11[bendMap];
      }
      else if (bendRange == 10) {
        bendFactor[channel] = bendVal10[bendMap];
      }
      else if (bendRange == 9) {
        bendFactor[channel] = bendVal9[bendMap];
      }
      else if (bendRange == 8) {
        bendFactor[channel] = bendVal8[bendMap];
      }
      else if (bendRange == 7) {
        bendFactor[channel] = bendVal7[bendMap];
      }
      else if (bendRange == 6) {
        bendFactor[channel] = bendVal6[bendMap];
      }
      else if (bendRange == 5) {
        bendFactor[channel] = bendVal5[bendMap];
      }
      else if (bendRange == 4) {
        bendFactor[channel] = bendVal4[bendMap];
      }
      else if (bendRange == 3) {
        bendFactor[channel] = bendVal3[bendMap];
      }
      else if (bendRange == 1) {
        bendFactor[channel] = bendVal1[bendMap];
      }
    }
  }
}

void myControlChange(byte channel, byte firstByte, byte secondByte) {
  if (channel > 0 && channel <= maxChannel) {
    if (firstByte == 121) { //reset all controllers
      resetAll();
    }
    if (firstByte == 120 || firstByte == 123) { //mute all sounds
      mute();
    }
    if (firstByte == 101) {
      controlValue1 = secondByte;
    }
    if (firstByte == 100) {
      controlValue2 = secondByte;
    }
    if (firstByte == 6 && controlValue1 == 0 && controlValue2 == 0) { //pitch bend sensitivity
      if (secondByte > 0 && secondByte < 13) {
        bendSens[channel] = secondByte;
      }
      controlValue1 = -1;
      controlValue2 = -1;
    }
  }
}

void resetAll() {
  for (int i = 0; i <= maxChannel; i++) {
    bendSens[i] = 2;
    bendFactor[i] = 1;
  }
  for (int i = 26; i < 54; i += 2) {
    digitalWriteFast(i, HIGH);
  }
  for (int i = 6; i < 13; i++) {
    digitalWriteFast(i, HIGH);
  }
  digitalWriteFast(dirPin_M1, LOW);
  digitalWriteFast(dirPin_M2, LOW);
  digitalWriteFast(dirPin_M3, LOW);
  digitalWriteFast(dirPin_M4, LOW);
}

void mute() {
  for (int i = 0; i <= maxChannel; i++) {
    pitchCurrent[i] = -1;
  }
  for (int i = 26; i < 54; i += 2) {
    digitalWriteFast(i, HIGH);
  }
}
