#include <TimerOne.h>
#include <MIDI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
bool buzzerState = false;
bool enableBuzzer = false;
bool buzzerFlag = false;
bool enableLED = false;
bool ledState = false;
bool flag = false;
bool newLine = false;
byte buff[127];
int i = 0;
int j = 0;
int printedChar = 0;
unsigned long prevMillis = 0;
unsigned long duration = 100;

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display
MIDI_CREATE_DEFAULT_INSTANCE();

void setup()
{
  Timer1.initialize(40000);
  Timer1.attachInterrupt(buzz);
  lcd.init();
  lcd.backlight();
  lcd.cursor();
  lcd.blink();
  for (int i = 2; i < 14; i++) {
    pinMode(i, OUTPUT);
  }
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(115200);
}

void loop()
{
  MIDI.read();
  if (flag && millis() - prevMillis >= duration) {
    prevMillis = millis();
    if (i >= j) {
      flag = false;
      enableBuzzer = false;
      buzzerState = false;
      enableLED = false;
      ledState = false;
      i = 0;
    }
    else {
      lcd.print(char(buff[i]));
      printedChar++;
      i++;
      if (printedChar == 16 && !newLine) {
        lcd.setCursor(0, 1);
        newLine = true;
      }
      else if (printedChar == 32) {
        clearDisplay();
      }
    }
  }
  if (buzzerState) {
    tone(2, 880);
  }
  else {
    noTone(2);
  }
  if (ledState) {
    digitalWrite(3, HIGH);
  }
  else {
    digitalWrite(3, LOW);
  }
}

void buzz() {
  if (enableBuzzer) {
    buzzerState = !buzzerState;
  }
  if (enableLED) {
    ledState = !ledState;
  }
}

void handleControlChange(byte channel, byte firstByte, byte secondByte) {
  if (firstByte == 121) { //reset everything
    clearDisplay();
    j = 0;
  }
  if (firstByte == 14) {
    if (secondByte == 6) { //print buffer
      flag = true;
      enableLED = true;
      enableBuzzer = buzzerFlag;
    }
    else if (secondByte == 7) { //clear buffer
      j = 0;
    }
    else if (secondByte == 8) { //clear display
      clearDisplay();
    }
    else if (secondByte == 9) { //enable buzzer
      buzzerFlag = true;
    }
    else if (secondByte == 10) { //disable buzzer
      buzzerFlag = false;
    }
    else if (secondByte == 11) { //newline
      lcd.setCursor(0, 1);
      newLine = true;
    }
    else if(secondByte == 12) { //backspace
      if(printedChar > 0) {
        if(newLine) {
          lcd.setCursor(printedChar-1,1);
        }
        else {
          lcd.setCursor(printedChar-1,0);
        }
      }
    }
    else {
      buff[j] = secondByte;
      j++;
    }
  }
}

void clearDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  printedChar = 0;
  newLine = false;
}
