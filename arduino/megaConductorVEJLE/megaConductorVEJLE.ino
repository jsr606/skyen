// MEGA conductor for SKYEN

int led [] = {2, 3, 4, 5};
int button = 6;
int pot [] = {A0, A1};
boolean active [] = {true, true, true, true};
int tickLed = 13;
int pulseOut [] = {8, 9, 10, 11};

boolean buttonPushed = false;

unsigned long nextTick = millis();
unsigned long lastSerialCommand = millis();
int blinkDuration = 15;
int seconds = 0;
boolean tock = false, ticked = false;
int nextProgramChange = 1;
int morphingParameter = -1, startVal = -1, endVal = -1, morphLength = -1, morphProgression = -1, morphVal = -1, lastMorphVal = -1, peakDuration = -1;
boolean morphing = false, morphRiseAndFall = true, morphLooping = true;
int morphState = -1;

unsigned long lastMorph;
int morphFreq = 40;
int lastFeedbackSec = -1;
int counterActionFrequency = 10, counterAction = 0, counterLow = 0, counterHigh = 251;
int burstChance = -1;

int currentPreset = -1, amountOfPresets = 2;

int program = 2, lastProgram = -1, amountOfPrograms = 9;

int fadeInTime = -1, fadeOutTime = -1, intensity = -1, probability = -1;

int program1 = 1, program2 = 7, programTendency = 30, programChance = 100;

boolean fanActive = false;

// programs
#define REST 0
#define STARS 1
#define SOLID 2
#define BLINKSOLID 3
#define VU 4
#define MIDDLEVU 5
#define DIRECTIONAL 6
#define RANDOMBITS 7
#define FAN 8

// helper programs
#define PULSETEST 9
#define AUXTEST 10
#define SHOWID 11

// morphing / counter action parameters
#define NONE 0
#define STEPDELAY 1
#define INTENSITY 2
#define PROBABILITY 3
#define FADEIN 4
#define FADEOUT 5
#define RANDOMSTEPDELAY 6

// morph states
#define RISE 0
#define PEAK 1
#define FALL 2

int slowness = 4;
boolean morphFeedback = false;

boolean serialUSBFeedback = false;

void setup() {

  Serial.begin(115200);
  Serial1.begin(115200);
  Serial2.begin(115200);
  Serial3.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(led[i], OUTPUT);
    pinMode(pulseOut[i], OUTPUT);
    digitalWrite(pulseOut[i], HIGH); // pulse active low
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
  preset(0);
}

void loop() {

  tick();

  if (digitalRead(button) == LOW) {
    if (buttonPushed == false) {
      sendPulse();
      buttonPushed = true;
      morphFeedback = !morphFeedback;
    }
  } else {
    buttonPushed = false;
  }

  if (Serial.available() > 2) {

    byte startByte = Serial.read();
    byte command, value;

    if (startByte == 255) {
      // serial data
      command = Serial.read();
      value = Serial.read();
      sendData(command, value);
    }

    if (startByte == 254) {
      // stealth serial data
      command = Serial.read();
      value = Serial.read();
      sendStealthData(command, value);
    }

    if (startByte == 251) {
      // activate / deactivate serial ports
      byte port = Serial.read();
      value = Serial.read();
      activateSerialPort(port , value);
    }

  }

  if (lastSerialCommand + blinkDuration < millis()) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(led[i], active[i]);
    }
  }


  if (ticked) {
    if (serialUSBFeedback) Serial.print(seconds);
    if (serialUSBFeedback) Serial.print("/");
    if (serialUSBFeedback) Serial.print(nextProgramChange * slowness);
    if (serialUSBFeedback) Serial.print(".");
    if (serialUSBFeedback && seconds % 10 == 0) Serial.println();
    if (seconds >= nextProgramChange * slowness) {
      int nextPreset = currentPreset;
      while (currentPreset == nextPreset) {
        nextPreset = random(21);
      }
      preset(nextPreset);
    }
    if (seconds % counterActionFrequency == 0) {
      doCounterAction();
    }

    int ran = random(1000 * slowness);
    if (ran < programChance) {
      if (serialUSBFeedback) Serial.println("time to pick a random program");

      if (ran < programChance / 10) {
        if (serialUSBFeedback) Serial.println("and time to shuffle the programs");
        program1 = random(amountOfPrograms);
        program2 = random(amountOfPrograms);
      }

      if (random(100) < programTendency) {
        if (serialUSBFeedback) Serial.println("picked program 1");
        programChange(program1);
      } else {
        if (serialUSBFeedback) Serial.println("picked program 2");
        programChange(program2);
      }

    }

  }

  if (morphing) {
    if (lastMorph + morphFreq < millis()) {
      morph();
      lastMorph = millis();
    }
  }

  if (burstChance > 0) {
    int ran = random(10000);
    if (ran < burstChance) {
      sendData('B', random(30));
    }
  }

}

void activateSerialPort(byte port, byte value) {
  if (port < 4) {
    if (value == 1) {
      active[port] = true;
    } else {
      active[port] = false;
    }
  }
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
  if (counterAction == NONE) return;
  if (serialUSBFeedback) Serial.print("counterAction: ");
  if (counterAction == INTENSITY) {
    if (serialUSBFeedback) Serial.println("intensity");
    int val = random(counterLow, counterHigh);
    sendData('V', val);
  }
  if (counterAction == RANDOMSTEPDELAY) {
    if (serialUSBFeedback) Serial.println("random step delay");
    int val = random(counterLow, counterHigh);
    sendData('D', val);
  }
  if (counterAction == STEPDELAY) {
    if (serialUSBFeedback) Serial.println("nudge step delay");
    nudgeParameter(STEPDELAY, 3);
  }
  if (counterAction == FADEIN) {
    if (serialUSBFeedback) Serial.println("nudge fadeIn");
    nudgeParameter(FADEIN, 5);
  }
  if (counterAction == FADEOUT) {
    if (serialUSBFeedback) Serial.println("nudge fadeOut");
    nudgeParameter(FADEOUT, 15);
  }
  if (counterAction == INTENSITY) {
    nudgeParameter(INTENSITY, 8);
  }
  if (counterAction == PROBABILITY) {
    nudgeParameter(INTENSITY, 15);
  }
}

void rest() {
  if (serialUSBFeedback) Serial.print("resting");
  delay(random(500, 5000));
  if (serialUSBFeedback) Serial.println(".");
}

void nudgeParameter (int _param, int amount) {
  switch (_param) {
    case FADEIN:
      fadeInTime += random(-amount, amount);
      fadeInTime = constrain(fadeInTime, 0, 250);
      sendData('I', fadeInTime);
      if (serialUSBFeedback) Serial.print("fadeIn time is now ");
      if (serialUSBFeedback) Serial.println(fadeInTime);
      break;
    case FADEOUT:
      fadeOutTime += random(-amount, amount);
      fadeOutTime = constrain(fadeOutTime, 0, 250);
      sendData('O', fadeOutTime);
      if (serialUSBFeedback) Serial.print("fadeOut time is now ");
      if (serialUSBFeedback) Serial.println(fadeOutTime);
      break;
    case INTENSITY:
      intensity += random(-amount, amount);
      intensity = constrain(intensity, 0, 100);
      sendData('V', intensity);
      if (serialUSBFeedback) Serial.print("intensity is now ");
      if (serialUSBFeedback) Serial.println(intensity);
      break;
    case PROBABILITY:
      probability += random(-amount, amount);
      probability = constrain(probability, 0, 100);
      sendData('C', probability);
      if (serialUSBFeedback) Serial.print("probability is now ");
      if (serialUSBFeedback) Serial.println(probability);
      break;
  }
}

void preset(int _preset) {
  currentPreset = _preset;
  //rest();
  morphing = false;
  if (serialUSBFeedback) Serial.print("change preset to ");
  if (serialUSBFeedback) Serial.println(currentPreset);

  int ran = random(3);
  switch (currentPreset) {
    case 0:
      if (serialUSBFeedback) Serial.println("random bits, maybe scrolling");
      fadeInTime = 10;
      sendData('I', fadeInTime);
      fadeOutTime = random(50, 250);
      sendData('O', fadeOutTime);
      sendData('D', random(150, 250));
      sendData('S', 127);
      intensity = 50;
      sendData('V', intensity);
      sendData('s', random(50, 200));
      sendData('A', random(1, 3));
      sendData('B', 4);
      programChange(RANDOMBITS);
      newMorph(INTENSITY, 100, 0, 50, true, true);
      nextProgramChange = random(5, 8);
      counterActionFrequency = random(5, 10);
      counterAction = INTENSITY;
      counterHigh = 250;
      counterLow = 5;
      break;
    case 11:
      if (serialUSBFeedback) Serial.println("set autoburst off");
      sendData('A', 0);
      seconds = (nextProgramChange * slowness) - 1;
      break;
    case 12:
      if (serialUSBFeedback) Serial.println("set white noise level");
      sendData('N', random(0, 30));
      seconds = (nextProgramChange * slowness) - 1;
      break;
    case 13:
      if (serialUSBFeedback) Serial.println("white noise off");
      sendData('N', 0);
      seconds = (nextProgramChange * slowness) - 1;
      break;
    case 14:
      if (serialUSBFeedback) Serial.println("set black noise level");
      sendData('n', random(0, 30));
      seconds = (nextProgramChange * slowness) - 1;
      break;
    case 15:
      if (serialUSBFeedback) Serial.println("black noise off");
      sendData('n', 0);
      seconds = (nextProgramChange * slowness) - 1;
      break;
    case 16:
      if (fanActive) {
        if (serialUSBFeedback) Serial.println("fan on");
        programChange(FAN);
        sendData('V', random(0, 251));
        seconds = (nextProgramChange * slowness) - 1;
      }
      break;
    case 17:
      if (serialUSBFeedback) Serial.println("fan off");
      programChange(FAN);
      sendData('V', random(0));
      seconds = (nextProgramChange * slowness) - 1;
      break;
    case 18:
      if (serialUSBFeedback) Serial.println("maybe scroll?");
      sendData('S', random(125, 129));
      sendData('s', random(250));
      seconds = (nextProgramChange * slowness) - 1;
      break;
    case 19:
      int ran = random(5);
      if (serialUSBFeedback) Serial.println("set counter action to");
      if (ran == 0) counterAction = PROBABILITY;
      if (ran == 1) counterAction = INTENSITY;
      if (ran == 2) counterAction = STEPDELAY;
      if (ran == 3) counterAction = FADEIN;
      if (ran == 4) counterAction = FADEOUT;
      seconds = (nextProgramChange * slowness) - 1;
      break;
    case 20:
      if (serialUSBFeedback) Serial.println("set autoburst on");
      sendData('A', random(250));
      seconds = (nextProgramChange * slowness) - 1;
      break;

    //Serial.println("nudge fadein time");
    //fadeIn
    /*
      case 1:
      Serial.println("stars");
      sendData('L', random(100, 150));
      programChange(STARS);
      sendData('I', random(5));
      sendData('O', random(200, 251));
      sendData('S', random(5, 25));
      newMorph(STEPDELAY, random(30, 100), random(0, 15), random(200, 800), true, true);
      counterAction = STEPDELAY;
      counterHigh = 35;
      counterLow = 1;
      burstChance = random(100);
      nextProgramChange = random(4);
      break;

      case 2:
      Serial.println("totes random");
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
      sendData('I', random(5));
      sendData('O', random(10, 250));
      nextProgramChange = 0;
      break;
      case 6:
      Serial.println("nothing here");
      break;
      case 7:
      Serial.println("VU middle");
      sendData('I', 0);
      sendData('O', random(1, 25));
      sendData('S', random(0, 10));
      programChange(MIDDLEVU);
      newMorph(STEPDELAY, random(0, 30), random(30, 251), random(50, 200), true, true);
      nextProgramChange = random(1, 3);
      break;
      case 8:
      Serial.println("autoburst on");
      sendData('A', random(1, 251));
      break;
      case 9:
      Serial.println("autoburst off");
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
    */
    default:
      break;

  }
  if (serialUSBFeedback) Serial.print("new preset ");
  if (serialUSBFeedback) Serial.print(_preset);
  if (serialUSBFeedback) Serial.print(": ");
  printMorphSettings();
  currentPreset = _preset;
}

void printMorphSettings() {
  if (serialUSBFeedback) {
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

  morphProgression++;

  switch (morphState) {
    case RISE:
      morphVal = map(morphProgression, 0, morphLength, startVal, endVal);
      if (morphProgression == morphLength) {
        morphState = PEAK;
        peakDuration = random(morphLength / 5 , morphLength / 3);
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
      if (serialUSBFeedback) {
        if (morphState == RISE) Serial.print("RISE ");
        if (morphState == PEAK) Serial.print("PEAK ");
        if (morphState == FALL) Serial.print("FALL ");

        Serial.print(morphProgression);
        Serial.print(" / ");
        Serial.print(morphLength);
        Serial.print(" val ");
        Serial.print(morphVal);
      }
    }
    if (morphingParameter == STEPDELAY) {
      sendStealthData('S', morphVal);
      if (serialUSBFeedback && morphFeedback) Serial.print(" stepDelay");
    }
    if (morphingParameter == INTENSITY) {
      sendStealthData('V', morphVal);
      if (serialUSBFeedback && morphFeedback) Serial.print(" intensity");
    }
    if (morphingParameter == PROBABILITY) {
      sendStealthData('C', morphVal);
      if (serialUSBFeedback && morphFeedback) Serial.print(" probability");
    }
    if (serialUSBFeedback && morphFeedback) Serial.println();

    if (morphState == PEAK) {
      if (serialUSBFeedback && morphFeedback) Serial.print("hold it for ");
      if (serialUSBFeedback && morphFeedback) Serial.println(peakDuration);
    }
    lastMorphVal = morphVal;
  }

}

void programChange(int p) {
  if (serialUSBFeedback) Serial.print("changing to program ");
  if (serialUSBFeedback) Serial.println(p);
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

  for (int i = 0; i < 4; i++) {

    if (active[i]) {
      digitalWrite(led[i], LOW);

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
    lastSerialCommand = millis();
    //delay(2);
  }
}

void sendStealthData(byte command, byte value) {
  for (int i = 0; i < 4; i++) {
    if (active[i]) {
      digitalWrite(led[i], LOW);
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
    //delay(2);
    lastSerialCommand = millis();
  }
}
