/*
 * Bedlights controller.
 *
 * Controls an RGB LED Stripe and a 3W white LED over NRF24L01+ radio modules.
 * Lights can be switched on and off either by a PIR sensor or manually.
 */

#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
//#include <ClickEncoder.h>
//#include <TimerOne.h>

// Pins for RF24 module
#define CE       7
#define CS       8

//header type constants, common to all sensors
#define HEADER_HUM_TEMP 0
#define HEADER_TEMP 1
#define HEADER_LEDS 2

//time in milli seconds to leave lights on if motion is detected
#define MOTION_ON_TIME 30000

// Pins for LEDs
#define GREEN    3 
#define RED      5 
#define BLUE     6 

//other pins
#define WHITE   10
#define CLICK_A A0 
#define CLICK_B A1
#define CLICK_BTN A2

// Address of our node and the other
const uint16_t this_node = 5;
const uint16_t other_node = 0;

//*************** Structure of our payload ********
struct payload
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t white;
  //operating mode. 
  //autofire: 1
  //always on: 2
  uint8_t mode;
};
typedef struct payload Payload;

//*************** Structure of our payload ********

//*************** setup necessary stuff *********
RF24 radio(CE, CS);
RF24Network network(radio);

Payload payload;
//ClickEncoder *encoder;
//*************** setup necessary stuff *********

//************* some global variables **********
volatile unsigned long lastMotionDetected;
bool autofire = true;
bool alwayson = false;
uint8_t r = 0; 
uint8_t g = 0; 
uint8_t b = 0;
uint8_t white = 255; //white is inverted, so 255 means off and 0 means fully on
uint8_t mode = 1; //start with mode == 1: if the PIR sensor triggers, switch on lights
bool light_is_on = true; //the current state of lights

//int16_t currentEncoderValue;
//int16_t lastEncoderValue;
//************* some global variables **********

void setup(void)
{
  //Initialize SPI and NRF24 module
  SPI.begin();
  radio.begin();
  network.begin( 90,  this_node);

  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setCRCLength(RF24_CRC_16);

  //attach interrupt for PIR module to pin D2. This is not the pin number but the interrupt number!
  attachInterrupt(0, MOTION_ISR, RISING);

  //rotary encoder stuff
  /*
  encoder = new ClickEncoder(CLICK_A, CLICK_B, CLICK_BTN);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(ENCODER_ISR);
  */
}

void loop(void)
{
/*
  //handle button klicks etc.
  currentEncoderValue += encoder->getValue();
  if(currentEncoderValue != lastEncoderValue)
  {
    white += currentEncoderValue;
    lastEncoderValue = currentEncoderValue;
  }
    //FIXME: adjust brightness
  ClickEncoder::Button button = encoder->getButton();
  switch (button) {
    case ClickEncoder::Pressed:
      break;
    case ClickEncoder::Held:
      break;
    case ClickEncoder::Released:
      break;
    case ClickEncoder::Clicked: 
      //toggle light on/off
      alwayson = !alwayson;
      break;
    case ClickEncoder::DoubleClicked:
      break;
  }
*/

  // Pump the network regularly
  network.update();
  
  // Is there anything ready for us?
  while ( network.available() )
  {
    // If so, grab and handle it
    RF24NetworkHeader header;
    network.peek(header);
    if(header.type == HEADER_LEDS)
    {
      network.read(header,&payload,sizeof(payload));
      r = payload.r;
      g = payload.g;
      b = payload.b;
      white = 255 - payload.white; //white is inverted
      mode = payload.mode;
      autofire = (mode & 1) == 1;
      alwayson = (mode & 2) == 2;
      if(alwayson)
      {
        //make sure, changed values get picked up by the if statements below
        light_is_on = false;
      }
    }
  }
 
  if(light_is_on)
  {
    //only consider switching off events
    //FIXME: overflow of millis()?
    if(alwayson || autofire && lastMotionDetected > millis() - MOTION_ON_TIME)
    {
      //leave on
    }
    else
    {
      //switch off
      rgbw(0, 0, 0, 255);
      light_is_on = false;
    }
  }
  else
  {
    //only consider switching on events
    if(alwayson || autofire && lastMotionDetected > millis() - MOTION_ON_TIME)
    {
      rgbw(r, g, b, white);
      light_is_on = true;
    }
  }
}

void rgbw(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _w)
{
    analogWrite(RED, _r);
    analogWrite(GREEN, _g);
    analogWrite(BLUE, _b);
    analogWrite(WHITE, _w);
}

void MOTION_ISR()
{
  //just store when the last motion event came in
  lastMotionDetected = millis();
}
/*
void ENCODER_ISR()
{
  encoder->service();
}
*/

// vim:ai:cin:sts=2 sw=2 ft=cpp
