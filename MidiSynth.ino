#include <MIDI.h>
#include <digitalWriteFast.h>
#include "pitches.h"

//Pin definitions:
#define stepPin_M1 3
#define stepPin_M2 2
#define stepPin_M3 12
#define stepPin_M4 4
#define dirPin_M1 6
#define dirPin_M2 5
#define dirPin_M3 13
#define dirPin_M4 7
#define disablePin1 8 //Steppers are disabled when this pin is HIGH.
#define disablePin2 9
#define drum1 10

//Variable definitions:
int pitchTarget[] = { -1, -1, -1, -1, -1}; //Holds the target pitch for the pulse functions.
int pitchCurrent[] = { -1, -1, -1, -1, -1}; //Holds the previous pitch value (useful for acceleration).
int acceleration[] = { -1, -1, -1, -1, -1};
unsigned long prevStepMicros[] = {0, 0, 0, 0, 0}; //last time
bool motorDirections[] = {LOW, LOW, LOW, LOW, LOW}; //Directions of the motors.
bool motorStallMode[] = {LOW, LOW, LOW, LOW, LOW};
MIDI_CREATE_DEFAULT_INSTANCE(); //use default MIDI settings

int currentPitch1 = -1;
int currentPitch2 = -1;
int bendVal1 = 0;
int bendVal2 = 0; 

bool drumFlag = false;
unsigned long drumPrevious = 0;
int duration = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  pinMode(stepPin_M1, OUTPUT);
  pinMode(stepPin_M2, OUTPUT);
  pinMode(stepPin_M3, OUTPUT);
  pinMode(stepPin_M4, OUTPUT);
  pinMode(dirPin_M1, OUTPUT);
  pinMode(dirPin_M2, OUTPUT);
  pinMode(dirPin_M3, OUTPUT);
  pinMode(dirPin_M4, OUTPUT);
  pinMode(disablePin1, OUTPUT);
  pinMode(disablePin2, OUTPUT);
  pinMode(drum1, OUTPUT);

  digitalWrite(dirPin_M1, LOW);
  digitalWrite(dirPin_M2, LOW);
  digitalWrite(dirPin_M3, LOW);
  digitalWrite(dirPin_M4, LOW);
  digitalWrite(disablePin1, HIGH);//Starts disabled!
  digitalWrite(disablePin2, HIGH);//Starts disabled!

  MIDI.begin(MIDI_CHANNEL_OMNI); //listen to all MIDI channels
  MIDI.setHandleNoteOn(handleNoteOn); //execute function when note on message is recieved
  MIDI.setHandleNoteOff(handleNoteOff); //execute function when note off message is recieved
  MIDI.setHandlePitchBend(myPitchBend);
  Serial.begin(115200); //Allows for serial MIDI communication.
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{
  //Read midi messages and step each stepper at the correct speed (called frequently)
  MIDI.read();
  singleStep(1, stepPin_M1);
  singleStep(2, stepPin_M2);
  singleStep(3, stepPin_M3);
  singleStep(4, stepPin_M4);
  if(micros() - drumPrevious >= duration && drumFlag) {
    digitalWriteFast(drum1, LOW);
    drumFlag = false;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handleNoteOn(byte channel, byte pitch, byte velocity) //MIDI Note ON Command
{
  if ((channel < 5) && (channel > 0)) //If it's a valid midi channel (1-4):
  {
    if (pitch == 54 || pitch == 55 || pitch == 56 || pitch == 57 || pitch == 58 || pitch == 59) //If it's a resonance note when rotating....
    {
      motorStallMode[channel] = HIGH;//The motor now intentionally "stalls" itself flag.
    }
    else
    {
      motorStallMode[channel] = LOW;//No longer stalling itself.
    }

    if (pitch > 83) //84)//needs acceleration control over this point. It's not guaranteed to start.
    {
      pitchTarget[channel] = pitchVals[pitch]; //Save the pitch to a global target array.
      pitchCurrent[channel] = pitchVals[70];//Force an acceleration from the highest pitch guaranteed to lock the rotor in first step.
      acceleration[channel] = 70;
    }
    else
    {
      pitchTarget[channel] = pitchVals[pitch]; //Save the pitch to a global target array.
      pitchCurrent[channel] = pitchVals[pitch]; //No acceleration!
    }

    if (channel == 1) {
      digitalWriteFast(disablePin1, LOW);//Therefore the system needs to be switched on. It is faster to write each time than check the flags and write!
      currentPitch1 = pitch;
    }

    if (channel == 2) {
      digitalWriteFast(disablePin2, LOW);//Therefore the system needs to be switched on. It is faster to write each time than check the flags and write!
      currentPitch2 = pitch;
    }
    prevStepMicros[channel] = micros();//Works better!?! BY A HUGE MARGIN TOO!?! (-300 helps the upper range a bit).
  }
  if(channel == 10 && !drumFlag) { //drum
    drumFlag = true;
    if(pitch <= 50 && pitch >= 35) {
      duration = map(velocity,0,127,0,4000);
    }
    else{
      duration = 1;
    }
    digitalWriteFast(drum1, HIGH);
    drumPrevious = micros();
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) //MIDI Note OFF Command
{

  if ((channel < 5) && (channel > 0)) //If it's a valid midi channel (1-4):
  {
    if (channel == 1) //If all the steppers are flagged as off:
    {
      digitalWriteFast(disablePin1, HIGH);//Disable all the steppers!
      currentPitch1 = -1;
    }

    if (channel == 2) //If all the steppers are flagged as off:
    {
      digitalWriteFast(disablePin2, HIGH);//Disable all the steppers!
      currentPitch2 = -1;
    }
    pitchTarget[channel] = -1;//set motor pitch to -1  (basically flags the situation later).
    pitchCurrent[channel] = -1;//Reset to a safe previous for the next next. Good practice.
    acceleration[channel] = -1;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void singleStep(byte motorNum, byte stepPin)
{
  if (((micros() - prevStepMicros[motorNum]) <= pitchCurrent[motorNum]))
  { //step when correct time has passed and the motor is at a nonzero speed
    //prevStepMicros[motorNum] += motorSpeeds[motorNum];//added accel
    //^^^ Instead of setting to Micros, this line allows for a feedback control that adjusts the total wait interval to account of "overshoot". BRILLIANT!
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
      if (currentPitch1 != -1 && bendVal1 != 0) {
        processBend(1);
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
      if (currentPitch2 != -1 && bendVal2 != 0) {
        processBend(2);
      }
    }
    else if (motorNum == 3)
    {
      digitalWriteFast(stepPin_M3, HIGH);
      digitalWriteFast(stepPin_M3, LOW);
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
    else if (motorNum == 4)
    {
      digitalWriteFast(stepPin_M4, HIGH);
      digitalWriteFast(stepPin_M4, LOW);
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
    prevStepMicros[motorNum] = prevStepMicros[motorNum] + pitchCurrent[motorNum];//Keeps track of the last time a tick occurred for the next time.
    if (pitchCurrent[motorNum] > pitchTarget[motorNum]) //Not yet on target?
    {
      acceleration[motorNum]++;
      pitchCurrent[motorNum] = pitchVals[acceleration[motorNum]]; //Move the current closer to the target (acceleration control)!
    }
  }
}

void myPitchBend(byte channel, int val) {
  if (channel == 1) {
    bendVal1 = val;
    if (bendVal1 == 0) {
      pitchCurrent[1] = pitchVals[currentPitch1];
      pitchTarget[1] = pitchCurrent[1];
    }
  }
  if (channel == 2) {
    bendVal2 = val;
    if (bendVal1 == 0) {
      pitchCurrent[2] = pitchVals[currentPitch2];
      pitchTarget[2] = pitchCurrent[2];
    }
  }
}

void processBend(int channel) {
  if (channel == 1) {
    if (currentPitch1 > 1 && currentPitch1 < 126) {
      int high = pitchVals[currentPitch1 - 2] - pitchVals[currentPitch1];
      int low = pitchVals[currentPitch1 + 2] - pitchVals[currentPitch1];
      if (bendVal1 < 0) {
        int bend = map(bendVal1, -1, -7250, -1, low);
        pitchTarget[1] = pitchVals[currentPitch1] - bend;
        pitchCurrent[1] = pitchTarget[1];
      }
      else if (bendVal1 > 0) {
        int bend = map(bendVal1, 1, 9250, 1, high);
        pitchTarget[1] = pitchVals[currentPitch1] - bend;
        pitchCurrent[1] = pitchTarget[1];
      }
    }
  }
  if (channel == 2) {
    if (currentPitch2 > 1 && currentPitch2 < 126) {
      int high = pitchVals[currentPitch2 - 2] - pitchVals[currentPitch2];
      int low = pitchVals[currentPitch2 + 2] - pitchVals[currentPitch2];
      if (bendVal2 < 0) {
        int bend = map(bendVal2, -1, -7250, -1, low);
        pitchTarget[2] = pitchVals[currentPitch2] - bend;
        pitchCurrent[2] = pitchTarget[2];
      }
      else if (bendVal2 > 0) {
        int bend = map(bendVal2, 1, 9250, 1, high);
        pitchTarget[2] = pitchVals[currentPitch2] - bend;
        pitchCurrent[2] = pitchTarget[2];
      }
    }
  }
}
