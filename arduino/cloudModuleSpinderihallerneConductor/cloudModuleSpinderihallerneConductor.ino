#include <SoftPWM.h>
#include <EEPROM.h>

#define EEPROM_INIT 1023
#define EEPROM_SPELL 1022
#define EEPROM_ID 0
#define EEPROM_FLIPPED 1
#define EEPROM_TURNED 2
#define EEPROM_XPOS 3
#define EEPROM_YPOS 4
#define EEPROM_SPELL_BEGIN 10
#define EEPROM_SPELL_END 1010

boolean flipped = false, turned = false;
byte xPos = -1, yPos = -1;
int id = -1, selectedID = -1;

unsigned long nextTick = millis();
int seconds = 0;
boolean ticked = false;
boolean pulsed = false;

int led [] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, A3, A4};
int unFlippedLeds [] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, A3, A4};
int flippedLeds [] = {A4, A3, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4};
int ledBrightness [] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int amountOfLeds = 12;
int maxBrightness = 35;
#define MAXPWM 35

int fan = A5;
int pulseInput = A0, pulseOutput = A1;
int lcdShutter = A2;
int rxLed = 2, txLed = 3;

int aux1 = A6, aux2 = A7;

int stepDelay = 30;
unsigned long nextStep = millis() + stepDelay;
int delayMultiplier = 1;
int transmitDelay = 0;

int intensity = 20, fadeIn = 150, fadeOut = 500, probability = 40;

int burstDuration = 0;
unsigned long burstEnd = millis() + burstDuration;
boolean bursting = false, autoBursting = true;
int autoBurstFrequency = 10000;
int oldProbability = probability;

int fanCount = 0;

int scrollOffset = 0, scrollStep = -1, scrollDelay = 100;
unsigned long nextScroll = millis() + scrollDelay;
boolean scrolling = false;

byte beaconRange = 255, beaconXPos = 127, beaconYPos = 127;
boolean beaconActive = false;

int whiteNoise = 0, blackNoise = 0;

int brightness = 255;

int program = 7;

#define REST 0
#define STARS 1
#define SOLID 2
#define BLINKSOLID 3
#define VU 4
#define MIDDLEVU 5
#define DIRECTIONAL 6
#define RANDOMBITS 7

#define FAN 8
#define SHOWID 9

boolean feedback = false;
boolean conductor = true;

int specificID = -1;

boolean feedbackEEPROM = false;
boolean receivingSpell = false, hasSpell = false;
int spellCounter = 0;

int amountOfPresets = 9;
byte presets [] [17] = {
  {2, 24, 1, 3, 14, 198, 0, 0, 1, 139, 58, 100, 6},       //25
  {5, 5, 1, 54, 89, 236, 160, 133, 132, 128, 170, 0, 0},  //30
  {7, 223, 9, 98, 65, 244, 75, 4, 5, 125, 204, 0, 5},     //35
  {7, 71, 1, 100, 3, 236, 87, 5, 82, 126, 222, 0, 40 },   //40
  {6, 58, 1, 33, 0, 58, 170, 0, 7, 155, 250, 15, 99},     //45
  {6, 216, 1, 100, 77, 40, 167, 5, 82, 192, 182, 3, 10},  //45
  {7, 223, 9, 98, 10, 244, 250, 4, 5, 0, 58, 0, 4},       //45
  {7, 85, 1, 4, 91, 10, 130, 0, 0, 126, 58, 0, 7},        //50
  {6, 58, 1, 54, 89, 58, 170, 133, 132, 155, 250, 2, 9},  //50
  {6, 58, 1, 54, 89, 32, 96, 133, 132, 130, 250, 2, 9},   //55
  {5, 71, 1, 100, 3, 243, 10, 5, 82, 126, 182, 3, 11 },   //60
  {4, 250, 1, 0, 77, 10, 130, 0, 0, 126, 58, 0, 7},       //60
  {5, 247, 1, 54, 89, 8, 15, 133, 132, 138, 199, 0, 32},  //65
  {6, 85, 1, 4, 91, 10, 130, 0, 0, 126, 58, 0, 7},        //70
  {6, 0, 1, 4, 0, 10, 91, 60, 98, 126, 58, 0, 7},         //75
  {4, 195, 1, 96, 100, 84, 0, 2, 0, 128, 25, 0, 2},       //80
  {3, 0, 1, 6, 6, 58, 170, 0, 7, 128, 13, 0, 0}           //90
};

// program specific globals
int vuValue = 0;
int blinkStep = 0;
int directionalLed = 0;
boolean directionalRunning = false;
boolean generative = true;

void setup() {
  SoftPWMBegin();
  for (int i = 0; i < amountOfLeds; i++) {
    pinMode(led[i], OUTPUT);
    SoftPWMSet(led[i], 0);
  }
  SoftPWMSetFadeTime(ALL, fadeIn, fadeOut);

  SoftPWMSet(fan, 0);

  pinMode(lcdShutter, OUTPUT);
  pinMode(pulseOutput, OUTPUT);
  pinMode(rxLed, OUTPUT);
  pinMode(txLed, OUTPUT);

  pinMode(pulseInput, INPUT_PULLUP);
  pinMode(pulseOutput, OUTPUT);

  Serial.begin(115200);

  digitalWrite(rxLed, HIGH);
  digitalWrite(txLed, HIGH);

  for (int i = 0; i < amountOfLeds; i++) {
    digitalWrite(led[i], HIGH);
    delay(50);
    digitalWrite(led[i], LOW);
  }

  digitalWrite(rxLed, LOW);
  digitalWrite(txLed, LOW);

  if (EEPROM.read(EEPROM_INIT) == 'X') {
    id = EEPROM.read(EEPROM_ID);
    byte _flipped = EEPROM.read(EEPROM_FLIPPED);
    byte _turned = EEPROM.read(EEPROM_TURNED);
    xPos = EEPROM.read(EEPROM_XPOS);
    yPos = EEPROM.read(EEPROM_YPOS);

    if (_flipped == 1) {
      flipped = true;
    } else {
      flipped = false;
    }
    if (_turned == 1) {
      turned = true;
    } else {
      turned = false;
    }

    if (feedbackEEPROM) {
      //Serial.println("EEPROM initialized");
      delay(100);
      printEEPROMData();
    }
  } else {
    if (feedbackEEPROM) {
      //Serial.println("no settings in EEPROM");
    }
  }

  if (EEPROM.read(EEPROM_SPELL) == 'X') {
    hasSpell = true;
  }
}

void loop() {

  tick();
  checkPulse();

  if (receivingSpell) receiveSpell();

  if (Serial.available() > 2) {

    digitalWrite(rxLed, HIGH);

    byte startByte = Serial.read();

    if (startByte == 255 || startByte == 254) {

      byte command, value;
      command = Serial.read();
      value = Serial.read();
      parseIncomingSerial(command, value);

      delay(transmitDelay * delayMultiplier);

      if (command == 'R') {
        value++;
        value = value % 250;
      }

      if (startByte == 255) sendData(command, value);
      if (startByte == 254) sendStealthData(command, value);

    } else {
      // doesn't make sense, just pass it along anyways
      digitalWrite(txLed, HIGH);
      Serial.write(startByte);
      digitalWrite(txLed, LOW);
    }
    digitalWrite(rxLed, LOW);
  }

  if (millis() > nextStep) {
    switch (program) {
      case REST:
        // do nothing
        break;
      case STARS:
        stars();
        break;
      case SOLID:
        solid();
        break;
      case BLINKSOLID:
        blinkSolid();
        break;
      case VU:
        vu();
        break;
      case MIDDLEVU:
        middleVu();
        break;
      case DIRECTIONAL:
        directional();
        break;
      case RANDOMBITS:
        randomBits();
        break;
      case FAN:
        runFan();
        break;
      case SHOWID:
        showID();
        break;
    }
    nextStep = millis() + stepDelay * delayMultiplier;
  }

  if (generative) {

    if (ticked && seconds % 75 == 0) {
      //int p = map(analogRead(pot[1]), 0, 1023, 0, amountOfPresets);
      //int r = random(-8,8);
      //p = p + r;
      //p = constrain(p, 0, 16);
      int p = random(amountOfPresets);
      sendPreset(p);
      //Serial.println("generative");
      //Serial.print("preset selected ");
      //Serial.println(p);
    }
  }

  scroll();
  updateBurst();
  updateBeacon();
  noise();
  updateLeds();

}

void noise() {
  if (whiteNoise > 0 || blackNoise > 0) {
    int chance = map(probability, 0, 100, 100, 0);
    int r = random(chance);
    if (r == 0) {
      int l = random(amountOfLeds);

      ledBrightness[l] += random(whiteNoise);
      ledBrightness[l] -= random(blackNoise);
      ledBrightness[l] = constrain(ledBrightness[l], 0, maxBrightness);
      SoftPWMSetFadeTime(led[l], 0, 0);
      SoftPWMSet(led[l], ledBrightness[l]);
      SoftPWMSetFadeTime(led[l], fadeIn, fadeOut);

    }
  }
}

void updateBurst() {
  if (bursting && millis() > burstEnd) {
    bursting = false;
    probability = oldProbability;
  }

  if (autoBursting && millis() > burstEnd + autoBurstFrequency) {
    createBurst();
  }
}

void updateBeacon() {
  if (beaconActive) {
    if (xPos != -1 && yPos != -1) {

      byte d = getDistance(xPos, beaconXPos, yPos, beaconYPos);

      int bright = map(d, 0, beaconRange, MAXPWM, 0);
      bright = constrain(bright, 0, MAXPWM);
      maxBrightness = bright;

    }
  }
}

void receiveSpell() {
  if (Serial.available()) {

    byte b = Serial.read();
    SoftPWMSetFadeTime(ALL, 0, 0);
    solid(b);
    EEPROM.write(EEPROM_SPELL_BEGIN + spellCounter, b);
    spellCounter++;
    if (spellCounter >= 1000) {
      EEPROM.write(EEPROM_SPELL, 'X');
      hasSpell = true;
      receivingSpell = false;
    }
    solid(0);
    SoftPWMSetFadeTime(ALL, fadeIn, fadeOut);
  }
}

void castSpell() {
  for (int i = EEPROM_SPELL_BEGIN; i < EEPROM_SPELL_END; i++) {
    byte b = EEPROM.read(i);
    b = constrain(b, 0, 250);
    Serial.write(b);
    solid(b);
    delay(5);
  }
}

void scroll() {
  if (scrolling) {
    if (millis() > nextScroll) {
      scrollOffset += scrollStep;
      scrollOffset = scrollOffset % amountOfLeds;
      if (scrollOffset < 0) scrollOffset += amountOfLeds;
      nextScroll = millis() + scrollDelay;
    }
  }
}

void updateLeds() {
  for (int i = 0; i < amountOfLeds; i++) {
    int sI = (i + scrollOffset) % amountOfLeds;
    SoftPWMSet(led[sI], map(brightness, 0, 255, 0, ledBrightness[i]));
  }
}

void checkPulse() {
  if (digitalRead(pulseInput) == LOW) {
    digitalWrite(pulseOutput, LOW);
    createBurst();
  } else {
    digitalWrite(pulseOutput, HIGH);
  }
}

void blinkAll(int _times, int _delay) {
  SoftPWMSetFadeTime(ALL, 0, 0);
  for (int i = 0; i < _times; i++) {
    solid(maxBrightness);
    delay(_delay);
    solid(0);
    delay(_delay);
  }
  SoftPWMSetFadeTime(ALL, fadeIn, fadeOut);
}

void showID() {
  if (id == specificID) {
    solid(1);
    scrollOffset = 0;
    ledBrightness[0] = maxBrightness;
    if (turned) ledBrightness[3] = maxBrightness;
  } else {
    solid(0);
  }
  scrolling = false;
}

void runFan() {
  int fanSpeed = map(intensity, 0, 100, 0, 255);
  SoftPWMSet(fan, fanSpeed);
}

void solid() {
  int pwmVal = map(intensity, 0, 100, 0, maxBrightness);
  pwmVal = constrain(pwmVal, 0, maxBrightness);
  for (int i = 0; i < amountOfLeds; i++) {
    ledBrightness[i] = pwmVal;
  }
}

void solid(int v) {
  int pwmVal = constrain(v, 0, maxBrightness);
  for (int i = 0; i < amountOfLeds; i++) {
    ledBrightness[i] = pwmVal;
  }
}

void blinkSolid() {
  blinkStep++;
  blinkStep = blinkStep % 100;
  if (blinkStep % intensity == 0) {
    solid(maxBrightness);
  }
  else {
    solid(0);
  }
}

void randomBits() {
  int chance = map(probability, 0, 100, 100, 0);
  int r = random(chance);
  if (r == 0) {
    for (int i = 0; i < amountOfLeds; i++) {
      int on = random(100);
      if (on < intensity) {
        ledBrightness[i] = maxBrightness;
      } else {
        ledBrightness[i] = 0;
      }
    }
  }
}

void stars() {
  int chance = map(probability, 0, 100, 100, 0);
  int r = random(chance);
  if (r == 0) {
    int l = random(amountOfLeds);
    SoftPWMSet(led[l], maxBrightness);
    delay(5);
    SoftPWMSet(led[l], 0);
  }
}

void middleVu() {
  int chance = map(probability, 0, 100, 100, 0);
  int r = random(chance);
  if (r == 0) {
    int v = random(7) + 1;
    for (int i = 0; i < 7; i++) {
      if (i < v) {
        ledBrightness[6 - i] = maxBrightness;
        ledBrightness[6 + i] = maxBrightness;
      } else {
        ledBrightness[6 - i] = 0;
        ledBrightness[6 + i] = 0;
      }
    }
  }
}

void vu() {
  int chance = map(probability, 0, 100, 100, 0);
  int r = random(chance);
  if (r == 0) {
    int v = random(14);
    for (int i = 0; i < amountOfLeds; i++) {
      if (i < v) {
        ledBrightness[i] = maxBrightness;
      } else {
        ledBrightness[i] = 0;
      }
    }
  }
}

void directional() {
  if (!directionalRunning) {
    int chance = map(probability, 0, 100, 100, 0);
    int r = random(chance);
    if (r == 0) {
      directionalRunning = true;
      directionalLed = 0;
    }
  }

  if (directionalRunning) {

    for (int i = 0; i < amountOfLeds; i++) {
      if (i == directionalLed) {
        ledBrightness[i] = maxBrightness;
      } else {
        ledBrightness[i] = 0;
      }
    }

    directionalLed ++;

    if (directionalLed > amountOfLeds + 1) {
      directionalRunning = false;
      solid(0);
    }
  }
}

void parseIncomingSerial(char command, byte value) {

  // set trainsmit delay
  if (command == 'T') {
    transmitDelay = value;
  }

  // set delay multiplier
  if (command == 'M') {
    delayMultiplier = value;
  }

  // set program
  if (command == 'P') {
    program = value;
  }

  // dump preset
  if (command == 'p') {
    dumpPreset();
  }

  // set stepDelay
  if (command == 'D') {
    stepDelay = value;
  }

  // set stepDelay relative to id
  if (command == 'd') {
    if (id == -1) {
      stepDelay = map(10, 0, 250, value / 10, value);
    } else {
      stepDelay = map(id, 0, 250, value / 10, value);
    }
  }

  // set intensity
  if (command == 'V') {
    intensity = constrain(value, 0, 100);
  }

  // set fadeIn time
  if (command == 'I') {
    fadeIn = map(value, 0, 250, 0, 4000);
    SoftPWMSetFadeTime(ALL, fadeIn, fadeOut);
  }

  // set fadeOut time
  if (command == 'O') {
    fadeOut = map(value, 0, 250, 0, 4000);
    SoftPWMSetFadeTime(ALL, fadeIn, fadeOut);
  }

  // show specific ID
  if (command == 'i') {
    specificID = value;
    program = SHOWID;
    printEEPROMData();
  }

  // reset ID
  if (command == 'R') {
    SoftPWMSetFadeTime(ALL, 0, 0);
    solid(maxBrightness);
    randomSeed(value);
    id = value;
    EEPROM.update(EEPROM_ID, id);
    EEPROM.update(EEPROM_INIT, 'X');
    solid(0);
    SoftPWMSetFadeTime(ALL, fadeIn, fadeOut);
  }

  // hard freeze
  if (command == 'F') {
    delay(random(value * 100));
  }

  // set probability
  if (command == 'C') {
    probability = constrain(value, 0, 100);
  }

  // set white noise
  if (command == 'N') {
    whiteNoise = value;
  }

  // set black noise
  if (command == 'n') {
    blackNoise = value;
  }

  // burst
  if (command == 'B') {
    createBurst(value);
  }

  // set beacon active
  if (command == 'b') {
    if (value == 1) {
      beaconActive = true;
    } else {
      beaconActive = false;
    }
  }

  // set autoBurst
  if (command == 'A') {
    if (value == 0) {
      autoBursting = false;
    } else {
      autoBursting = true;
      autoBurstFrequency = map(value, 1, 250, 1000, 60000);
    }
  }

  // set scroll step (127 = 0 / no scrolling)
  if (command == 'S') {
    if (value == 127) {
      scrolling = false;
    } else {
      scrolling = true;
      scrollStep = 127 - value;
    }
  }

  // set scrollDelay
  if (command == 's') {
    scrollDelay = value;
  }

  // set beacon XPos
  if (command == 'X' ) {
    beaconXPos = value;
  }

  // set beacon YPos
  if (command == 'Y' ) {
    beaconYPos = value;
  }

  // set beacon range
  if (command == 'l') {
    beaconRange = value;
  }

  // id specific commands
  if (id == specificID) {

    // set xPos
    if (command == 'x') {
      xPos = value;
      EEPROM.update(EEPROM_XPOS, xPos);
      if (feedbackEEPROM) Serial.print("storing x ");
      if (feedbackEEPROM) Serial.println(xPos);
    }

    // set yPos
    if (command == 'y') {
      yPos = value;
      EEPROM.update(EEPROM_YPOS, yPos);
      if (feedbackEEPROM) Serial.print("storing y ");
      if (feedbackEEPROM) Serial.println(yPos);
    }
  }

  // set flipped
  if (command == 'f') {
    if (id == value) {
      flipped = !flipped;
      EEPROM.update(EEPROM_FLIPPED, flipped);
      for (int i = 0; i < amountOfLeds; i++) {
        if (flipped) {
          led[i] = flippedLeds[i];
        } else {
          led[i] = unFlippedLeds[i];
        }
      }
    }
  }

  // set turned
  if (command == 't') {
    if (id == value) {
      turned = !turned;
      EEPROM.update(EEPROM_TURNED, turned);
    }
  }

  // receive spell
  if (command == 'Z') {
    if (id == value) {
      receivingSpell = true;
    }
  }

  // cast spell
  if (command == 'z') {
    if (id == value) {
      castSpell();
    }
  }

}

void createBurst() {
  burstEnd = millis() + burstDuration;
  if (!bursting) {
    oldProbability = probability;
    probability = 100;
    bursting = true;
  }
}

void createBurst(byte dur) {
  burstDuration = map(dur, 0, 250, 0, 10000);
  burstEnd = millis() + burstDuration;
  if (!bursting) {
    oldProbability = probability;
    probability = 100;
    bursting = true;
  }
}

void createRelativeBurst(byte dur) {
  burstDuration = map(dur, 0, 250, 0, 10000);
  burstEnd = millis() + burstDuration;
  if (!bursting) {
    oldProbability = probability;
    probability = constrain(probability * 5, 0, 100);
    bursting = true;
  }
}

void sendData(byte command, byte value) {
  digitalWrite(txLed, HIGH);
  Serial.write(255);
  Serial.write(command);
  Serial.write(value);
  digitalWrite(txLed, LOW);
}
void sendStealthData(byte command, byte value) {
  Serial.write(254);
  Serial.write(command);
  Serial.write(value);
}

void tick() {
  ticked = false;
  if (millis() > nextTick) {
    seconds ++;
    nextTick = millis() + 1000;
    ticked = true;
  }
}

void dumpPreset() {
  Serial.println();
  Serial.println("CLOUDMODULE PRESET");
  Serial.print(program);
  Serial.print(",");
  Serial.print(stepDelay);
  Serial.print(",");
  Serial.print(delayMultiplier);
  Serial.print(",");
  Serial.print(intensity);
  Serial.print(",");
  Serial.print(probability);
  Serial.print(",");
  // reverse scale fadeIn
  Serial.print(map(fadeIn, 0, 4000, 0, 250));
  Serial.print(",");
  // scale from to 0-255
  Serial.print(map(fadeOut, 0, 4000, 0, 250));
  Serial.print(",");
  // reverse scale
  Serial.print(map(burstDuration, 0, 10000, 0, 250));
  Serial.print(",");
  // reverse scale / set to 0 if autoBursting is off
  if (autoBursting) {
    Serial.print(map(autoBurstFrequency, 1000, 60000, 1, 250));
  } else {
    Serial.print(0);
  }
  Serial.print(",");
  // add offset
  Serial.print(127 - scrollStep);
  Serial.print(",");
  Serial.print(scrollDelay);
  Serial.print(",");
  Serial.print(whiteNoise);
  Serial.print(",");
  Serial.println(blackNoise);
  Serial.println();
}

void printEEPROMData () {
  //Serial.print("id: ");
  //Serial.print(id);
  //Serial.print(" p: ");
  //Serial.print(xPos);
  //Serial.print(",");
  //Serial.print(yPos);
  //Serial.print(" f/t: ");
  //Serial.print(byte(flipped));
  //Serial.print("/");
  //Serial.println(byte(turned));
}

byte getDistance(byte x1, byte y1, byte x2, byte y2) {
  int d;
  d = sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
  d = constrain(d, 0, 255);
  return byte(d);
}

void sendPreset(int _thePreset) {
  char presetOrder [] = {'P', 'D', 'M', 'V', 'C', 'I', 'O', 'B', 'A', 'S', 's', 'N', 'n'};
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 13; i++) {
      sendData(presetOrder[i], presets[_thePreset][i]);
      parseIncomingSerial(presetOrder[i], presets[_thePreset][i]);
      delay(5);
    }
    delay(25);
  }
}
