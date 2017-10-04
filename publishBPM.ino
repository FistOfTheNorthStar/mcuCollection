//#include "application.h"
//#include "math.h"

// This #include statement was automatically added by the Particle IDE.
// This is needed for the HTU21D so that it doesn't iterfere with the music level
#include "SparkIntervalTimer/SparkIntervalTimer.h"

// This #include statement was automatically added by the Particle IDE.
#include "HTU21D/HTU21D.h"

//debugging
//if this is defined then serial will work
//#define SERIALout

//SYSTEM_MODE(AUTOMATIC);	//Just to have the blink start immediately
SYSTEM_THREAD(ENABLED);

//adding a recalibration for the mic
unsigned long ONE_DAY_MILLIS = 24 * 60 * 60 * 1000;
unsigned long lastSync = 0; 
//millis() + 12*60*60*1000+ 60*1000*30;

/////////////////TEMP/HUM VARIABLES
//HTU sensor power
byte htupow = A0;
//init the temp sensor
HTU21D myHumidity;
//string to pass
String passData = "NOT YET CALIBRATED";
//temporary humidity & temperature for sensor reset
float temporaryH = 40;
float temporaryT = 5;
//read the hum / tem sensor
unsigned long interval=19000; // the time we need to wait, set the time to 19s because otherwise we'll lose the cloud connection
unsigned long previousMillis=0; // millis() returns an unsigned long.
//this variable is used to set which sensor value is read
byte sensorHorT = 0;

// led when the HTU is being read
bool ledState = false; // state variable for the LED
////////////////////////////////

// these are mic variables
//start timer for
IntervalTimer myTimer;
// another timer for mic calibration
IntervalTimer micCal;

//microphone array to hold the samples
const int maxIterations = 256; // this is the max number of sample iterations there are 40 sample intervals in 1s
//volatile double fftData[maxIterations]; //this is where we will hold the fft data
volatile int sampleIterator = 0; // iterates through the samples
volatile unsigned long oneSample = 0;
//const int maxIntervals = 40; //intervals that are to be checked against the average 40 = 5s
const int maxIntervals = 8; //intervals that are to be checked against the average 8 = 1s
volatile int intervalIterator = 0; // iterates throught the intervals
volatile long sampleSqAdd = 0; //add all the squares of the samples
//volatile long sampleSqAddFFT = 0; //add all the FFT
volatile unsigned long intervalEnergy[maxIntervals]; // store the various FFT intervals here
volatile unsigned long totalEnergy = 0; // add everything together to get the full energy per second
//volatile unsigned long totalEnergyFFT = 0; // add everything together to get the full energy per second
volatile unsigned long average5sPublish = 0; // this is the 5s average sound power to be published
volatile unsigned long tenSecondAve[10];
volatile int tenIterator = 0;
//volatile int numberOfBeats = 0; //rolling count of bpm
//volatile int beatPublish = 0; //bpm that goes out
volatile unsigned long passThisEnergy = 0;
volatile int totalNoise = 0;
volatile int noiseIterator = 0;
int tempThreshold = 0;
float pi = 3.1415927;
float secondsLoop = 0.125 * maxIntervals; //one frequency sample is 8Hz, so we can calculate the average of 
boolean beatDetectionStarted = false; //this checks that the beat is not calculated twice
boolean blockingCheck = false; // this makes sure that when blocking occurs then the false BPM is passed along

byte microphoneCheck = A1; //mic analogue input
byte micDig = A2; //mic digital toggle for 
int sensorReading = 0;      // variable to store the value read from the sensor pin
int threshold = 0; //this is the threshold variable

String testVar1 = "";
String testVar2 = "";

const uint8_t ledPin = D7; //blink
String infotwerk = "e-twerk @ Kertsi: Temperature (C) / Humidity (Rel %) / Volume (dB)";

void setup() {
  
  
  //reset to 0 the ten second ave
  for (int i=0;i<10;i++){
      tenSecondAve[i]=0;
  }
  
  ONE_DAY_MILLIS = 16*60*60*1000 + 40*60*1000;
  
    //led test
    pinMode(ledPin, OUTPUT);
    digitalWriteFast(ledPin, HIGH);
   
    Particle.variable("bileinfo", infotwerk);
    Particle.variable("diskotappi",passData);
    // Particle.variable("test1",testVar1);
    // Particle.variable("test2",testVar2);
    // Particle.variable("torstai", torstai, STRING);

    //initialize the sound energy history buffer
    for (int i = 0; i< maxIntervals; i++){
        intervalEnergy[i]=0;
    }
   
    //set microphoe to low
    pinMode(micDig, OUTPUT);
    digitalWrite(micDig, LOW);
    delay(50);

    pinMode(microphoneCheck, INPUT);

    //calculate what is silent
    int totalNoise = 0;
    int iteratorNoise = 250;
    
    for (int i = 0; i < iteratorNoise; i++ ){
        totalNoise += analogRead(microphoneCheck);
        delay(50);
    }
    
   // testVar1 = (String)totalNoise;
    
    //silence downscale by 10
    threshold = totalNoise / iteratorNoise ;

    //testVar2 = (String)threshold;

    digitalWrite(htupow, HIGH);
    myHumidity.begin();
    //wait for the sensor to start
    delay(1000);

    // this is 8 * 512 samples
    myTimer.begin(getData, 488, uSec);
    micCal.begin(calibrateFunc, 50, hmSec);
    
}

void loop() {

    //sensorReading = analogRead(microphoneCheck);

    //check if 20 seconds have passed and it is time to read the sensor
    unsigned long currentMillis = millis(); // grab current time
    // check if "interval" time has passed
    //Serial.println(currentMillis - previousMillis);
    //read temperature every e.g. 20s and humidity every 20s
    if (currentMillis < previousMillis){
        previousMillis = 0;
    }
    
    if ((unsigned long)(currentMillis - previousMillis) >= interval) {
        blockingCheck = true;
      
        readTempHumd();
         
        //save the "current" time
        previousMillis = millis();
        
    }
    
    //delay(500);
    if (currentMillis < lastSync){
        lastSync = 0;
    }
    

    if ((unsigned long)(currentMillis - lastSync) >= ONE_DAY_MILLIS) {
        // Request time synchronization from the Particle Cloud
        //Particle.syncTime();
        
        
        ONE_DAY_MILLIS = 24 * 60 * 60 * 1000;
        
        //calculate what is silent
       /* 
        int iteratorNoise = 250;
    
        for (int i = 0; i < iteratorNoise; i++ ){
            totalNoise += analogRead(microphoneCheck);
            delay(50);
        }
        */
        lastSync = millis();
        
        threshold = tempThreshold ;
        
    }

   
    String tempTs(temporaryT,2);
    String tempHs(temporaryH,2);
    if (average5sPublish < 65){
        passData = "e-twerk @ kertsi: "+ tempTs +"°C, humidity: "+tempHs + "%, " + "vol: ALLE 70" + " dB";
        //+ (String)(totalEnergy / 1000) ;
    } else if (average5sPublish > 65 && average5sPublish < 100){
	passData = "e-twerk @ kertsi: "+ tempTs +"°C, humidity: "+tempHs + "%, vol: " + String(average5sPublish) + " dB";
	//+(String)(totalEnergy / 1000) ;
    } else {
	passData = "e-twerk @ kertsi: "+ tempTs +"°C, humidity: "+tempHs + "%, vol: " + "YLI 95" + " dB";
	//+ (String)(totalEnergy / 1000) ; 
    }
    //+ ", volume: " + String(average5sPublish) + ", BPM:" + String(beatPublish);
    //passData = " TEST: " + String(outputTester) + " TEST2: " + String(outputTester2) + " no: "+outputter;
    
}


//read the temperature or humidity
void readTempHumd(){
    if (sensorHorT == 0){
        //float temp = 0;
        float temp = myHumidity.readTemperature();
        if (temp != 998){
            temporaryT = temp;
        }else{
            resetHTU();  
        }
        sensorHorT = 1;
    }else{
        //float hum = 0;
        float hum = myHumidity.readHumidity();
        if (hum != 998){
            temporaryH = hum;
        } else {
            resetHTU(); 
        }
        sensorHorT = 0;
    }
}

//reset the temp sensor
void resetHTU(){
    digitalWrite(htupow, LOW);
    delay(1000);
    digitalWrite(htupow, HIGH);
    delay(1000);
}

void blinkBPM(){
    // "toggles" the state, when beat is detected flick the led
    digitalWriteFast(ledPin, !pinReadFast(ledPin)); // sets the LED based on ledState    
}

void getData(){
    
    oneSample = abs(analogRead(microphoneCheck));
    
    //sampleSqAdd += oneSample; //val 3350 is a result of testing in the environment
    sampleSqAdd += abs(oneSample-threshold);
    //sampleSqAddFFT = abs(oneSample-threshold);
    ++sampleIterator;
    
    //average5sPublish = oneSample;

    if(sampleIterator == maxIterations){
        sampleIterator = 0;
        totalEnergy += sampleSqAdd;
        //totalEnergyFFT += sampleSqAddFFT;
        sampleSqAdd = 0;
        //sampleSqAddFFT = 0;
        
        intervalIterator++;
        //every 5s take a reading of the vol levels
        if(intervalIterator == maxIntervals){
            
            tenSecondAve[tenIterator] = totalEnergy ;
            if (tenIterator == 9){
                tenIterator = 0;
                for (int i = 0; i<10; i++){
                    passThisEnergy += tenSecondAve[i]/10;
                    
                }
                passThisEnergy /= 1000;
                if (passThisEnergy < 15){
                    average5sPublish = 60;
                } else if (passThisEnergy  > 15 && passThisEnergy  < 23){
                    average5sPublish = 70;
                } else if (passThisEnergy > 23 && passThisEnergy  < 45){
                    average5sPublish = 80;
                } else if (passThisEnergy  > 45 && passThisEnergy  < 73){
                    average5sPublish = 90;
                } else if (passThisEnergy  > 73 ){
                    average5sPublish = 100;
                }
                passThisEnergy = 0;
            } else {
                tenIterator++;
                
            }
            
            intervalIterator = 0;
            totalEnergy = 0;
            
            /*
            if (totalEnergy / 1000 < 15){
                average5sPublish = 60;
            } else if (totalEnergy / 1000 > 15 && totalEnergy / 1000 < 23){
                average5sPublish = 70;
            } else if (totalEnergy / 1000> 23 && totalEnergy / 1000 < 45){
                average5sPublish = 80;
            } else if (totalEnergy / 1000 > 45 && totalEnergy / 1000 < 73){
                average5sPublish = 90;
            } else if (totalEnergy / 1000 > 73 ){
                average5sPublish = 100;
            } 
            */
            
               
        }
    }
    
    
    
    //add the squares of the samples
    unsigned long oneSample = abs(analogRead(microphoneCheck)-threshold);
    sampleSqAdd += sq(oneSample);
    fftData[sampleIterator] = (double)oneSample;
   
    ++sampleIterator;

    if(sampleIterator == maxIterations){ //we've hit the end of one interval and it is time to add everything together
        sampleIterator = 0; //reset the iterator
        totalEnergy += sampleSqAdd /100; //add to total energy for the sound level
        
        double midFFTholderR; //REAL
        double midFFTholderI; //IMAGINARY
        double loopEnergyFFT;
        //we need to calculate the FFT, we use the Cooley-Tukey transform 80 - 256 Hz, and calculate the energy for that!
        for (int j = 5; j < 16 ; j++){
            for (int i = 0; i < maxIterations / 2; i+=2 ){
                midFFTholderR = fftData[i]*cos(j*i*pi/64)+fftData[i+1]*cos((1+2*i)*j*pi/128);
                midFFTholderI = fftData[i]*sin(j*i*pi/64)+fftData[i+1]*sin((1+2*i)*j*pi/128);
            }
            loopEnergyFFT += sqf(midFFTholderR) + sqf(midFFTholderI); //FFT energy addition
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
        //    outputter = "YEES";
        } else {
            //to remove the possibility that the beat is calculated twice, we have to wait until the energy drops below the threshold
            if (beatDetectionStarted){
                numberOfBeats += 1;
                blinkBPM();
        //        outputter = "HAPPENED";
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

void calibrateFunc(){
    
    if(noiseIterator < 256){
        totalNoise += oneSample;
    } else {
        tempThreshold = totalNoise / 256;
        totalNoise = 0;
        noiseIterator = 0;
    }
 
    noiseIterator++;    
}

unsigned long sq(long product){
    return product * product;
}

double sqf(double product){
    return product * product;
}
