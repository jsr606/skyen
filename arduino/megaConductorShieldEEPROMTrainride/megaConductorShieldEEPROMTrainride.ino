#include <MCP48xx.h>

MCP4822 dac(53);

int led [] = {2, 3, 4, 5};
int button = 6;
int pot [] = {A0, A1};
boolean active [] = {true, true, true, true};
int tickLed = 13;
int pulseOut [] = {8, 9, 10, 11};

boolean buttonPushed = false;

unsigned long nextTick = millis();
int seconds = 0;
boolean tock = false, ticked = false;
int nextProgramChange = 1;
int morphingParameter = -1, startVal = -1, endVal = -1, morphLength = -1, morphProgression = -1, morphVal = -1, lastMorphVal = -1, peakDuration = -1;
boolean morphing = false, morphRiseAndFall = true, morphLooping = true;
int morphState = -1;
byte formation = 0b00000111;

unsigned int lastMorph;
int morphFreq = 40, lastFeedbackSec = -1;
int counterActionFrequency = 10, counterAction = -1, counterLow = 0, counterHigh = 251;
int burstChance = -1;

int currentPreset = -1;

#define RISE 0
#define PEAK 1
#define FALL 2
#define DONE 3

int program = 2, lastProgram = -1, amountOfPrograms = 14;
#define REST 0
#define STARS 1
#define SOLID 2
#define BLINKSOLID 3
#define VU 4
#define MIDDLEVU 5
#define DIRECTIONAL 6
#define RANDOMBITS 7
#define FAN 8
#define PULSETEST 9
#define AUXTEST 10
#define SHOWID 11

#define NONE 0
#define STEPDELAY 1
#define INTENSITY 2
#define PROBABILITY 3
#define LCDFREQUENCY 4

int slowness = 15;
boolean morphFeedback = false;

boolean activeVoltage = false;
boolean scrolling = false;

boolean receivingSpell = false;

void setup() {
  dac.init();
  dac.turnOnChannelA();
  dac.turnOnChannelB();
  dac.setGainA(MCP4822::High);
  dac.setGainB(MCP4822::High);

  Serial.begin(115200);
  Serial1.begin(115200);
  Serial2.begin(115200);
  Serial3.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(led[i], OUTPUT);
    pinMode(pulseOut[i], OUTPUT);
    digitalWrite(pulseOut[i], HIGH);
  }

  pinMode(tickLed, OUTPUT);
  pinMode(button, INPUT_PULLUP);

  // blink LEDs
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 4; j++) {
      digitalWrite(led[j], HIGH);
    }
    delay(50);
    for (int j = 0; j < 4; j++) {
      digitalWrite(led[j], LOW);
    }
    delay(50);
  }

  delay(100);
  sendData('R', 0);
}

void loop() {

  tick();

  // send control voltages

  int p0 = analogRead(pot[0]);
  int p1 = analogRead(pot[1]);
  int v0 = map(p0, 1023, 0, 0, 4095);
  int v1 = map(p1, 1023, 0, 0, 4095);

  dac.setVoltageA(v0);
  dac.setVoltageB(v1);
  dac.updateDAC();

  if (digitalRead(button) == LOW) {
    if (buttonPushed == false) {
      sendPulse();
      buttonPushed = true;
    }
  } else {
    buttonPushed = false;
  }

  if (receivingSpell) {
    receiveSpell();
  } else if (Serial.available() > 2) {
    byte startByte = Serial.read();
    byte command, value;
    if (startByte == 255) {
      command = Serial.read();
      value = Serial.read();
      sendData(command, value);
    }
    if (startByte == 254) {
      command = Serial.read();
      value = Serial.read();
      sendStealthData(command, value);
    }
    if (startByte == 253) {
      receivingSpell = true;
      for (int i = 0; i < 4; i++) {
        digitalWrite(led[i], HIGH);
      }
    }
    if (startByte == 252) {
      receivingSpell = false;
      for (int i = 0; i < 4; i++) {
        digitalWrite(led[i], LOW);
      }
    }
  }


  /*
    if (ticked && seconds % 5 == 0) {

    sendData('P', PULSETEST);
    //Serial.println("pulseTest");


    sendData('R', 0);
    sendData('V', 0);
    sendData('F', FAN);
    sendData('V', 10);
    sendData('P', SOLID);


    sendData('V', 0);
    sendData('P', FAN);
    Serial.println("fans off");

    sendData('P', SOLID);

    }
  */
  /*
    if (ticked) {
    Serial.print(seconds);
    Serial.print(".");
    if (seconds >= nextProgramChange * slowness) {
      int nextPreset = currentPreset;
      while (currentPreset == nextPreset) {
        nextPreset = random(10);
      }
      preset(nextPreset);
    }
    if (seconds % counterActionFrequency == 0) {
      doCounterAction();
    }
    }
    if (morphing && lastMorph + morphFreq < millis()) {
    morphProgression++;
    morph();
    lastMorph = millis();
    }
    if (burstChance > 0) {
    int ran = random(10000);
    if (ran < burstChance) {
      sendData('B', random(30));
    }
    }
  */
  delay(20);
}

void receiveSpell() {
  if (Serial.available() > 0) {
    byte spellByte = Serial.read();

    if (spellByte == 252) {
      receivingSpell = false;
      for (int i = 0; i < 4; i++) {
        digitalWrite(led[i], LOW);
      }
    }

    Serial.write(spellByte);
    Serial1.write(spellByte);
    Serial2.write(spellByte);
    Serial3.write(spellByte);
  }
}

void parseIncomingSerial(char command, byte value) {

}

void sendPulse() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(pulseOut[i], LOW);
  }
  delay(100);
  for (int i = 0; i < 4; i++) {
    digitalWrite(pulseOut[i], HIGH);
  }
}

void doCounterAction() {
  Serial.println("counterAction time");
  formation = ~formation;
  if (counterAction == LCDFREQUENCY) {
    sendData('L', random(counterLow, counterHigh));
  }
  if (counterAction == INTENSITY) {
    sendData('V', random(counterLow, counterHigh));
  }
  formation = ~formation;
}

void rest() {
  Serial.print("resting");
  delay(random(500, 5000));
  Serial.println(".");
}

void preset(int _preset) {
  currentPreset = _preset;
  rest();
  morphing = false;
  Serial.print("change preset to ");
  Serial.println(currentPreset);

  int ran = random(3);
  switch (currentPreset) {
    case 0:
      Serial.println("directionals");
      formation = 0b00000111;
      sendData('I', 10);
      sendData('O', random(50, 250));
      sendData('S', random(150, 250));
      sendData('C', random(1, 5));
      programChange(DIRECTIONAL);
      newMorph(PROBABILITY, random(100), 0, random(200, 800), true, true);
      formation = 0b00000101;
      nextProgramChange = random(5, 7);
      counterActionFrequency = random(13, 25);
      counterAction = STEPDELAY;
      counterHigh = 251;
      counterLow = 20;
      break;
    case 1:
      Serial.println("stars");
      formation = 0b00000111;
      sendData('L', random(100, 150));
      programChange(STARS);
      sendData('I', random(5));
      sendData('O', random(200, 251));
      sendData('S', random(5, 25));
      formation = 0b00000101;
      newMorph(STEPDELAY, random(30, 100), random(0, 15), random(200, 800), true, true);
      counterAction = STEPDELAY;
      counterHigh = 35;
      counterLow = 1;
      burstChance = random(100);
      nextProgramChange = random(4);
      break;
    case 2:
      Serial.println("totes random");
      if (ran == 0) formation = 0b00000111;
      if (ran == 1) formation = 0b00000101;
      if (ran == 2) formation = 0b00000010;
      programChange(random(0, amountOfPrograms));
      newMorph(random(4), random(0, 50), random(200, 250), random(200, 800), true, true);
      counterAction = random(1, 5);
      counterHigh = random(255);
      counterLow = 0;
      burstChance = random(100);
      nextProgramChange = random(4);
      break;
    case 3:
      Serial.println("hectic stuff");
      formation = 0b00000111;
      sendData('I', 0);
      sendData('O', random(1, 25));
      sendData('S', random(0, 10));
      nextProgramChange = random(2);
      break;
    case 4:
      Serial.println("rest");
      programChange(REST);
      nextProgramChange = random(1, 2);
      break;
    case 5:
      Serial.println("chill trails");
      formation = 0b00000111;
      sendData('I', random(5));
      sendData('O', random(10, 250));
      nextProgramChange = 0;
      break;
    case 6:
      Serial.println("LCD frequency");
      formation = 0b00000111;
      newMorph(LCDFREQUENCY, random(0, 251), random(30, 251), random(50, 200), false, false);
      nextProgramChange = 2;
      break;
    case 7:
      Serial.println("VU middle");
      formation = 0b00000010;
      sendData('I', 0);
      sendData('O', random(1, 25));
      sendData('S', random(0, 10));
      programChange(MIDDLEVU);
      newMorph(STEPDELAY, random(0, 30), random(30, 251), random(50, 200), true, true);
      nextProgramChange = random(1, 3);
      break;
    case 8:
      Serial.println("autoburst on");
      if (ran == 0) formation = 0b00000111;
      if (ran == 1) formation = 0b00000101;
      if (ran == 2) formation = 0b00000010;
      sendData('A', random(1, 251));
      break;
    case 9:
      Serial.println("autoburst off");
      formation = 0b00000111;
      sendData('A', 0);
      break;
    case 10:
      Serial.println("new counter action frequency");
      counterActionFrequency = random(0, 200);
      break;
    case 11:
      Serial.println("no more knight rider");
      break;
    case 12:
      Serial.println("fast LCD shutter");
      sendData('L', random(0, 15));
      break;
    case 13:
      Serial.println("random bits");
      sendData('I', 1);
      sendData('O', random(50));
      sendData('P', 8);
      sendData('S', random(15));
      break;
    default:
      break;

  }
  Serial.print("new preset ");
  Serial.print(_preset);
  Serial.print(": ");
  printMorphSettings();
  currentPreset = _preset;
}

void printMorphSettings() {
  if (morphing) {
    Serial.print("morph");
    if (morphRiseAndFall) Serial.print (" riseAndFall");
    if (morphLooping) Serial.print(" looping");

    Serial.print(" ");
    Serial.print(startVal);
    Serial.print(" / ");
    Serial.print(endVal);
    Serial.print(" length: ");
    Serial.print(morphLength);

    switch (morphingParameter) {
      case (STEPDELAY):
        Serial.print(" stepDelay");
        break;
      case (INTENSITY):
        Serial.print(" intensity");
        break;
      case (PROBABILITY):
        Serial.print(" probability");
        break;
      case (LCDFREQUENCY):
        Serial.print(" LCD frequency");
        break;
    }
  }

  Serial.print(" ");
  for (int i = 0; i < 3; i++) {
    bool _f = bitRead(formation, i);
    if (_f) {
      Serial.write('1');
    } else {
      Serial.write('0');
    }
  }

  Serial.println();
  Serial.print("slowness ");
  Serial.print(slowness);
  Serial.print(" next program change ");
  Serial.print(nextProgramChange * slowness);
  Serial.print(" counterAction frequency ");
  Serial.println(counterActionFrequency);

}

void newMorph(int _param, int _startVal, int _endVal, int _dur, boolean _loop, boolean _riseAndFall) {
  morphingParameter = _param;
  morphRiseAndFall = _riseAndFall;
  morphLooping = _loop;
  morphState = RISE;
  setupMorph (_startVal, _endVal, _dur);
}

void setupMorph(int _startVal, int _endVal, int dur) {
  startVal = _startVal;
  endVal = _endVal;
  morphLength = dur;
  morphProgression = 0;
  morph();
  morphing = true;
}

void morph() {
  switch (morphState) {
    case RISE:
      morphVal = map(morphProgression, 0, morphLength, startVal, endVal);
      if (morphProgression == morphLength) {
        morphState = PEAK;
        peakDuration = random(morphLength / 5, morphLength / 3);
        morphProgression = 0;
      }
      break;
    case PEAK:
      if (morphProgression == peakDuration) {
        if (morphRiseAndFall) {
          setupMorph(endVal, startVal, morphLength);
          morphState = FALL;
        } else {
          if (morphLooping) {
            setupMorph(startVal, endVal, morphLength);
            morphState = RISE;
          } else {
            morphing = false;
          }
        }
      }

      break;
    case FALL:
      morphVal = map(morphProgression, 0, morphLength, startVal, endVal);
      if (morphProgression == morphLength) {
        if (morphLooping) {
          setupMorph(endVal, startVal, morphLength);
          morphState = RISE;
        } else {
          morphing = false;
        }
      }
      break;
  }

  if (morphVal != lastMorphVal) {
    if (morphFeedback) {
      if (morphState == RISE) Serial.print("RISE ");
      if (morphState == PEAK) Serial.print("PEAK ");
      if (morphState == FALL) Serial.print("FALL ");

      Serial.print(morphProgression);
      Serial.print(" / ");
      Serial.print(morphLength);
      Serial.print(" val ");
      Serial.print(morphVal);
    }
    if (morphingParameter == STEPDELAY) {
      sendData('S', morphVal);
      if (morphFeedback) Serial.print(" stepDelay");
    }
    if (morphingParameter == INTENSITY) {
      sendData('V', morphVal);
      if (morphFeedback) Serial.print(" instensity");
    }
    if (morphingParameter == PROBABILITY) {
      sendData('C', morphVal);
      if (morphFeedback) Serial.print(" probability");
    }
    if (morphingParameter == LCDFREQUENCY) {
      sendData('L', morphVal);
      if (morphFeedback) Serial.print(" LCD frequency");
    }
    if (morphFeedback) Serial.println();

    if (morphState == PEAK) {
      if (morphFeedback) Serial.print("hold it for ");
      if (morphFeedback) Serial.println(peakDuration);
    }
    lastMorphVal = morphVal;
  }
}

void programChange(int p) {
  sendData('P', p);
  seconds = 0;
}

void programChange() {
  morphing = false;
  lastProgram = program;
  while (program == lastProgram) {
    program = random(amountOfPrograms);
  }
  sendData('P', program);
  seconds = 0;
}

void tick() {
  ticked = false;
  if (millis() > nextTick) {
    tock = !tock;
    digitalWrite(tickLed, tock);
    seconds ++;
    nextTick = millis() + 1000;
    ticked = true;
  }
}

void sendData(byte command, byte value) {

  /*
    if (morphFeedback) Serial.print("\t");
    for (int i = 0; i < 3; i++) {
    active[i] = bitRead(formation, i);
    if (active[i]) {
      if (morphFeedback) Serial.write('1');
    } else {
      if (morphFeedback) Serial.write('0');
    }
    }
  */

  for (int i = 0; i < 4; i++) {
    digitalWrite(led[i], active[i]);

    if (active[i]) {
      if (i == 0) {
        Serial.write(255);
        Serial.write(command);
        Serial.write(value);
      } else if (i == 1) {
        Serial1.write(255);
        Serial1.write(command);
        Serial1.write(value);
      } else if (i == 2) {
        Serial2.write(255);
        Serial2.write(command);
        Serial2.write(value);
      } else if (i == 3) {
        Serial3.write(255);
        Serial3.write(command);
        Serial3.write(value);
      }
    }
    delay(2);
    digitalWrite(led[i], LOW);
  }
}

void sendStealthData(byte command, byte value) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(led[i], active[i]);
    if (active[i]) {
      if (i == 0) {
        Serial.write(254);
        Serial.write(command);
        Serial.write(value);
      } else if (i == 1) {
        Serial1.write(254);
        Serial1.write(command);
        Serial1.write(value);
      } else if (i == 2) {
        Serial2.write(254);
        Serial2.write(command);
        Serial2.write(value);
      } else if (i == 3) {
        Serial3.write(254);
        Serial3.write(command);
        Serial3.write(value);
      }
    }
    delay(2);
    digitalWrite(led[i], LOW);
  }
}
