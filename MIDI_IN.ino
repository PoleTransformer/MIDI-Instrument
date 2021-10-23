byte incomingByte;
int lsb;
int msb;
byte data;
byte note;
byte channel;
byte velocity;
byte bendMsb;
byte bendLsb;
int action = -1;
int bendVal = 8191;

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
    if (msb == 9 && action == -1) { // note on
      channel = lsb;
      note = data;
      action = 1;
    }
    else if (action == 1) { //receive velocity
      velocity = data;
      action = -1;
    }
    if (msb == 8) { //note off
      channel = lsb;
      note = data;
    }
    if (msb == 14 && action == -1) { //pitch bend
      channel = lsb;
      bendLsb = data;
      action = 2;
    }
    else if (action = 2) { //receive bendmsb
      bendMsb = data;
      bendVal = (bendMsb << 7) | bendLsb;
      action = -1;
    }
  }
  if(bendVal == 16383) {
    digitalWrite(13, HIGH);
  }
  else {
    digitalWrite(13, LOW);
  }
}
