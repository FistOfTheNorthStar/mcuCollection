#include <TimerOne.h>
#include "math.h"

//debugging
//if this is defined then serial will work
#define SERIALout

String passData = "NOT YET CALIBRATED";

unsigned long interval=19000; // the time we need to wait, set the time to 19s because otherwise we'll lose the cloud connection
unsigned long previousMillis=0; // millis() returns an unsigned long.

//microphone array to hold the samples
const int maxIterations = 512; // this is the max number of sample iterations there are 40 sample intervals in 1s
volatile int fftData[maxIterations]; //this is where we will hold the fft data
volatile int sampleIterator = 0; // iterates through the samples
const int maxIntervals = 40; //intervals that are to be checked against the average 40 = 5s
volatile int intervalIterator = 0; // iterates throught the intervals
volatile long sampleSqAdd = 0; //add all the squares of the samples
volatile unsigned long intervalEnergy[maxIntervals]; // store the various FFT intervals here
volatile unsigned long totalEnergy = 0; // add everything together to get the full energy per second
volatile unsigned long average5sPublish = 0; // this is the 5s average sound power to be published
volatile int numberOfBeats = 0; //rolling count of bpm
volatile int beatPublish = 0; //bpm that goes out
float secondsLoop = 0.125 * maxIntervals; //one frequency sample is 8Hz, so we can calculate the average of 
boolean beatDetectionStarted = false; //this checks that the beat is not calculated twice
boolean blockingCheck = false; // this makes sure that when blocking occurs then the false BPM is passed along
float pi = 3.141;

//testing
#define testingOutput
#ifdef testingOutput
    volatile unsigned long outputTester = 0;
    volatile unsigned long outputTester2 = 0;
    String outputter = "";
#endif

//two different wifi networks
#define testHomeWifi
//#define setKertsiWifi

byte microphoneCheck = A1; //mic analogue input
byte micDig = A2; //mic digital toggle for 
int sensorReading = 0;      // variable to store the value read from the sensor pin
int threshold = 0; //this is the threshold variable

const uint8_t ledPin = 13; //blink

void setup() {
    
    // this is 4 * 512 samples
    Timer1.initialize(844);
    Timer1.attachInterrupt(getData); // blinkLED to run every 0.15 seconds
    
    //initialize the sound energy history buffer
    for (int i = 0; i< maxIntervals; i++){
        intervalEnergy[i]=0;
    }
    
    //led test
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
    
    //set microphoe to low
    pinMode(micDig, OUTPUT);
    digitalWrite(micDig, LOW);
    delay(50);

    //pinMode(microphoneCheck, INPUT);

    //calculate what is silent
    int totalNoise = 0;
    int iteratorNoise = 0;
    unsigned long timeToTest = millis() + 6600;
    while (millis() < timeToTest){
        totalNoise += analogRead(microphoneCheck);
        iteratorNoise++;
        delay(20);
    }
    
    //silence downscale by 10
    threshold = totalNoise / iteratorNoise ;

    delay(1000);

  #ifdef SERIALout
    Serial.begin(57600);
    Serial.println("Threshold value in calibration");
      Serial.println(threshold);
  #endif
  
}

void loop() {

    //sensorReading = analogRead(microphoneCheck);


    
    //check if 20 seconds have passed and it is time to read the sensor
    unsigned long currentMillis = millis(); // grab current time
    // check if "interval" time has passed
    //Serial.println(currentMillis - previousMillis);
    //read temperature every e.g. 20s and humidity every 20s
     

  #ifdef SERIALout    

    Serial.println(passData);
  #endif    
  
    //passData = "temperature: "+String(temporaryT)+", humidity: "+String(temporaryH) + ", volume: " + String(average5sPublish) + ", BPM:" + String(beatPublish);
    passData = " TEST: " + String(outputTester) + " TEST2: " + String(outputTester2) + " no: "+outputter;
    
}


void blinkBPM(){
    // "toggles" the state, when beat is detected flick the led
    digitalWrite(ledPin, !digitalRead(ledPin)); // sets the LED based on ledState    
}

void getData(){
   
    //add the squares of the samples
    unsigned long oneSample = abs(analogRead(microphoneCheck)-threshold);
    sampleSqAdd += sq(oneSample);
    fftData[sampleIterator] = oneSample;
    ++sampleIterator;

    if(sampleIterator == maxIterations){ //we've hit the end of one interval and it is time to add everything together
        sampleIterator = 0; //reset the iterator
        totalEnergy += sampleSqAdd /100; //add to total energy for the sound level
        
        double midFFTholderR; //REAL
        double midFFTholderI; //IMAGINARY
        double loopEnergyFFT;
        //we need to calculate the FFT, we use the Cooley-Tukey transform 80 - 256 Hz, and calculate the energy for that!
        for (int j = 10; j < 32 ; j++){
            for (int i = 0; i < maxIterations / 2; i+=2 ){
                midFFTholderR = fftData[i]*cos(j*i*pi/128)+fftData[i+1]*cos((1+2*i)*j*pi/256);
                midFFTholderI = fftData[i]*sin(j*i*pi/128)+fftData[i+1]*sin((1+2*i)*j*pi/256);
            }
             
            loopEnergyFFT += sq(midFFTholderR) + sq(midFFTholderI); //FFT energy addition
            
        }
        
        //outputTester= (long)loopEnergyFFT;
        //check for the beat if one add it!

        intervalEnergy[intervalIterator] = loopEnergyFFT; // add it to the energy history buffer
        //time to calculate the total energy from the buffer, which is 0 in the beginning
        double averageLocalEnergy = 0;
        for (int i = 0; i< maxIntervals; i++){
            averageLocalEnergy += intervalEnergy[i] / maxIntervals;
        }
    
        /* WE'RE NOT USING VARIANCE IN V 1.0
        double variance = 0; // this is the variance needed for our device
        //calculate variance
        for (int i = 0; i<maxIntervals; i++){
            variance += sq((averageLocalEnergy - intervalEnergy[i])/1000) / maxIntervals;
        }
        
        
        //use linear regression model [from a paper], as we used almost half the samples just multiply by 2
        double C = max(1.8142857 - 0.0025714 * variance / 3, 1.5);
        outputTester = numberOfBeats;
        */
        
        double c = 1.5; //we will try this for the treshold
      
        //outputTester2= (unsigned long)( c * averageLocalEnergy);
        //check if the beat goes above the threshold
        if (loopEnergyFFT > c * averageLocalEnergy){
            beatDetectionStarted = true;
            outputter = "YEES";
        } else {
            //to remove the possibility that the beat is calculated twice, we have to wait until the energy drops below the threshold
            if (beatDetectionStarted){
                numberOfBeats += 1;
                blinkBPM();
                outputter = "HAPPENED";
            }
            beatDetectionStarted = false; 
            //outputter = "NOO";
        }
        
        sampleSqAdd = 0; // reset samples to next interval
        
        //iterate to the next interval
        intervalIterator++;
        if(intervalIterator == maxIntervals){ // check if the max number of intervals has been reached
            
            //reset interval
            intervalIterator = 0;
            
            if(blockingCheck){
                blockingCheck = false;
            } else {
                beatPublish = (int)(numberOfBeats * 60 / secondsLoop) ; // this is the beats, might need to do FFT, but that can be updated
            }
            numberOfBeats = 0; // reset beats counter
            
            
            //sound level calculation per second
            average5sPublish = totalEnergy / secondsLoop;
            totalEnergy = 0;
            
            
           // outputTester2= (unsigned long)average5sPublish;
            outputTester = (unsigned long)secondsLoop;
            
        }
    }
}

