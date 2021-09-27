#include <MIDI.h>
#include <digitalWriteFast.h>
#include "pitches.h"

//Stepper Motors:
#define stepPin_M1 26
#define stepPin_M2 27
#define dirPin_M1 24
#define dirPin_M2 25

//Floppy Drives:
#define floppyStep 8
#define floppyDirPin 9
#define floppyStep2 6
#define floppyDirPin2 7
#define floppyStep3 4
#define floppyDirPin3 5

//Hard Drives:
#define bassDrum 10
#define snareDrum 11
#define highDrum 12
#define highDrum2 13

//MIDI Configuration:
#define maxChannel 7 //excluding channel 10
#define stepperChannels 2 //stepper channels
#define floppyChannels 3 //floppy channels
#define hddChannels 5 //hdd channels
bool floppyDrum = false; //enable or disable floppy drum. Ensure nothing is playing on channel 7 when enabled!!!

//Variable definitions:

int pitchTarget[maxChannel + 1];
int pitchCurrent[maxChannel + 1];
int acceleration[maxChannel + 1];
byte bendSens[maxChannel + 1];
byte origPitch[maxChannel + 1];
float bendFactor[maxChannel + 1];
unsigned long prevMicros[maxChannel + 1];
bool motorDirections[maxChannel + 1];
bool motorStallMode[maxChannel + 1];

unsigned long prevDrum[hddChannels];
uint16_t drumDuration[hddChannels];

bool floppyDir[maxChannel + 1];

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
  for (int i = 22; i < 36; i++) {
    pinMode(i, OUTPUT);
  }

  digitalWriteFast(dirPin_M1, LOW);
  digitalWriteFast(dirPin_M2, LOW);
  digitalWriteFast(stepPin_M1, HIGH);
  digitalWriteFast(stepPin_M2, HIGH);
  digitalWriteFast(floppyStep, HIGH);
  digitalWriteFast(floppyStep2, HIGH);
  digitalWriteFast(floppyStep3, HIGH);
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
  //Read midi messages and step each stepper at the correct speed (called frequently)
  MIDI.read();
  singleStep(1);
  singleStep(2);
  floppySingleStep(5);
  floppySingleStep(6);
  floppySingleStep(7);
  processDrum();
}

void processDrum() {
  if (drumDuration[0] > 0 && (micros() - prevDrum[0]) >= drumDuration[0]) { //bass drum
    drumDuration[0] = 0;
    digitalWriteFast(bassDrum, LOW);
  }
  else if (drumDuration[1] > 0 && (micros() - prevDrum[1]) >= drumDuration[1]) { //snare drum
    drumDuration[1] = 0;
    digitalWriteFast(snareDrum, LOW);
  }
  else if (drumDuration[2] > 0 && (micros() - prevDrum[2]) >= drumDuration[2]) { //high drum
    drumDuration[2] = 0;
    digitalWriteFast(highDrum, LOW);
  }
  else if (drumDuration[3] > 0 && (micros() - prevDrum[3]) >= drumDuration[3]) { //high drum 2
    drumDuration[3] = 0;
    digitalWriteFast(highDrum2, LOW);
  }
  else if (drumDuration[4] > 0 && (micros() - prevDrum[4] >= drumDuration[4])) { //floppy drum
    drumDuration[4] = 0;
    digitalWriteFast(floppyStep3, HIGH);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handleNoteOn(byte channel, byte pitch, byte velocity) //MIDI Note ON Command
{
  if ((channel > 0 && channel <= maxChannel) && velocity > 0)
  {
    if (channel <= stepperChannels) //Stepper Motors
    {
      if (pitch == 51 || pitch == 52 || pitch == 53 || pitch == 54 || pitch == 55 || pitch == 56 || pitch == 57 || pitch == 58) {
        motorStallMode[channel] = HIGH;//The motor now intentionally "stalls" itself flag.
      }
      else {
        motorStallMode[channel] = LOW;//No longer stalling itself.
      }
      if (pitch > 83) {
        pitchTarget[channel] = pitchVals[pitch]; //Save the pitch to a global target array.
        pitchCurrent[channel] = pitchVals[70]; //Force an acceleration from the highest pitch guaranteed to lock the rotor in first step.
        acceleration[channel] = 70;
      }
      else {
        pitchTarget[channel] = pitchVals[pitch]; //Save the pitch to a global target array.
        pitchCurrent[channel] = pitchVals[pitch]; //No acceleration!
      }
      origPitch[channel] = pitch;
    }
    else if (channel > stepperChannels) { //Floppy Drives
      origPitch[channel] = pitch;
      if (pitch > 67) pitch -= 12;
      else if (pitch < 38) pitch += 12;
      pitchCurrent[channel] = pitchVals[pitch];
    }
    prevMicros[channel] = micros();
  }
  if (channel == 10 && velocity > 0) { //drum
    if (pitch == 35 || pitch == 36) { //bass drum
      if (velocity > 119) {
        drumDuration[0] = map(velocity, 120, 127, 2000, 2500);
      }
      else {
        drumDuration[0] = map(velocity, 1, 119, 1750, 2000);
      }
      digitalWriteFast(bassDrum, HIGH);
      prevDrum[0] = micros();
    }
    else if (pitch == 38 || pitch == 40) { //snare drum
      if (velocity > 119) {
        drumDuration[1] = map(velocity, 120, 127, 2250, 3000);
      }
      else {
        drumDuration[1] = map(velocity, 1, 119, 1000, 1500);
      }
      digitalWriteFast(snareDrum, HIGH);
      prevDrum[1] = micros();
    }
    else if (pitch == 39 || pitch == 54 || pitch == 81) { //high drum 2
      drumDuration[3] = 1500;
      digitalWriteFast(highDrum2, HIGH);
      prevDrum[3] = micros();
    }
    else if (floppyDrum) { //floppy drum
      drumDuration[4] = 10000;
      floppyDir[7] = !floppyDir[7];
      if(floppyDir[7]) {
        digitalWriteFast(floppyDirPin3, HIGH);
      }
      else {
        digitalWriteFast(floppyDirPin3, LOW);
      }
      digitalWriteFast(floppyStep3, LOW);
      prevDrum[4] = micros();
    }
    else { //high drum
      drumDuration[2] = 1250;
      digitalWriteFast(highDrum, HIGH);
      prevDrum[2] = micros();
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) //MIDI Note OFF Command
{
  if (channel > 0 && channel <= maxChannel)
  {
    if(origPitch[channel] == pitch) {
      pitchCurrent[channel] = -1;//Reset to -1
      origPitch[channel] = -1;
    }
    if (channel == 1)
    {
      digitalWriteFast(stepPin_M1, HIGH);
    }

    else if (channel == 2)
    {
      digitalWriteFast(stepPin_M2, HIGH);
    }

    else if (channel == 5) {
      digitalWriteFast(floppyStep, HIGH);
    }

    else if (channel == 6) {
      digitalWriteFast(floppyStep2, HIGH);
    }

    else if (channel == 7) {
      digitalWriteFast(floppyStep3, HIGH);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void singleStep(byte motorNum)
{
  if (pitchCurrent[motorNum] != -1 && ((micros() - prevMicros[motorNum]) <= pitchCurrent[motorNum]))
  {
    if (motorNum == 1)
    {
      digitalWriteFast(stepPin_M1, HIGH);
      digitalWriteFast(stepPin_M1, LOW);
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
    else if (motorNum == 2)
    {
      digitalWriteFast(stepPin_M2, HIGH);
      digitalWriteFast(stepPin_M2, LOW);
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
    prevMicros[motorNum] += pitchCurrent[motorNum] * bendFactor[motorNum]; //Keeps track of the last time a tick occurred for the next time.
    if (pitchCurrent[motorNum] > pitchTarget[motorNum]) //Not yet on target?
    {
      acceleration[motorNum]++;
      pitchCurrent[motorNum] = pitchVals[acceleration[motorNum]]; //Move the current closer to the target (acceleration control)!
    }
  }
}

void floppySingleStep(byte channel) {
  if (pitchCurrent[channel] != -1 && ((micros() - prevMicros[channel]) <= pitchCurrent[channel])) {
    floppyDir[channel] = !floppyDir[channel];
    if (channel == 5) {
      digitalWriteFast(floppyStep, HIGH);
      digitalWriteFast(floppyStep, LOW);
      if (floppyDir[5]) {
        digitalWriteFast(floppyDirPin, HIGH);
      }
      else {
        digitalWriteFast(floppyDirPin, LOW);
      }
    }
    else if (channel == 6) {
      digitalWriteFast(floppyStep2, HIGH);
      digitalWriteFast(floppyStep2, LOW);
      if (floppyDir[6]) {
        digitalWriteFast(floppyDirPin2, HIGH);
      }
      else {
        digitalWriteFast(floppyDirPin2, LOW);
      }
    }
    else if (channel == 7) {
      digitalWriteFast(floppyStep3, HIGH);
      digitalWriteFast(floppyStep3, LOW);
      if (floppyDir[7]) {
        digitalWriteFast(floppyDirPin3, HIGH);
      }
      else {
        digitalWriteFast(floppyDirPin3, LOW);
      }
    }
    prevMicros[channel] += pitchCurrent[channel] * bendFactor[channel];
  }
}

void myPitchBend(byte channel, int val) {
  if (channel > 0 && channel <= maxChannel) {
    int bendMap = map(val*-1,-8192,8191,0,126);
    byte bendRange = bendSens[channel];
    if(bendRange == 2) {
      bendFactor[channel] = bendVal2[bendMap];
    }
    else if(bendRange == 7) {
      bendFactor[channel] = bendVal7[bendMap];
    }
  }
}

void myControlChange(byte channel, byte firstByte, byte secondByte) {
  if (firstByte == 121) { //reset all controllers
    resetAll();
  }
  if (firstByte == 120) { //mute all sounds
    mute();
  }
  if (firstByte == 101) {
    controlValue1 = secondByte;
  }
  if (firstByte == 100) {
    controlValue2 = secondByte;
  }
  if (firstByte == 6 && controlValue1 == 0 && controlValue2 == 0 && (channel > 0 && channel <= maxChannel)) { //pitch bend sensitivity
    bendSens[channel] = secondByte;
    controlValue1 = -1;
    controlValue2 = -1;
  }
}

void resetAll() {
  for (int i = 0; i < maxChannel + 1; i++) {
    prevMicros[i] = 0;
    motorDirections[i] = 0;
    motorStallMode[i] = 0;
    bendSens[i] = 2;
    bendFactor[i] = 1;
    floppyDir[i] = 1;
    origPitch[i] = 0;
  }
  for (int i = 0; i < hddChannels + 1; i++) {
    prevDrum[i] = 0;
    drumDuration[i] = 0;
  }
  controlValue1 = -1;
  controlValue2 = -1;
  mute();
}

void mute() {
  for (int i = 0; i < maxChannel + 1; i++) {
    pitchCurrent[i] = -1;
  }
}
