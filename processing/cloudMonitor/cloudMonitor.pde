import processing.serial.*;

PrintWriter output;

Serial incomingSerial;
String incomingMessage;
int incomingByte [] = {0, 0, 0};
String serialPort = "/dev/ttyUSB1";
ArrayList feedbackMessages;
int amountOfFeedbackMessages = 20;
PFont mono;

int textSize = 12;

String [] commands = {"REST", "STARS", "SOLID", "BLINKSOLID", "VU", "MIDDLEVU", "DIRECTIONAL", "RANDOMBITS", "FAN", "PULSETEST", "AUXTEST", "SHOWID"};

void setup() {
  size(400, 700);
  feedbackMessages = new ArrayList();
  printArray(Serial.list());
  incomingSerial = new Serial(this, serialPort, 115200);
  incomingSerial.buffer(3);

  mono = loadFont("PTMono-Regular-12.vlw");
  textFont(mono, textSize);

  output = createWriter("presets.txt"); 
}

void draw() {
  background(0);
  fill(255);
  textFeedback(0, 100);
}

void serialEvent(Serial myPort) {
  try {
    for (int i = 0; i<3; i++) {
      incomingByte[i] = int(myPort.read());
      char c = char(incomingByte[i]); 
      print(c);
      output.print(c);
      if (i == 0) {
        if (incomingByte[0] != 255) {
          return;
        }
      }
    }
  }
  catch(RuntimeException e) {
    println("argh");
    e.printStackTrace();
  }
  parseIncomingSerial();
}

void parseIncomingSerial() {
  incomingMessage = "";
  for (int i = 0; i<3; i++) {
    //print(incomingByte[i]);
    //print(" ");
    incomingMessage += str(incomingByte[i]);
    incomingMessage += (" ");
  }
  // new program
  if (incomingByte[1] == 80) {
    incomingMessage += "program: ";
    incomingMessage += commands[incomingByte[2]];
  }
 if (incomingByte[1] == 82) {
    incomingMessage += "reset ID: ";
    incomingMessage += incomingByte[2];
  }
 
  feedbackMessages.add(incomingMessage);
  //println();
}


void textFeedback(int x, int y) {
  pushMatrix();
  translate(x, y);
  while(feedbackMessages.size() > amountOfFeedbackMessages) {
    feedbackMessages.remove(0);
  }
  for (int i = 0; i<feedbackMessages.size(); i++) {
    String msg = (String) feedbackMessages.get(i);
    text(msg, 10, 10+textSize*i);
  }
  popMatrix();
}

void keyPressed () {
  if (key == 'c') {
    output.flush();
    output.close(); // Finishes the file
    println("file closed");
  }
}
