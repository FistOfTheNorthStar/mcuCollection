// Do not remove the include below
#include "nanoPinPad.h"
//timer to reset the pins
#include <MsTimer2.h>
//keypad library
#include <Keypad.h>
//add EEPROM
#include <EEPROM.h>

//memory testing
//#define MEMTEST

#ifdef MEMTEST
	#include <MemoryFree.h>
#endif

//time to block misuse
#define DELAYtime

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
byte keys[ROWS][COLS] = {
  {1,2,3},
  {4,5,6},
  {7,8,9},
  {240,0,240}};
//this case 1,2,3,4 CHECK THIS
byte rowPins[ROWS] = {4, 5, 6, 7}; //connect to the row pinouts of the keypad, 1st row, 2nd, 3rd, 4th
//1,2,3 CHECK THIS
byte colPins[COLS] = {8, 9, 10}; //connect to the column pinouts of the keypad, 1st, 2nd, 3rd

//keypad instance
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//led pins
#define ledPlus 11
#define door 12

//this variable is needed for to check how many times the buttons have been pressed
byte pinNoState=0;

//testing variable the number of tries PIN has been entered, if wrong then wait 5s
byte testingTries = 0;

//the byte array to hold the pressed keys
byte pinpass[] = {0,0,0,0,0};

//read the number of pins
byte noPins=5;

//this holds the PIN from the memory
byte pinpassCOMP[5];
//{0,0,0,0,0};

//changing PIN
byte changingPIN = 0;

//changing the long changing code
byte changingCODE = 0;

//this is to hold the change code to be put into the EEPROM
byte changeCode[] ={240,240,240,240,240,240,240,240,240,240,240,240,240,240,240};

//output variables to SERIAL
//#define SERIALTEST

//if the keypad has a red light
//#define RED

//pin change state
boolean pinChange=false;

//long pin change state
boolean longPinChange=false;

//uncomment to reset the EEPROM!
//#define EEPROMreset

void resetPINs(){

	//stop the timer to reset the pins
	MsTimer2::stop();

	#ifdef SERIALTEST
		Serial.print("reset pins");
	#endif

	testingTries++;

	#ifdef SERIALTEST
		Serial.print(testingTries);
	#endif

	if (testingTries==5){
        // if user tries 3 pins then wait 5s
        //could enter red light here if there is such

		#ifdef DELAYtime
			delay(5000);
		#endif

     } else if(testingTries==10){

		#ifdef DELAYtime
    	 	delay(15000);
		#endif

     } else if (testingTries==20){

		#ifdef DELAYtime
    	 	delay(150000);
		#endif
    	testingTries=0;
     }

	//this resets the state so that new Pin set can be given
	pinNoState=0;
	//set the changing pin to zero to begin again
	changingPIN = 0;
	//set the changing code to ZERO
	changingCODE = 0;
	//pin change state
	pinChange=false;
	//long pin state
	longPinChange=false;

	#ifdef MEMTEST
		Serial.print("freeMemory()=");
		Serial.println(freeMemory());
	#endif

}

//testing function
void keyTest(byte pinpass[]){
	//testing keys function
	//stop the timer
	MsTimer2::stop();

    //if we reach the number of pins expected then check
    if (pinNoState==5){

    	//this resets the state so that new Pin set can be given
        pinNoState=0;

        //testing boolean
        boolean testCorrect=false;
        //boolean no door open
        boolean noDoor = false;


        //test the regular pin
        if(pinpass[0]!=240){

        	//go through the various pins
        	for (byte i = 0; i<5;i++){
        		if(pinpass[i]==pinpassCOMP[i] && changingPIN==0){

        			testCorrect = true;
        		//test if there is pin change in progress
        		}else if(pinChange &&  pinpass[i]!=240){
        			testCorrect = true;
        			noDoor = true;

        		}else{
					testCorrect = false;
        			break;
        		}
        	}

        	//open door if correct
        	if(testCorrect){
        		//open door only if not changing the pin
        		if(!noDoor){
        			digitalWrite(door,HIGH);
        			delay(200);
        			digitalWrite(door,LOW);
					#ifdef SERIALTEST
        				Serial.println("Door OPEN!");
					#endif
        				//put green led on
        				digitalWrite(ledPlus,HIGH);
        				delay(1000);
        				digitalWrite(ledPlus,LOW);
        		}

        		if(noDoor){
        			//put the code into the EEPROM
        			for(byte j=0;j<5;j++){
        				EEPROM.write(j,pinpass[j]);
						#ifdef SERIALTEST
        					Serial.println(pinpass[j]);
						#endif
        				pinpassCOMP[j]=pinpass[j];
        			}

					#ifdef SERIALTEST
        				Serial.println("PIN CHANGED");
					#endif

        			//put green led on
        			digitalWrite(ledPlus,HIGH);
        			delay(100);
        			digitalWrite(ledPlus,LOW);
        			delay(100);
        			digitalWrite(ledPlus,HIGH);
        			delay(100);
        			digitalWrite(ledPlus,LOW);
        			delay(100);
        		}
        		//set testing tries to 0
        		testingTries=0;

        	}else{
        		testingTries++;
				#ifdef SERIALTEST
        			Serial.print("wrong");
				#endif
				#ifdef RED
        			//put green red on
        			digitalWrite(4,HIGH);
        			delay(1000);
        			digitalWrite(4,LOW);
				#endif
        	}

        	//set the changing pin to zero to begin again
        	changingPIN = 0;
        	//set the changing code to ZERO
        	changingCODE = 0;
        	//pin change state
        	pinChange=false;
        	//long pin state
        	longPinChange=false;

        //test the pin change code
        }else if(pinpass[0]==240 && !pinChange){

        	//reset changingCODE variable to zero
        	changingCODE = 0;
        	//long pin state
        	longPinChange=false;

        	//change the pin test 3 times needed
        	for (byte i = 5+changingPIN*5; i<10+changingPIN*5;i++){
        		#ifdef SERIALTEST
        			Serial.print(pinpass[i-5-changingPIN*5]);
        			Serial.print("#");
        			Serial.println(EEPROM.read(i));
        		#endif
        		//test the change PIN
        		if(EEPROM.read(i)==pinpass[i-5-changingPIN*5]){
        			testCorrect = true;
        		}else{
        			testCorrect = false;
        			break;
        		}
        	}

        	//set the pin
        	if(testCorrect){
        		//set testing tries to 0
        		testingTries=0;
        		//reset testCorrect boolean to false in order to test the functionality
        		testCorrect=false;

        		//add one time more
        		changingPIN++;

        		//check if the changing code has been inputted 3 times
        		if(changingPIN==3){
        			//reset changing pin and set pinchange to true
        			changingPIN=0;
        			pinChange = true;
        		}

        		//put green led on twice
        		digitalWrite(ledPlus,HIGH);
        		delay(500);
        		digitalWrite(ledPlus,LOW);
        		delay(500);
        		digitalWrite(ledPlus,HIGH);
        		delay(500);
        		digitalWrite(ledPlus,LOW);

        	}else{
        	    testingTries++;
        		#ifdef SERIALTEST
        	    	Serial.print("wrong when changing pin");
        		#endif
				#ifdef RED
        	    	//put green red on
        	    	digitalWrite(4,HIGH);
        	    	delay(1000);
        	    	digitalWrite(4,LOW);
				#endif

        	    //set the changing pin to zero to begin again
        	    changingPIN = 0;
        	    //pin change state
        	    pinChange=false;
        	    //long pin state
        	    longPinChange=false;
        	}


        //change the CHANGE CODE
        }else if(pinpass[0]==240 && pinChange ){

        	//reset testing tries
        	testingTries=0;

        	for (byte i = 1+changingCODE*5; i<5+changingCODE*5;i++){
        		//check that the user is not trying to put in 240 nor 240 keys
        	    if (pinpass[i]!=240 ){
        	    	testCorrect=true;
        	    	//put the value into the vector
        	    	changeCode[i]=pinpass[i-changingCODE*5];
					#ifdef SERIALTEST
        					Serial.print(changeCode[i]);
        					Serial.print("x");
        					Serial.print("zzzz");
        					Serial.print(i);
        					Serial.print("wwww");
        			#endif
        	    }else{
        	    	testCorrect=false;
        	    	break;
        	    }
        	}

        	if(testCorrect){
        		//add one more
        		changingCODE++;
        		if(changingCODE==3){
        			//long pin state
        		    longPinChange=true;
        		}

        		if(longPinChange){
        			//long pin state
        			longPinChange=false;
        			//pin change state
        			pinChange=false;
        			//changing code successfully changed
        			changingCODE = 0;
        			//set changing pin to zero
        			changingPIN = 0;
        			for(byte j= 5; j<20;j++){

        				byte insert = changeCode[j-5];
        				//insert the changed number into the system
        				EEPROM.write(j,insert);

						#ifdef SERIALTEST

        					//print all the inserted bytes into the EEPROM
        					if(j==19){

        						Serial.print(EEPROM.read(j));
        					}else{
        						Serial.print(EEPROM.read(j));
        						Serial.print(",");
        					}
        					//empty line
        					Serial.println("");
        				#endif
        			}
        		}

        		//put green led on twice
        		digitalWrite(ledPlus,HIGH);
        		delay(500);
        		digitalWrite(ledPlus,LOW);
        		delay(500);
        		digitalWrite(ledPlus,HIGH);
        		delay(500);
        		digitalWrite(ledPlus,LOW);

        	}else{

        		//set the changing code to ZERO
        		changingCODE = 0;
        		//long pin state
        		longPinChange=false;
        		//pin change state
        		pinChange=false;

        		#ifdef RED
        			//put red on
        			digitalWrite(4,HIGH);
        			delay(1000);
        			digitalWrite(4,LOW);
				#endif
        	}
       	}

    	}//end the given 5 pin

        //error function
        if (testingTries==5){
            // if user tries 5 pins then wait 5s
            //could enter red light here if there is such
			#ifdef DELAYtime
            	delay(5000);
			#endif
            return;
        } else if(testingTries==10){
			#ifdef DELAYtime
            	delay(15000);
			#endif
            return;
       	} else if (testingTries==10){
			#ifdef DELAYtime
       			delay(150000);
			#endif
       		testingTries=0;
       		return;
       	}

		#ifdef MEMTEST
        	Serial.print("freeMemory()=");
        	Serial.println(freeMemory());
		#endif

}


//The setup function is called once at startup of the sketch
void setup()
{
	#ifdef EEPROMreset
		EEPROM.write(20,0);
	#endif

	//serial to test the pins
	#ifdef SERIALTEST
		Serial.begin(9600);
	#endif

	//set the timer to reset the pins to 7s, seems like a good number
	MsTimer2::set(7000, resetPINs);

	//setup the first pin
	if(EEPROM.read(20)==0){
		//write the first setup pins to the EEPROM positions 0-4
		for(byte i=0;i<5;i++){
			EEPROM.write(i,i);
		}
		//set the first #change code, and place # to every beginning
		for(byte i=5;i<20;i++){
			if(i==5 || i==10 || i==15){
				EEPROM.write(i,240);
			}else{
				EEPROM.write(i,1);
			}
		}
		//add one to EEPROM position 20, so that the setup is complete
		EEPROM.write(20,1);
		#ifdef SERIALTEST
			for(byte i=0;i<21;i++){
				Serial.print(EEPROM.read(i));
				Serial.print(",");
			}
		#endif
	}

	//get the pin to be compared to
	for(byte i=0;i<5;i++){
		pinpassCOMP[i]=EEPROM.read(i);
	}



	//setup the output pins
	//door open pin
	pinMode(door, OUTPUT);
	//led green
	pinMode(ledPlus, OUTPUT);

#ifdef RED
	//led red
	pinMode(4, OUTPUT);
#endif

}

// The loop function is called in an endless loop
void loop()
{
	//this is the keypad part
	byte key = keypad.getKey();
	delay(20);

	if (key != NO_KEY){
		#ifdef SERIALTEST
		 	  Serial.print(key);
		 #endif
		 if (pinNoState==0){
			 //start the timer
			 MsTimer2::start();
		 }


		 //check the number of times the key has been pressed
		 pinpass[pinNoState]=key;
		 pinNoState++;

		 if(pinNoState==5){
			keyTest(pinpass);

			#ifdef MEMTEST
			 	 Serial.print("freeMemory()=");
			 	 Serial.println(freeMemory());
			#endif

		 }

	}
}
