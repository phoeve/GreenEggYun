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
#define GRILL_OFF_MEAT_TEMP   195
#define MEAT_FINISHING_TEMP   195
#define INITIAL_GRILL_TEMP    233      // Temp for the first hours of the smoke :)
#define SECONDARY_GRILL_TEMP  220      // Finishing temp of the smoke.
#define HOURS_TRIGGER          48      // Set to a large number (i.e. 48) if you don't want to trigger on time.
#define FOOD_TEMP_TRIGGER     140      //  Food temp to trigger SECONDARY_GRILL_TEMP state
#endif

#if BUTT
#define GRILL_OFF_MEAT_TEMP   195 
#define MEAT_FINISHING_TEMP   195
#define INITIAL_GRILL_TEMP    230     // Temp for the first hours of the smoke :)
#define SECONDARY_GRILL_TEMP  275      // Finishing temp of the smoke.
#define HOURS_TRIGGER           5      // Set to a large number (i.e. 24) if you don't want a secondary temp.
#define FOOD_TEMP_TRIGGER     160      //  Food temp to trigger SECONDARY_GRILL_TEMP state
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


void setup() {   

  Bridge.begin();

  Console.begin();
  
  pinMode(FAN_CONTROL_PIN, OUTPUT);    // init fan pin

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

#define FAN_OFF 0
#define FAN_LOW 65
#define FAN_HIGH 255


void loop() 
{
  static int desiredGrillTemp=0;
  

    Console.print("Timer = ");  Console.print((float)millis()/(1000.0*60.0*60.0));  Console.print("\n");
    Console.print("State = ");  Console.print(MyState);  Console.print("\n");
    Console.print("desiredGrillTemp = ");  Console.print(desiredGrillTemp);  Console.print("\n");
    Console.print("GRILL Probe = ");  Console.print(getTemp(GRILL_PROBE_PIN));  Console.print(", ");
    Console.print("Food Probes 1-3 = ");  Console.print(getTemp(FOOD_PROBE_1_PIN));  Console.print(", ");
    Console.print(getTemp(FOOD_PROBE_2_PIN));  Console.print(", ");  Console.print(getTemp(FOOD_PROBE_3_PIN));  Console.print("\n");


  switch (MyState){
    
    case GRILL_INIT:
      setFanDutyCycle(0);                  // turn fan off
      MyState = GRILL_PHASE_1;
      return;
      break;
      
    case GRILL_PHASE_1:
      desiredGrillTemp = INITIAL_GRILL_TEMP;
      
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
      desiredGrillTemp = SECONDARY_GRILL_TEMP;
      break;
      
    case GRILL_OFF:          
                            // SEND TEXT MSG - Light LED indicating "Time to Wrap in Foil !!!"
      setFanDutyCycle(FAN_OFF);
      desiredGrillTemp = 150;
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
          // Set the necessary fan duty cycle
   setFanDutyCycle( (desiredGrillTemp - getTemp(GRILL_PROBE_PIN)) );  // 1 degree below -> run fan @ 10%,   10 degrees below - run fan @ 100%
   
   if (desiredGrillTemp - getTemp(GRILL_PROBE_PIN) > 15 ){
                                                               // TEXT MSG and flash all LED's
     Console.print("Grill > 15 degrees below temp !! - Add charcoal and reignite if necessary\n"); 
   }

  return;
}



void setFanDutyCycle(int diff)
{
  float dutyCycle = diff * 17.0 + FAN_LOW;    // Fan starts working at 65.  *19 gives us 65-255
  
  if (diff <= 0)     // At or above temp
      dutyCycle = FAN_OFF;
  else
    if (diff > 2)       // > 10 degrees low go wide open
      dutyCycle=FAN_HIGH;
  
  analogWrite(FAN_CONTROL_PIN, dutyCycle);
  
  Console.print("dutyCycle = ");
  Console.print(dutyCycle);
  Console.print("\n");
}



#define NUM_SAMPLES 2
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

