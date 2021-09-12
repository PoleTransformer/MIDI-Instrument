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

//Hard Drives:
#define bassDrum 10
#define snareDrum 11
#define highDrum 12
#define highDrum2 13

//MIDI Configuration:
#define maxChannel 6 //excluding channel 10
#define stepperChannels 2 //stepper channels
#define floppyChannels 2 //floppy channels
#define hddChannels 4 //hdd channels
#define maxFloppy 70 //floppy drive max steps

//Variable definitions:

int pitchTarget[maxChannel + 1];
int pitchCurrent[maxChannel + 1];
byte acceleration[maxChannel + 1];
byte initialPitch[maxChannel + 1];
byte bendSens[maxChannel + 1];
float bendFactor[maxChannel + 1];
unsigned long prevMicros[maxChannel + 1];
bool motorDirections[maxChannel + 1];
bool motorStallMode[maxChannel + 1];

unsigned long prevDrum[hddChannels];
uint16_t drumDuration[hddChannels];

bool floppyDir[maxChannel + 1];
byte floppyCount[maxChannel + 1];

byte controlValue1 = -1;
byte controlValue2 = -1;
MIDI_CREATE_DEFAULT_INSTANCE(); //use default MIDI settings

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  pinMode(stepPin_M1, OUTPUT);
  pinMode(stepPin_M2, OUTPUT);
  pinMode(dirPin_M1, OUTPUT);
  pinMode(dirPin_M2, OUTPUT);

  pinMode(floppyStep, OUTPUT);
  pinMode(floppyDirPin, OUTPUT);
  pinMode(floppyStep2, OUTPUT);
  pinMode(floppyDirPin2, OUTPUT);
  
  pinMode(bassDrum, OUTPUT);
  pinMode(snareDrum, OUTPUT);
  pinMode(highDrum, OUTPUT);
  pinMode(highDrum2, OUTPUT);

  digitalWriteFast(dirPin_M1, LOW);
  digitalWriteFast(dirPin_M2, LOW);
  digitalWriteFast(stepPin_M1, HIGH);
  digitalWriteFast(stepPin_M2, HIGH);
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
  processDrum();
}

void processDrum() {
  if (drumDuration[0] != -1 && (micros() - prevDrum[0]) >= drumDuration[0]) { //bass drum
    drumDuration[0] = -1;
    digitalWriteFast(bassDrum, LOW);
  }
  if (drumDuration[1] != -1 && (micros() - prevDrum[1]) >= drumDuration[1]) { //snare drum
    drumDuration[1] = -1;
    digitalWriteFast(snareDrum, LOW);
  }
  if (drumDuration[2] != -1 && (micros() - prevDrum[2]) >= drumDuration[2]) { //high drum
    drumDuration[2] = -1;
    digitalWriteFast(highDrum, LOW);
  }
  if (drumDuration[3] != -1 && (micros() - prevDrum[3]) >= drumDuration[3]) { //high drum
    drumDuration[3] = -1;
    digitalWriteFast(highDrum2, LOW);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handleNoteOn(byte channel, byte pitch, byte velocity) //MIDI Note ON Command
{
  if ((channel > 0 && channel <= maxChannel) && velocity > 0)
  {
    if (channel <= stepperChannels && (pitch == 51 || pitch == 52 || pitch == 53 || pitch == 54 || pitch == 55 || pitch == 56 || pitch == 57 || pitch == 58)) //If it's a resonance note when rotating....
    {
      motorStallMode[channel] = HIGH;//The motor now intentionally "stalls" itself flag.
    }
    else
    {
      motorStallMode[channel] = LOW;//No longer stalling itself.
    }

    if (channel <= stepperChannels && pitch > 83) //needs acceleration control over this point. It's not guaranteed to start.
    {
      pitchTarget[channel] = pitchVals[pitch]; //Save the pitch to a global target array.
      pitchCurrent[channel] = pitchVals[70]; //Force an acceleration from the highest pitch guaranteed to lock the rotor in first step.
      acceleration[channel] = 70;
    }
    else
    {
      pitchTarget[channel] = pitchVals[pitch]; //Save the pitch to a global target array.
      pitchCurrent[channel] = pitchVals[pitch]; //No acceleration!
    }
    
    initialPitch[channel] = pitch;
    prevMicros[channel] = micros();
  }
  if (channel == 10 && velocity > 0) { //drum
    if (pitch == 35 || pitch == 36) { //bass drum
      if (velocity > 63) {
        drumDuration[0] = map(velocity, 64, 127, 1500, 2500);
      }
      else {
        drumDuration[0] = map(velocity, 1, 63, 1, 1500);
      }
      digitalWriteFast(bassDrum, HIGH);
      prevDrum[0] = micros();
    }
    else if (pitch == 38 || pitch == 40 || pitch == 39) { //snare drum
      if (velocity > 63) {
        drumDuration[1] = map(velocity, 64, 127, 2000, 3000);
      }
      else {
        drumDuration[1] = map(velocity, 1, 63, 1, 1500);
      }
      digitalWriteFast(snareDrum, HIGH);
      prevDrum[1] = micros();
    }
    else if (pitch > 44) { //high drum 2
      drumDuration[3] = 1750;
      digitalWriteFast(highDrum2, HIGH);
      prevDrum[3] = micros();
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
    pitchTarget[channel] = -1;//set motor target pitch to -1
    pitchCurrent[channel] = -1;//Reset to -1
    initialPitch[channel] = -1;//MIDI note reset
    acceleration[channel] = -1;//Accel Reset
    if (channel == 1)
    {
      digitalWriteFast(stepPin_M1, HIGH);
    }

    if (channel == 2)
    {
      digitalWriteFast(stepPin_M2, HIGH);
    }

    if (channel == 5) {
      digitalWriteFast(floppyStep, HIGH);
    }

    if (channel == 6) {
      digitalWriteFast(floppyStep2, HIGH);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void singleStep(byte motorNum)
{
  if (initialPitch[motorNum] != -1 && ((micros() - prevMicros[motorNum]) <= pitchCurrent[motorNum]))
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
    if (bendFactor[motorNum] != 1) {
      processBend(motorNum);
    }
    prevMicros[motorNum] = prevMicros[motorNum] + pitchCurrent[motorNum];//Keeps track of the last time a tick occurred for the next time.
    if (pitchCurrent[motorNum] > pitchTarget[motorNum]) //Not yet on target?
    {
      acceleration[motorNum]++;
      pitchCurrent[motorNum] = pitchVals[acceleration[motorNum]]; //Move the current closer to the target (acceleration control)!
    }
  }
}

void floppySingleStep(int channel) {
  if (initialPitch[channel] != -1 && ((micros() - prevMicros[channel]) <= pitchCurrent[channel])) {
    if (channel == 5) {
      digitalWriteFast(floppyStep, HIGH);
      digitalWriteFast(floppyStep, LOW);
    }
    if (channel == 6) {
      digitalWriteFast(floppyStep2, HIGH);
      digitalWriteFast(floppyStep2, LOW);
    }
    floppyCount[channel]++;
    if (floppyCount[channel] >= maxFloppy) {
      floppyDir[channel] = !floppyDir[channel];
      if (floppyDir[5]) {
        digitalWriteFast(floppyDirPin, HIGH);
      }
      else {
        digitalWriteFast(floppyDirPin, LOW);
      }
      if (floppyDir[6]) {
        digitalWriteFast(floppyDirPin2, HIGH);
      }
      else {
        digitalWriteFast(floppyDirPin2, LOW);
      }
      floppyCount[channel] = 0;
    }
    if (bendFactor[channel] != 1) {
      processBend(channel);
    }
    prevMicros[channel] = prevMicros[channel] + pitchCurrent[channel];
  }
}

void myPitchBend(byte channel, int val) {
  if (channel > 0 && channel <= maxChannel) {
    if (val == 0) {
      pitchCurrent[channel] = pitchVals[initialPitch[channel]];
      pitchTarget[channel] = pitchCurrent[channel];
      bendFactor[channel] = 1;
      return;
    }
    val = val * -1;
    int bendRange = bendSens[channel];
    int bend = map(val, -8192, 8191, 0, 126);
    if (bendRange == 1) {
      bendFactor[channel] = bendVal1[bend];
    }
    else if (bendRange == 2) {
      bendFactor[channel] = bendVal2[bend];
    }
    else if (bendRange == 3) {
      bendFactor[channel] = bendVal3[bend];
    }
    else if (bendRange == 4) {
      bendFactor[channel] = bendVal4[bend];
    }
    else if (bendRange == 5) {
      bendFactor[channel] = bendVal5[bend];
    }
    else if (bendRange == 6) {
      bendFactor[channel] = bendVal6[bend];
    }
    else if (bendRange == 7) {
      bendFactor[channel] = bendVal7[bend];
    }
    else if (bendRange == 8) {
      bendFactor[channel] = bendVal8[bend];
    }
    else if (bendRange == 9) {
      bendFactor[channel] = bendVal9[bend];
    }
    else if (bendRange == 10) {
      bendFactor[channel] = bendVal10[bend];
    }
    else if (bendRange == 11) {
      bendFactor[channel] = bendVal11[bend];
    }
    else if (bendRange == 12) {
      bendFactor[channel] = bendVal12[bend];
    }
  }
}

void processBend(int channel) {
  pitchCurrent[channel] = pitchVals[initialPitch[channel]] * bendFactor[channel];
  pitchTarget[channel] = pitchCurrent[channel];
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
    floppyCount[i] = 0;
  }
  for (int i = 0; i < hddChannels + 1; i++) {
    prevDrum[i] = 0;
    drumDuration[i] = 0;
  }
  digitalWriteFast(floppyDirPin, LOW);
  digitalWriteFast(floppyDirPin2, LOW);

  for (int i = 0; i < maxFloppy; i++) {
    digitalWriteFast(floppyStep, HIGH);
    digitalWriteFast(floppyStep2, HIGH);
    digitalWriteFast(floppyStep, LOW);
    digitalWriteFast(floppyStep2, LOW);
    delay(5);
  }
  digitalWriteFast(floppyDirPin, HIGH);
  digitalWriteFast(floppyDirPin2, HIGH);
  digitalWriteFast(floppyStep, HIGH);
  digitalWriteFast(floppyStep2, HIGH);
  controlValue1 = -1;
  controlValue2 = -1;
  mute();
}

void mute() {
  for (int i = 0; i < maxChannel + 1; i++) {
    initialPitch[i] = -1;
    pitchTarget[i] = -1;
    pitchCurrent[i] = -1;
    acceleration[i] = -1;
  }
}
