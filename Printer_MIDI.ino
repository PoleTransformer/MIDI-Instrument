byte midiNote;
byte midiBendLsb;
byte midiControlNum;
byte midiAction = 0;
byte offsetChannel;

#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 5
#define maxSteps 1000
byte stepOrder[] = {IN1, IN3, IN2, IN4};
byte currentNote = 0;
unsigned long currentTime = 0;
unsigned long currentPeriod = 0;
unsigned long previousTime = 0;
int currentSteps = 0;
int currentOrder = 0;
bool currentDir = 1;

const unsigned long pitchVals[128] = {
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

void setup() {
  Serial.begin(115200);
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
}

void loop() {
  currentTime = micros();
  readMIDI();
  if (currentNote > 0 && currentTime - previousTime >= currentPeriod) {
    previousTime = currentTime;
    if (currentSteps >= maxSteps) {
      currentSteps = 0;
      currentDir = !currentDir;
      if (currentDir) {
        currentOrder = 0;
      }
      else {
        currentOrder = 3;
      }
    }
    if (currentDir) {
      if (currentOrder == 4) {
        currentOrder = 0;
        digitalWrite(stepOrder[3], LOW);
      }
      if (currentOrder > 0) {
        digitalWrite(stepOrder[currentOrder - 1], LOW);
      }
      digitalWrite(stepOrder[currentOrder], HIGH);
      currentOrder++;
    }
    else {
      if (currentOrder == -1) {
        currentOrder = 3;
        digitalWrite(stepOrder[0], LOW);
      }
      if (currentOrder < 3) {
        digitalWrite(stepOrder[currentOrder + 1], LOW);
      }
      digitalWrite(stepOrder[currentOrder], HIGH);
      currentOrder--;
    }
    currentSteps++;
  }
}

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (channel == 1) {
    currentNote = note;
    currentPeriod = pitchVals[note];
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (channel == 1) {
    if (note == currentNote) {
      currentNote = 0;
    }
  }
}

void handlePitchBend(byte channel, int bend) {

}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {

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
