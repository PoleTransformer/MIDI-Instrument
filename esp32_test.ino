byte midiNote;
byte midiBendLsb;
byte midiControlNum;
byte midiAction = 0;
byte offsetChannel;

uint32_t stepper[] = {0, BIT4, BIT16, BIT17, BIT5}; //index 0 is not used!!!
const int stepperLength = sizeof(stepper) / sizeof(stepper[0]);
unsigned long currentTime;
unsigned long previousTime[17];
unsigned int currentPeriod[17];
byte currentNote[17];
byte bendSens[17];
byte controlVal1[17];
byte controlVal2[17];
float bendFactor[17];
float originalBend[17];
float maxBend[17];
bool vibratoState[17];

unsigned long previousDrum[stepperLength];
unsigned int drumDuration[stepperLength];

hw_timer_t * timer = NULL;
volatile bool interruptbool1 = false;

const unsigned int pitchVals[128] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  61162, 57737, 54496, 51414, 48544, 45809, 43253, 40816, 38521, 36364, 34317, 32394, //C0 - B0
  30581, 28860, 27241, 25714, 24272, 22910, 21622, 20408, 19264, 18182, 17161, 16197, //C1 - B1
  15288, 14430, 13620, 12857, 12134, 11453, 10811, 10204, 9631, 9091, 8581, 8099, //C2 - B2
  7645, 7216, 6811, 6428, 6068, 5727, 5405, 5102, 4816, 4545, 4290, 4050, //C3 - B3
  3822, 3608, 3405, 3214, 3034, 2863, 2703, 2551, 2408, 2273, 2145, 2025, //C4 - B4
  1911, 1804, 1703, 1607, 1517, 1432, 1351, 1276, 1204, 1136, 1073, 1012, //C5 - B5
  956, 902, 851, 804, 758, 716, 676, 638, 602, 568, 536, 506, //C6 - B6
  478, 451, 426, 402, 379, 358, 338, 319, 301, 284, 268, 253, //C7 - B7
  239, 225, 213, 201, 190, 179, 169, 159, 150, 142, 134, 127,//C8 - B8
  0, 0, 0, 0, 0, 0, 0, 0
};

void IRAM_ATTR onTimer() {
  interruptbool1 = true;
}

void setup() {
  Serial.begin(115200);
  for (int i = 1; i < stepperLength; i++) {
    REG_SET_BIT(GPIO_ENABLE_REG, stepper[i]);
  }
  for (int i = 1; i < 17; i++) {
    bendFactor[i] = 1;
    originalBend[i] = 1;
    bendSens[i] = 2;
    maxBend[i] = 0;
  }
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 80000, true);
  timerAlarmEnable(timer);
}

void loop() {
  readMIDI();
  currentTime = micros();
  for (int i = 1; i < stepperLength; i++) {
    if (currentNote[i] > 0 && currentTime - previousTime[i] >= currentPeriod[i]) {
      previousTime[i] = currentTime;
      REG_SET_BIT(GPIO_OUT_W1TS_REG, stepper[i]);
      REG_SET_BIT(GPIO_OUT_W1TC_REG, stepper[i]);
    }
    if (drumDuration[i] > 0 && currentTime - previousDrum[i] >= 1000) {
      previousDrum[i] = currentTime;
      drumDuration[i]--;
      REG_SET_BIT(GPIO_OUT_W1TS_REG, stepper[i]);
      REG_SET_BIT(GPIO_OUT_W1TC_REG, stepper[i]);
    }
  }
  if (interruptbool1) {
    interruptbool1 = false;
    for (int i = 1; i < stepperLength; i++) {
      if (maxBend[i] > 0) {
        if (vibratoState[i]) {
          bendFactor[i] = originalBend[i] + maxBend[i];
        }
        else {
          bendFactor[i] = originalBend[i] - maxBend[i];
        }
        vibratoState[i] = !vibratoState[i];
        currentPeriod[i] = pitchVals[currentNote[i]] * bendFactor[i];
      }
    }
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (velocity == 0) {
    handleNoteOff(channel, note, velocity);
  }
  else if (channel == 10) {
    if (note == 35 || note == 36) {
      drumDuration[1] = map(velocity, 1, 127, 1, 5);
    }
    else if (note == 38 || note == 40) {
      drumDuration[2] = map(velocity, 1, 127, 1, 10);
    }
    else {
      drumDuration[3] = map(velocity, 1, 127, 1, 3);
    }
  }
  else {
    currentNote[channel] = note;
    currentPeriod[channel] = pitchVals[note] * bendFactor[channel];
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (note == currentNote[channel]) {
    currentNote[channel] = 0;
  }
}

void handlePitchBend(byte channel, int bend) {
  float bendF = bend;
  bendF /= 8192;
  bendF *= bendSens[channel];
  bendF /= 12;
  bendFactor[channel] = pow(2, bendF);
  originalBend[channel] = bendFactor[channel];
  currentPeriod[channel] = pitchVals[currentNote[channel]] * bendFactor[channel];
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  if (firstByte == 1) {
    if (secondByte > 0) {
      maxBend[channel] = myMap(secondByte, 1, 127, 0, 0.03);
    }
    else {
      maxBend[channel] = 0;
      bendFactor[channel] = originalBend[channel];
      currentPeriod[channel] = pitchVals[currentNote[channel]] * bendFactor[channel];
    }
  }
  if (firstByte == 100) {
    controlVal1[channel] = secondByte;
  }
  if (firstByte == 101) {
    controlVal2[channel] = secondByte;
  }
  if (firstByte == 6 && controlVal1[channel] == 0 && controlVal2[channel] == 0) {
    bendSens[channel] = secondByte;
    controlVal1[channel] = 127;
    controlVal2[channel] = 127;
  }
}

float myMap(float x, float in_min, float in_max, float out_min, float out_max) {
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
