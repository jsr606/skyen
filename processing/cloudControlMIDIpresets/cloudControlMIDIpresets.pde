import processing.serial.*;
import controlP5.*;
import themidibus.*;

MidiBus busA;
MidiBus busB;

ControlP5 cp5;
String serialPort = "/dev/ttyACM0";
Serial serial;
int id;

boolean settingPosition = false;
float scrollStep = 0;
boolean scroll = false;
float scrollDelay = 100;
float stepDelay = 130, delayMultiplier = 1, fadeIn = 10, fadeOut = 200;

int beaconX = 125, beaconY = 125, beaconRange = 125;
boolean settingBeacon = false, beaconActive = false;

//float lfo1 = 0.23, lfo2 = 0.32, lfoAcc1 = 0, lfoAcc2 = 0;
int lfoMode1 = 0, lfoMode2 = 0;
float lfoFreq1 = 0.1, lfoFreq2 = 0.5;

int whiteNoise = 0, blackNoise = 0;

PVector loc;

boolean fanRunning = false;
float burstLength = 100, intensity = 20, probability = 40;

boolean feedback = false;
boolean serialActive[] = {true, true, true, true};

LFO lfo1, lfo2;

float program1 = 0, program2 = 7, programChance = 80, chance = 0;
int currentProgram = 0;

boolean noteIncoming = false, sendingProgram = false;
float autoBurst = 100;

int presetToSend = 0;

void setup() {
  size(850, 480);

  printArray(Serial.list());
  serial = new Serial(this, serialPort, 115200);

  MidiBus.list(); //List all available Midi devices. This will show each device's index and name.
  busA = new MidiBus(this, "Cable [hw:1,0,0]", -1, "busA"); //Create a first new MidiBus attached to the IncommingA Midi input device and the OutgoingA Midi output device. We will name it busA.
  busB = new MidiBus(this, "BeatStep [hw:0,0,0]", -1, "busB"); //Create a second new MidiBus attached to the IncommingB Midi input device and the OutgoingB Midi output device. We will name it busB.
  //busB.sendTimestamps(true);

  println();
  println("Inputs");
  println(busA.attachedInputs()); //Print the devices attached as inputs to busA
  println(busB.attachedInputs()); //Print the devices attached as inputs to busA
  println();
  //println("Outputs on busB");
  //println(busB.attachedOutputs()); //Prints the devices attached as outpus to busB

  cp5 = new ControlP5(this);
  cp5.addSlider("stepDelay").setRange(0, 250).setPosition(10, 60).setWidth(560);
  cp5.addSlider("scrollDelay").setRange(0, 250).setPosition(10, 70).setWidth(560);
  cp5.addSlider("scrollStep").setRange(-127, 123).setPosition(10, 80).setWidth(560);

  cp5.addSlider("fadeIn").setRange(0, 250).setPosition(10, 100).setWidth(560);
  cp5.addSlider("fadeOut").setRange(0, 250).setPosition(10, 110).setWidth(560);

  cp5.addSlider("whiteNoise").setRange(0, 100).setPosition(10, 120).setWidth(560);
  cp5.addSlider("blackNoise").setRange(0, 100).setPosition(10, 130).setWidth(560);

  cp5.addSlider("program1").setRange(0, 7).setPosition(10, 150).setWidth(560);
  cp5.addSlider("program2").setRange(0, 7).setPosition(10, 160).setWidth(560);
  cp5.addSlider("programChance").setRange(0, 100).setPosition(10, 170).setWidth(560);
  cp5.addSlider("chance").setRange(0, 100).setPosition(10, 180).setWidth(560);

  cp5.addSlider("burstLength").setRange(0, 250).setPosition(10, 200).setWidth(560);
  cp5.addSlider("autoBurst").setRange(0, 250).setPosition(10, 210).setWidth(560);
  cp5.addSlider("probability").setRange(0, 100).setPosition(10, 220).setWidth(560);
  cp5.addSlider("intensity").setRange(0, 100).setPosition(10, 230).setWidth(560);

  cp5.addSlider("beaconRange").setRange(0, 250).setPosition(10, 250).setWidth(560);
  cp5.addSlider("lfoFreq1").setRange(0, 1).setPosition(10, 260).setWidth(560);
  cp5.addSlider("lfoFreq2").setRange(0, 1).setPosition(10, 270).setWidth(560);

  cp5.addButton("rest").setSize(70, 15).setPosition(10, 10);
  cp5.addButton("stars").setSize(70, 15).setPosition(90, 10);
  cp5.addButton("solid").setSize(70, 15).setPosition(170, 10);
  cp5.addButton("blinkSolid").setSize(70, 15).setPosition(250, 10);
  cp5.addButton("vu").setSize(70, 15).setPosition(330, 10);
  cp5.addButton("middleVu").setSize(70, 15).setPosition(410, 10);
  cp5.addButton("directional").setSize(70, 15).setPosition(490, 10);
  cp5.addButton("randomBits").setSize(70, 15).setPosition(570, 10);
  cp5.addButton("fan").setSize(70, 15).setPosition(650, 10);

  cp5.addButton("scrolling").setSize(70, 15).setPosition(10, 30);
  cp5.addButton("beacon").setSize(70, 15).setPosition(90, 30);
  cp5.addButton("setBeacon").setSize(70, 15).setPosition(170, 30);
  cp5.addButton("midiOut").setSize(70, 15).setPosition(250, 30);
  cp5.addButton("dumpPreset").setSize(70, 15).setPosition(330, 30);
  cp5.addButton("sendPreset").setSize(70, 15).setPosition(410, 30);

  lfo1 = new LFO(2, lfoFreq1);
  lfo2 = new LFO(2, lfoFreq2);
}

void draw() {
  background(100);
  updateLFOs(700, 140);
  drawBeacon(700, 200);
  drawSerialPorts(700, 80);
  drawNoteIn(760, 100);
}

void drawBeacon(float _x, float _y) {
  pushMatrix();
  translate(_x, _y);
  stroke(0);
  rectMode(CORNER);
  noFill();
  rect(0, 0, lfo1.w, lfo1.w);
  rectMode(CENTER);
  fill(255, 100);
  noStroke();
  rect(map(lfo1.output, 0, 1, 0, lfo1.w), map(lfo2.output, 0, 1, 0, lfo1.w), map(beaconRange, 0, 250, 0, lfo1.w), map(beaconRange, 0, 250, 0, lfo1.w));
  popMatrix();
}

void updateLFOs(float _x, float _y) {
  lfo1.draw(_x, _y);
  if (lfo1.newValue == true) sendData ('X', int(map(lfo1.output, 0, 1, 0, 250)));
  lfo2.draw(_x, _y+15);
  if (lfo2.newValue == true) sendData ('Y', int(map(lfo2.output, 0, 1, 0, 250)));
}

class LFO {
  public float output = 0;
  public float accumulator = 0;
  public float increment = 0.1;
  public float limit = 10;
  public int type;
  public float w = 70, h = 10;
  public boolean newValue = false;

  public static final int SINE = 0;
  public static final int RAMP = 1;
  public static final int SAMPLEANDHOLD = 2;

  public LFO(int theType, float theIncrement) {
    type = theType;
    increment = theIncrement;
  }

  public void update() {
    accumulator += increment;
    newValue = false;
    if (accumulator > 1) {
      if (type == SAMPLEANDHOLD) {
        output = random(0, 1);
        newValue = true;
      }
      accumulator = 0;
    }
    switch(type) {
    case SINE:
      output=map(sin(radians(accumulator*360)), -1, 1, 0, 1);
      break;
    case RAMP:
      output=accumulator;
      break;
    case SAMPLEANDHOLD:
      break;
    }
  }

  void draw(float theX, float theY) {
    pushMatrix();
    translate(theX, theY);
    fill(255, 100, 100);
    noStroke();
    rectMode(CORNER);

    rect(0, 0, output*w, h);
    noFill();
    stroke(0);
    rect(0, 0, w, h);
    popMatrix();
  }
}

void drawNoteIn(float xPos, float yPos) {
  pushMatrix();
  translate(xPos, yPos);
  if (noteIncoming) {
    fill(100, 255, 100);
  } else {
    fill(120);
  }
  rectMode(CORNER);
  rect(0, 0, 10, 10);

  translate(-20, 0);
  if (sendingProgram) {
    fill(100, 255, 100);
  } else {
    fill(120);
  }
  rect(0, 0, 10, 10);

  translate(-20, 0);

  fill(0);
  textAlign(CENTER, CENTER);
  text(currentProgram, 5, 3);
  popMatrix();
}


void drawSerialPorts(float xPos, float yPos) {
  pushMatrix();
  translate(xPos, yPos);
  for (int i = 0; i<4; i++) {
    if (serialActive[i]) {
      fill(255, 100, 100);
    } else {
      fill(120);
    }
    rectMode(CORNER);
    rect(0, 0, 10, 10);
    translate(20, 0);
  }
  popMatrix();
}

void updateSerialPort(int thePort) {
  serial.write(251);
  serial.write(thePort);
  if (serialActive[thePort]) {
    serial.write(1);
  } else {
    serial.write(0);
  }
}

void sendData(char _command, int _value) {
  serial.write(255);
  serial.write(byte(_command));
  serial.write(_value);
}

void setPosition() {
  settingPosition = !settingPosition;
  color c = color(150+int(settingPosition)*100, 100, 100);
  cp5.getController("setPosition").setColorBackground(c);
}

void rest() {
  program(0);
}
void stars() {
  program(1);
}
void solid() {
  program(2);
}
void blinkSolid() {
  program(3);
}
void vu() {
  program(4);
}
void middleVu() {
  program(5);
}
void directional() {
  program(6);
}
void randomBits() {
  program(7);
}

void fan() {
  program(8);
}

void program(int _p) {
  sendData('P', _p);
}

void controlEvent(ControlEvent theEvent) {
  color c;
  switch(theEvent.getController().getName()) {
    case ("stepDelay"):
    sendData('D', byte(stepDelay));
    break;
    case ("scrollDelay"):
    sendData('s', byte(scrollDelay));
    break;
    case ("scrollStep"):
    sendData('S', byte(scrollStep)+127);
    break;
    case ("fadeIn"):
    sendData('I', byte(fadeIn));
    break;
    case ("fadeOut"):
    sendData('O', byte(fadeOut));
    break;
    case ("whiteNoise"):
    sendData('N', whiteNoise);
    break;
    case ("blackNoise"):
    sendData('n', blackNoise);
    break;
    case ("burstLength"):
    sendData('B', int(burstLength));
    break;
    case ("autoBurst"):
    sendData('A', int(autoBurst));
    break;
    case ("probability"):
    sendData('C', int(probability));
    break;
    case ("intensity"):
    sendData('V', int(intensity));
    break;
    case ("beaconRange"):
    sendData('l', int(beaconRange));
    break;
    case ("lfoFreq1"):
    lfo1.increment = lfoFreq1;
    break;
    case ("lfoFreq2"):
    lfo2.increment = lfoFreq2;
    break;

    case ("beacon"):
    beaconActive = !beaconActive;
    c = color(150+int(beaconActive)*100, 100, 100);
    cp5.getController("beacon").setColorBackground(c);
    sendData('b', int(beaconActive));
    break;
    case ("setBeacon"):
    settingBeacon = !settingBeacon;
    c = color(150+int(settingBeacon)*100, 100, 100);
    cp5.getController("setBeacon").setColorBackground(c);
    break;
  }
}

void scrolling() {
  scroll = !scroll;
  if (scroll) {
    sendData('S', 127+int(scrollStep));
  } else {
    sendData('S', 127);
  }
  color c = color(150+int(scroll)*100, 100, 100);
  cp5.getController("scrolling").setColorBackground(c);
}

void keyPressed() {
  if (key == '+') {
    scrollStep++;
    scrollStep = constrain(scrollStep, -127, 123);
    sendData('S', int(scrollStep+127));
    cp5.getController("scrollStep").setValue(int(scrollStep));
    println("scrollStep: "+int(scrollStep));
  }
  if (key == '-') {
    scrollStep--;
    scrollStep = constrain(scrollStep, -127, 123);
    sendData('S', int(scrollStep+127));
    cp5.getController("scrollStep").setValue(scrollStep);
    println("scrollStep: "+int(scrollStep));
  }
  if (key == '1') {
    serialActive[0] = !serialActive[0];
    updateSerialPort(0);
  }
  if (key == '2') {
    serialActive[1] = !serialActive[1];
    updateSerialPort(1);
  }
  if (key == '3') {
    serialActive[2] = !serialActive[2];
    updateSerialPort(2);
  }
  if (key == '4') {
    serialActive[3] = !serialActive[3];
    updateSerialPort(3);
  }
}

// notes start from 52
void noteOn(int channel, int pitch, int velocity, long timestamp, String bus_name) {
  /*
  print("Note On:");
   print(" Channel:"+channel);
   print(" Pitch:"+pitch);
   print(" Velocity:"+velocity);
   println("Recieved on Bus:"+bus_name);
   */
  float ran = random(100);
  sendingProgram = false;
  if (ran < chance) {
    ran = random(100);
    if (ran < programChance) {
      currentProgram = (int) program2;
    } else {
      currentProgram = (int) program1;
    }
    program(currentProgram);
    sendingProgram = true;
    // send pulse here in stead??

    sendData('B', int(burstLength));
  }
  noteIncoming = true;
  lfo1.update();
  lfo2.update();
}

void noteOff(int channel, int pitch, int velocity, long timestamp, String bus_name) {
  noteIncoming = false;
}

// CC from 1-16
void controllerChange(int channel, int number, int value, long timestamp, String bus_name) {
  float dVal = 0;
  switch (number) {
  case 1:
    dVal = value-64;
    stepDelay += dVal/5;
    stepDelay = constrain(stepDelay, 0, 250);
    cp5.getController("stepDelay").setValue(stepDelay);
    sendData('D', int(stepDelay));
    break;
  case 2:
    dVal = value-64;
    scrollDelay += dVal/5;
    scrollDelay = constrain(scrollDelay, 0, 250);
    cp5.getController("scrollDelay").setValue(scrollDelay);
    sendData('s', int(scrollDelay));
    break;
  case 3:
    dVal = value-64;
    scrollStep += dVal;
    scrollStep = constrain(scrollStep, -127, 123);
    cp5.getController("scrollStep").setValue(int(scrollStep));
    sendData('S', int(127+scrollStep));
    break;
  case 5:
    dVal = value-64;
    fadeIn += dVal/5;
    fadeIn = constrain(fadeIn, 0, 250);
    cp5.getController("fadeIn").setValue(fadeIn);
    sendData('I', int(fadeIn));
    break;
  case 6:
    dVal = value-64;
    fadeOut += dVal/5;
    fadeOut = constrain(fadeOut, 0, 250);
    cp5.getController("fadeOut").setValue(fadeOut);
    sendData('O', int(fadeOut));
    break;
  case 7:
    dVal = value-64;
    whiteNoise += dVal/5;
    whiteNoise = constrain(whiteNoise, 0, 100);
    cp5.getController("whiteNoise").setValue(whiteNoise);
    sendData('N', int(whiteNoise));
    break;
  case 8:
    dVal = value-64;
    blackNoise += dVal/5;
    blackNoise = constrain(blackNoise, 0, 100);
    cp5.getController("blackNoise").setValue(blackNoise);
    sendData('n', int(blackNoise));
    break;
  case 9:
    dVal = value-64;
    program1 += dVal/5;
    program1 = constrain(program1, 0, 7);
    cp5.getController("program1").setValue(program1);
    break;
  case 10:
    dVal = value-64;
    program2 += dVal/5;
    program2 = constrain(program2, 0, 7);
    cp5.getController("program2").setValue(program2);
    break;
  case 11:
    dVal = value-64;
    programChance += dVal/5;
    programChance = constrain(programChance, 0, 100);
    cp5.getController("programChance").setValue(programChance);
    break;
  case 12:
    dVal = value-64;
    chance += dVal/5;
    chance = constrain(chance, 0, 100);
    cp5.getController("chance").setValue(chance);
    break;
  case 13:
    dVal = value-64;
    burstLength += dVal/5;
    burstLength = constrain(burstLength, 0, 250);
    cp5.getController("burstLength").setValue(burstLength);
    sendData('B', int(burstLength));
    break;
  case 14:
    dVal = value-64;
    autoBurst += dVal/5;
    autoBurst = constrain(autoBurst, 0, 250);
    cp5.getController("autoBurst").setValue(autoBurst);
    sendData('A', int(autoBurst));
    break;
  case 15:
    dVal = value-64;
    probability += dVal/5;
    probability = constrain(probability, 0, 100);
    cp5.getController("probability").setValue(probability);
    sendData('C', int(probability));
    break;
  case 16:
    dVal = value-64;
    intensity += dVal/5;
    intensity = constrain(intensity, 0, 100);
    cp5.getController("intensity").setValue(intensity);
    sendData('V', int(intensity));
    break;
  }
  /*
  print("CC:");
   print(" Channel:"+channel);
   print(" Number:"+number);
   print(" Value:"+value);
   println(" Recieved on Bus:"+bus_name);
   */
}

void dumpPreset() {
  sendData('p', 0);
}

void sendPreset() {
  /*
   0 program
   1 stepDelay
   2 delayMultiplier
   3 intensity
   4 probability
   5 fadeIn
   6 fadeOut
   7 burstDuration
   8 autoBurstFrequency
   9 scrollStep
   10 scrollDelay
   11 whiteNoise
   12 blackNoise
   'P', 'D', 'M', 'V', 'C', 'I', 'O', 'B', 'A', 'S', 's', 'N', 'n'
   */
  //int preset [] = {7, 128, 1, 14, 12, 10, 37, 244, 0, 128, 188, 24, 28};

  //int preset [] = {5,229,1,57,89,8,15,246,0,125,147,2,8};
  
  //int preset [] = {5,203,1,54,89,8,15,133,134,138,60,0,0};
  int preset [] = {5,247,1,54,89,8,15,133,133,138,199,2,0};

  char presetOrder [] = {'P', 'D', 'M', 'V', 'C', 'I', 'O', 'B', 'A', 'S', 's', 'N', 'n'};
  sendData('P',0);
  for (int i = 0; i<14; i++) {
    sendData(presetOrder[i], preset[i]);
    delay(5);
  }
}
