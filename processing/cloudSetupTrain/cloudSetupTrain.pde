import processing.serial.*;
import controlP5.*;
import processing.video.*;
import gab.opencv.*;
OpenCV opencv;
PImage  before, after;

Capture cam;
ControlP5 cp5;
String serialPort = "/dev/ttyACM3";
Serial serial;
int id;

int videoX = 10, videoY = 100, videoWidth = 800, videoHeight= 448;

boolean settingPosition = false;
int scrollStep = 0;
boolean scroll = false;
int scrollDelay = 100;
int stepDelay = 130, delayMultiplier = 1, fadeIn = 10, fadeOut = 200;

int beaconX = 125, beaconY = 125, beaconRange = 125;
boolean settingBeacon = false, beaconActive = false;

float lfoX = 0.23, lfoY = 0.32, lfoAccX = 0, lfoAccY = 0;

int whiteNoise = 0, blackNoise = 0;

PVector loc;

String[] libraryOfBabel;
ArrayList spell = new ArrayList();

void setup() {
  size(820, 680);
  printArray(Serial.list());
  serial = new Serial(this, serialPort, 115200);

  String[] cameras = Capture.list();
  println("Available cameras:");
  for (int i = 0; i < cameras.length; i++) {
    println(cameras[i]);
  }
  cam = new Capture(this, videoWidth, videoHeight, "Video Capture 5");
  opencv = new OpenCV(this, videoWidth, videoHeight);
  opencv.loadCascade(OpenCV.CASCADE_FRONTALFACE);
  cam.start();

  cp5 = new ControlP5(this);
  cp5.addSlider("id").setRange(0, 250).setPosition(10, 10).setWidth(560);
  cp5.addSlider("stepDelay").setRange(0, 250).setPosition(10, 560).setWidth(560);
  cp5.addSlider("delayMultiplier").setRange(0, 250).setPosition(10, 570).setWidth(560);
  cp5.addSlider("scrollDelay").setRange(0, 250).setPosition(10, 580).setWidth(560);
  cp5.addSlider("fadeIn").setRange(0, 250).setPosition(10, 590).setWidth(560);
  cp5.addSlider("fadeOut").setRange(0, 250).setPosition(10, 600).setWidth(560);
  cp5.addSlider("beaconRange").setRange(0, 250).setPosition(10, 610).setWidth(560);
  cp5.addSlider("lfoX").setRange(0, 5).setPosition(10, 620).setWidth(560);
  cp5.addSlider("lfoY").setRange(0, 5).setPosition(10, 630).setWidth(560);
  cp5.addSlider("whiteNoise").setRange(0, 250).setPosition(10, 640).setWidth(560);
  cp5.addSlider("blackNoise").setRange(0, 250).setPosition(10, 650).setWidth(560);

  cp5.addButton("showID").setSize(70, 15).setPosition(10, 30);
  cp5.addButton("resetID").setSize(70, 15).setPosition(90, 30);
  cp5.addButton("flipped").setSize(70, 15).setPosition(170, 30);
  cp5.addButton("turned").setSize(70, 15).setPosition(250, 30);
  cp5.addButton("setPosition").setSize(70, 15).setPosition(330, 30);

  cp5.addButton("rest").setSize(70, 15).setPosition(10, 50);
  cp5.addButton("stars").setSize(70, 15).setPosition(90, 50);
  cp5.addButton("solid").setSize(70, 15).setPosition(170, 50);
  cp5.addButton("blinkSolid").setSize(70, 15).setPosition(250, 50);
  cp5.addButton("vu").setSize(70, 15).setPosition(330, 50);
  cp5.addButton("middleVu").setSize(70, 15).setPosition(410, 50);
  cp5.addButton("directional").setSize(70, 15).setPosition(490, 50);
  cp5.addButton("randomBits").setSize(70, 15).setPosition(570, 50);

  cp5.addButton("scrolling").setSize(70, 15).setPosition(10, 70);
  cp5.addButton("beacon").setSize(70, 15).setPosition(90, 70);
  cp5.addButton("setBeacon").setSize(70, 15).setPosition(170, 70);
  cp5.addButton("sendText").setSize(70, 15).setPosition(250, 70);
  cp5.addButton("dumpText").setSize(70, 15).setPosition(330, 70);

  libraryOfBabel = loadStrings("The Library of Babel.txt");
  for (int i = 0; i < libraryOfBabel.length; i++) {
    String s = libraryOfBabel[i];
    byte [] allChars = s.getBytes();
    for (int j = 0; j<s.length(); j++) {
      spell.add(char(allChars[j]));
    }
    spell.add(char(10));
  }
}

void draw() {
  background(100);
  //if (cam.available() == true) cam.read();
  opencv.loadImage(cam);
  loc = opencv.max();
  image(cam, videoX, videoY, videoWidth, videoHeight);
  if (settingPosition) {
    fill(255, 200, 200, 150);
    ellipse(videoX+loc.x, videoY+loc.y, 10, 10);
  }
  updateLFOs();
  delay(20);
}

void dumpText() {
  sendData('z', id);
}


void sendText() {
  println(spell.size() + " total characters in this spell");
  sendData('Z', id);
  serial.write(253);
  int startChar = id*1000;
  int endChar = id*1000+1000;
  endChar = min(endChar, spell.size());
  for (int i = startChar; i < endChar; i++) {
    int c = int((char) spell.get(i));
    c = constrain(c, 0, 250);
    serial.write((char) c);
    print((char) c);
    delay(5);
  }
  delay(50);
  serial.write(252);
  println();
  println("spell cast from "+startChar+" to "+endChar+" to id "+id);
}


void updateLFOs () {
  if (beaconActive) {
    lfoAccX += lfoX;
    lfoAccY += lfoY;

    float _x = sin(radians(lfoAccX));
    float _y = cos(radians(lfoAccY));
    _x = map(_x, -1, 1, videoX, videoX+videoWidth);
    _y = map(_y, -1, 1, videoY, videoY+videoHeight);
    rectMode(CENTER);
    noStroke();
    fill(255, 50);
    ellipseMode(CENTER);
    float _w = map(beaconRange, 0, 250, 0, videoWidth);
    float _h = map(beaconRange, 0, 250, 0, videoHeight);
    ellipse(_x, _y, _w, _h);

    if (frameCount % 5 == 0) {
      int posX = int(map(_x, videoX, videoX+videoWidth, 0, 250));
      int posY = int(map(_y, videoY, videoY+videoHeight, 0, 250));
      sendData('X', posX);
      sendData('Y', posY);
    }
  }
}

void captureEvent(Capture c) {
  c.read();
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

void program(int _p) {
  sendData('P', _p);
}

void showID() {
  sendData('i', id);
}

void resetID () {
  sendData('R', 0);
}

void flipped() {
  sendData('f', id);
}

void turned() {
  sendData('t', id);
}

void controlEvent(ControlEvent theEvent) {
  color c;
  switch(theEvent.getController().getName()) {
    case ("stepDelay"):
    sendData('D', byte(stepDelay));
    println("delay "+stepDelay);
    break;
    case ("delayMultiplier"):
    sendData('M', byte(delayMultiplier));
    println("delay multiplier "+delayMultiplier);
    break;
    case ("scrollDelay"):
    sendData('s', byte(scrollDelay));
    println("scroll delay "+scrollDelay);
    break;
    case ("fadeIn"):
    sendData('I', byte(fadeIn));
    println("fade in "+fadeIn);
    break;
    case ("fadeOut"):
    sendData('O', byte(fadeOut));
    println("fade out "+fadeOut);
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
    case ("beaconRange"):
    sendData('l', beaconRange);
    break;
    case ("whiteNoise"):
    sendData('N', whiteNoise);
    break;
    case ("blackNoise"):
    sendData('n', blackNoise);
    break;
  }
}

void scrolling() {
  scroll = !scroll;
  if (scroll) {
    sendData('S', 127+scrollStep);
  } else {
    sendData('S', 127);
  }
  color c = color(150+int(scroll)*100, 100, 100);
  cp5.getController("scrolling").setColorBackground(c);
}

void keyPressed() {
  if (key == 't') turned();
  if (key == 'f') flipped();
  if (key == '+') {
    scrollStep++;
    scrollStep = constrain(scrollStep, -11, 11);
    sendData('S', scrollStep+127);
    println("scrollstep "+scrollStep);
  }
  if (key == '-') {
    scrollStep--;
    scrollStep = constrain(scrollStep, -11, 11);
    sendData('S', scrollStep+127);
    println("scrollstep "+scrollStep);
  }
  if (key == CODED) {
    if (keyCode == UP) {
      id++;
      id = id % 250;
      cp5.getController("id").setValue(id);
      showID();
    }
    if (keyCode == DOWN) {
      id--;
      if (id < 0) id = 250;
      cp5.getController("id").setValue(id);
      showID();
    }
    if (keyCode == ENTER && settingPosition) {
      int _x = int(loc.x);
      int _y = int(loc.y);
      sendData('x', _x);
      sendData('y', _y);
      println("set location "+_x+","+_y);
    }
  }
}

void mousePressed() {
  if (mouseX > videoX && mouseX < videoX+videoWidth && mouseY > videoY && mouseY < videoY+videoHeight) {
    int posX = int(map(mouseX, videoX, videoX+videoWidth, 0, 250));
    posX = constrain(posX, 0, 250);
    int posY = int(map(mouseY, videoY, videoY+videoHeight, 0, 250));
    posY = constrain(posY, 0, 250);

    if (settingPosition) {
      sendData('x', posX);
      sendData('y', posY);
      settingPosition = false;
      color c = color(150+int(settingPosition)*100, 100, 100);
      cp5.getController("setPosition").setColorBackground(c);
    }

    if (settingBeacon) {
      sendData('X', posX);
      sendData('Y', posY);
    }
  }
}
