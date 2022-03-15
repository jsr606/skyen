HELP / PROTOCOL

COMMANDS

all commands consist of 2 bytes: one character + one number
first byte sets the command
second byte sets the value for that command
all numbers are between 0-250

ie. command "P0": sets program to 0
command "D100": sets stepDelay to 100ms

when a command is successfully parsed it appears below the text input field.

META COMMANDS

several commands can be nested and executed from one line of code using META COMMANDS

ie. entering the line
> R10 (L3 W2 L4 W5)
will execute the command "L3 W2 L4 W5" 10 times (R10 (X) = REPEAT X 10 TIMES)
L3 will execute line 3 stored below the input field
W2 will wait 2 seconds
L4 will execute line 4 stored below the input field
W5 will wait 5 seconds

META COMMANDS

'L'
	execute line X

'W'
	wait X seconds

COMMANDS

'T'
    transmitDelay = value;
   
'M'
	delayMultiplier = value;
    
'P'
	program = value;

	available programs

	REST 0
	STARS 1
	SOLID 3
	BLINKSOLID 4
	VU 5 	
	MIDDLEVU 6
	DIRECTIONAL 7
	RANDOMBITS 8
   
'D'
	stepDelay = value;

'V'
	intensity = constrain(value, 0, 100);

'I'
	fadeIn = map(value, 0, 250, 0, 4000);
    
'i'
	specificID = value;
    program = SHOWID;

'O'
	fadeOut = map(value, 0, 250, 0, 4000);
    
'F'
	delay(random(value * 100));
 
'C'
    probability = constrain(value, 0, 100);
  
'B'
	createBurst(value);
  
'b'
	if (value == 1) {
      beaconActive = true;
    } else {
      beaconActive = false;
    }
 
'A'
	if (value == 0) {
      autoBursting = false;
    } else {
      autoBursting = true;
      autoBurstFrequency = map(value, 1, 250, 1000, 60000);
    }
  
'S'
	if (value == 127) {
      scrolling = false;
    } else {
      scrolling = true;
      scrollStep = 127 - value;
    }
  
's'
	scrollDelay = value;
    
'X'
	beaconXPos = value;
  
'Y'
	beaconYPos = value;
  
'l'
	beaconRange = value;
