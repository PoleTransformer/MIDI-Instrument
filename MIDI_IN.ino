byte incomingByte;
byte data;
byte note;
byte channel;
byte bendMsb;
byte bendLsb;
byte controlNum;
byte controlVal;
int lsb;
int msb;
int action = -1;
int bendVal = 0;

void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(115200);
}

void loop () {
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    int command = (incomingByte & 0b10000000) >> 7;
    if (command == 1) { //status
      msb = (incomingByte & 0b11110000) >> 4;
      lsb = incomingByte & 0b00001111;
    }
    else if (command == 0) { //data
      data = incomingByte;
    }
    if (msb == 9) { //note on
      channel = lsb;
      note = data;
    }
    if (msb == 8) { //note off
      channel = lsb;
      note = data;
    }
    if (msb == 11 && action == -1) { //control change
      channel = lsb;
      controlNum = data;
      action = 1;
    }
    else if (action == 1) { //receive controller value
      controlVal = data;
      action = -1;
    }
    if (msb == 14 && action == -1) { //pitch bend
      channel = lsb;
      bendLsb = data;
      action = 2;
    }
    else if (action == 2) { //receive bendmsb
      bendMsb = data;
      bendVal = (bendMsb << 7) | bendLsb;
      bendVal -= 8192;
      action = -1;
    }
  }
  if (bendVal == 8191) { //debugging
    digitalWrite(13, HIGH);
  }
  else if(bendVal == -8192) {
    digitalWrite(13, LOW);
  }
}
