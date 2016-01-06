//=====================================================================================================

#define BRISKET 0
#define BUTT 1
#define PORK_LOIN 0
#define TURKEY_BREAST 0

#if TURKEY_BREAST
#define GRILL_OFF_MEAT_TEMP   160
#define MEAT_FINISHING_TEMP   160
#define INITIAL_GRILL_TEMP    325      // Temp for the first hours of the smoke :)
#define SECONDARY_GRILL_TEMP  325      // Finishing temp of the smoke.
#define HOURS_TRIGGER          48      // Set to a large number (i.e. 48) if you don't want to trigger on time.
#define FOOD_TEMP_TRIGGER     140      //  Food temp to trigger SECONDARY_GRILL_TEMP state
#endif


#if BRISKET
#define GRILL_OFF_MEAT_TEMP   203
#define MEAT_FINISHING_TEMP   203
#define INITIAL_GRILL_TEMP    250      // Temp for the first hours of the smoke :)
#define SECONDARY_GRILL_TEMP  275      // Finishing temp of the smoke.
#define HOURS_TRIGGER          48      // Set to a large number (i.e. 48) if you don't want to trigger on time.
#define FOOD_TEMP_TRIGGER     165      //  Food temp to trigger SECONDARY_GRILL_TEMP state
#endif

#if BUTT
#define GRILL_OFF_MEAT_TEMP   200 
#define MEAT_FINISHING_TEMP   200
#define INITIAL_GRILL_TEMP    235      // Temp for the first hours of the smoke :)
#define SECONDARY_GRILL_TEMP  265      // Finishing temp of the smoke.
#define HOURS_TRIGGER           6      // Set to a large number (i.e. 24) if you don't want a secondary temp.
#define FOOD_TEMP_TRIGGER     165      //  Food temp to trigger SECONDARY_GRILL_TEMP state STALL
#endif



#if PORK_LOIN
#define GRILL_OFF_MEAT_TEMP   138 
#define MEAT_FINISHING_TEMP   145
#define INITIAL_GRILL_TEMP    325      // Temp for the first hours of the smoke :)
#define SECONDARY_GRILL_TEMP  330      // Finishing temp of the smoke.
#define HOURS_TRIGGER          24      // Set to a large number (i.e. 24) if you don't want a secondary temp.
#define FOOD_TEMP_TRIGGER     120      //  Food temp to trigger SECONDARY_GRILL_TEMP state
#endif

//  The Chef does not touch anything below this line
//======================================================================================================

#define FAN_CONTROL_PIN 3       // PWM pin 3

#define GRILL_PROBE_PIN 0        // Analog pin 0  BROKEN CONNECTION - SHIFTED PROBE/PIN #'s
#define FOOD_PROBE_1_PIN 1      // Analog pin 1
#define FOOD_PROBE_2_PIN 2
#define FOOD_PROBE_3_PIN 3

#define PHASE_1_LED_PIN  09
#define PHASE_2_LED_PIN  10
#define PHASE_3_LED_PIN  11
#define PHASE_4_LED_PIN  12
#define DONE_LED_PIN     13


#include "Bridge.h"
#include <YunServer.h>
#include <YunClient.h>
#include <PID_v1.h>

YunServer server;


//Define Variables we'll be connecting to

double TargetGrillTemp=0.0, CurrentGrillTemp=0.0, FanSpeed=0.0;
//Specify the links and initial tuning parameters
// Lots of P not much I and a lot of D
PID myPID(&CurrentGrillTemp, &FanSpeed, &TargetGrillTemp ,25.0,1.0,15.0, DIRECT);

#define FAN_OFF 65.0
#define FAN_HIGH 255.0

void setup() {   

  Bridge.begin();

  //Console.begin();  Auto starts on IDE connect

    //initialize the variables we're linked to
  CurrentGrillTemp = getTemp(GRILL_PROBE_PIN);
  TargetGrillTemp = INITIAL_GRILL_TEMP;

  myPID.SetOutputLimits(FAN_OFF, FAN_HIGH);

  //turn the PID on
  myPID.SetMode(AUTOMATIC);

  
  pinMode(FAN_CONTROL_PIN, OUTPUT);    // init fan pin

  server.listenOnLocalhost();
  server.begin();

}

enum state {
  GRILL_INIT,
  GRILL_PHASE_1,        
  GRILL_PHASE_2,
  GRILL_OFF,
  MEAT_FINISHING,
  FINISHED
};

state MyState = GRILL_INIT;




void loop() 
{

  YunClient client = server.accept();

  String line = "";

  switch (MyState){
    
    case GRILL_INIT:
      setFanDutyCycle(0);                  // turn fan off
      MyState = GRILL_PHASE_1;
      return;
      break;
      
    case GRILL_PHASE_1:
      TargetGrillTemp = INITIAL_GRILL_TEMP;
      
                                              // If we reach the first phase time trigger
      if ( ((float)millis() / (60.0*60.0*1000.0)) >= HOURS_TRIGGER){
        MyState = GRILL_PHASE_2;
        return;
      }
                                              //  If we reach the first phase meat temp trigger
      if ( (getTemp(FOOD_PROBE_1_PIN) >= FOOD_TEMP_TRIGGER) && (getTemp(FOOD_PROBE_2_PIN) >= FOOD_TEMP_TRIGGER) ){
        MyState = GRILL_PHASE_2;
        return;
      }
      break;
      
    case GRILL_PHASE_2:
      TargetGrillTemp = SECONDARY_GRILL_TEMP;
      break;
      
    case GRILL_OFF:          
                            // SEND TEXT MSG - Light LED indicating "Time to Wrap in Foil !!!"
      setFanDutyCycle(FAN_OFF);
      TargetGrillTemp = 150;
      MyState = MEAT_FINISHING;
      return;
      break;
      
    case MEAT_FINISHING:
                            // Light LED !!!!!!
      if ( (getTemp(FOOD_PROBE_1_PIN) >= MEAT_FINISHING_TEMP) && (getTemp(FOOD_PROBE_2_PIN) >= MEAT_FINISHING_TEMP) ){
        MyState = FINISHED;
      }
      return;
      break;
      
    case FINISHED:
                            // Light LED !!!!!!
      return;
      break;  
  }
  
  
                                          // Check to see if BOTH probes are above GRILL_OFF_MEAT_TEMP
  if ( (getTemp(FOOD_PROBE_1_PIN) >= GRILL_OFF_MEAT_TEMP) && (getTemp(FOOD_PROBE_2_PIN) >= GRILL_OFF_MEAT_TEMP) ){
    MyState = GRILL_OFF;                     // SHUT'ER DOWN !!!!!!!  Get the tongs out !
    return;
  }

  CurrentGrillTemp = getTemp(GRILL_PROBE_PIN);
  myPID.Compute();      // Run PID alg //

  
          // Set the necessary fan duty cycle
  setFanDutyCycle(FanSpeed);  // 1 degree below -> run fan @ 10%,   10 degrees below - run fan @ 100%
   
  if (TargetGrillTemp - CurrentGrillTemp > 15 ){
                                                               // TEXT MSG and flash all LED's
    line += "Grill > 15 degrees below temp !! - Add charcoal and reignite if necessary\n"; 
  }


    line += String(CurrentGrillTemp) + "/" + String(TargetGrillTemp);
    line += " Food " + String(getTemp(FOOD_PROBE_1_PIN)) + "/";
    line += String(getTemp(FOOD_PROBE_2_PIN)) + "/" + String(getTemp(FOOD_PROBE_3_PIN));
    line += " Fan " + String(FanSpeed);
    line += " Timer " + String((float)millis()/(1000.0*60.0*60.0));
    line += " State " + String(MyState);
    line += "\n";
    
    Console.print(line);

    if (client) {
      Console.print("Got web connection\n");
      
      client.println(line);
      client.stop();
    }
  

  return;
}



void setFanDutyCycle(float speed)
{
  if (speed <= FAN_OFF)                 // fan damper won't open until 65 (FAN_OFF), so just stop the fan.
    analogWrite(FAN_CONTROL_PIN, 0.0);
  else
    analogWrite(FAN_CONTROL_PIN, speed);
}



#define NUM_SAMPLES 1
#define SAMPLE_INTERVAL 250
float getTemp(int pin){
  float valueSum=0;
  float value;
  float Temp;
 
  for (int i=0; i<NUM_SAMPLES; i++){      // Sample and average to smooth drift
    valueSum = valueSum + analogRead(pin);
    delay(SAMPLE_INTERVAL);
  } 
  
  value = valueSum / NUM_SAMPLES;
    
  Temp = log(((10240000/value) - 10000));
  Temp = 1 / (0.00024723753 + (0.00023402251 + (0.00000013879768 * Temp * Temp ))* Temp );
  Temp = Temp - 273.15;            // Convert Kelvin to Celcius
  Temp = (Temp * 9.0)/ 5.0 + 32.0; // Convert Celcius to Fahrenheit

  return Temp;
}

