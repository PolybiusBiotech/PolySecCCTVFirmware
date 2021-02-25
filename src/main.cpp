/*
    This sketch shows how to configure different external or internal clock sources for the Ethernet PHY
*/
#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>
#include <ArtnetnodeWifi.h>
#include <lx16a-servo.h>
#include <FastLED.h>
/*
   * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
   * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
*/
// #define ETH_CLK_MODE    ETH_CLOCK_GPIO0_OUT          // Version with PSRAM
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT            // Version with not PSRAM

// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define ETH_POWER_PIN   -1

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE        ETH_PHY_LAN8720

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR        0

// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN     23

// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN    18

#define NRST            5
static bool eth_connected = false;

WiFiUDP UdpSend;
ArtnetnodeWifi artnetnode;

LX16ABus servoBus;
LX16AServo servoTilt(&servoBus, 1);
int32_t lastPosRequestTiltHi = 0;
int32_t lastPosRequestTiltLow = 0;
LX16AServo servoPan(&servoBus, 2);
int32_t lastPosRequestPanHi = 0;
int32_t lastPosRequestPanLow = 0;

// How many leds in your strip?
#define NUM_LEDS 66 

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806, define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 12

// Define the array of leds
CRGB leds[NUM_LEDS];

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  boolean tail = false;
  
  Serial.print("DMX: Univ: ");
  Serial.print(universe, DEC);
  Serial.print(", Seq: ");
  Serial.print(sequence, DEC);
  Serial.print(", Data (");
  Serial.print(length, DEC);
  Serial.print("): ");
  
  if (length > 16) {
    length = 16;
    tail = true;
  }
  // send out the buffer
  for (int i = 0; i < length; i++)
  {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  if (tail) {
    Serial.print("...");
  }
  Serial.println();

  if(universe == 0){
    int32_t angle = 0;
    int32_t currentPos = 0;
    int32_t distance = 0;
    int32_t speed = 10;
    if(lastPosRequestPanHi != data[1] || lastPosRequestPanLow != data[2]){
        lastPosRequestPanHi = data[1];
        lastPosRequestPanLow = data[2];
        uint16_t combinedPan = ((lastPosRequestPanHi << 8) + lastPosRequestPanLow);
        Serial.print("combinedPan: ");
        Serial.println(combinedPan, HEX);
        if(combinedPan != 65535){
            angle = (24000.0/65535.0) * combinedPan;
        }
        else {
            angle = 24000;
        }
        Serial.print("Angle requested: ");
        Serial.println(angle);
        currentPos = servoPan.pos_read();
        if(currentPos > 24000){
            currentPos = 0;
        }
        Serial.print("Current angle: ");
        Serial.println(currentPos);
        if(angle < currentPos){
            distance = currentPos - angle;
        }
        else{
            distance = angle - currentPos;
        }
        Serial.print("Distance: ");
        Serial.println(distance);
        Serial.print("Time to move: ");
        Serial.println(distance / speed);
        servoPan.move_time(angle, distance / speed);
    }
    
    if(lastPosRequestTiltHi != data[3] || lastPosRequestTiltLow != data[4]){
        lastPosRequestTiltHi = data[3];
        lastPosRequestTiltLow = data[4];
        int16_t combinedTilt = (lastPosRequestTiltHi << 8) | (lastPosRequestTiltLow & 0xff);
        if(combinedTilt != 65535){
            angle = (24000.0/65535.0) * combinedTilt;
        }
        else {
            angle = 24000;
        }
        Serial.print("Angle requested: ");
        Serial.println(angle);
        currentPos = servoTilt.pos_read();
        if(currentPos > 24000){
            currentPos = 0;
        }
        Serial.print("Current angle: ");
        Serial.println(currentPos);
        if(angle < currentPos){
            distance = currentPos - angle;
        }
        else{
            distance = angle - currentPos;
        }
        Serial.print("Distance: ");
        Serial.println(distance);
        Serial.print("Time to move: ");
        Serial.println(distance / speed);
        servoTilt.move_time(angle, distance / speed);
    }
  }
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event) {
    case SYSTEM_EVENT_ETH_START:
        Serial.println("ETH Started");
        //set eth hostname here
        ETH.setHostname("esp32-ethernet");
        break;
    case SYSTEM_EVENT_ETH_CONNECTED:
        Serial.println("ETH Connected");
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        Serial.print("ETH MAC: ");
        Serial.print(ETH.macAddress());
        Serial.print(", IPv4: ");
        Serial.print(ETH.localIP());
        if (ETH.fullDuplex()) {
            Serial.print(", FULL_DUPLEX");
        }
        Serial.print(", ");
        Serial.print(ETH.linkSpeed());
        Serial.println("Mbps");
        artnetnode.begin();
        eth_connected = true;
        break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
        Serial.println("ETH Disconnected");
        eth_connected = false;
        break;
    case SYSTEM_EVENT_ETH_STOP:
        Serial.println("ETH Stopped");
        eth_connected = false;
        break;
    default:
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    servoBus.beginOnePinMode(&Serial2,33);
	servoBus.debug(true);
	servoBus.retry=0;

    WiFi.onEvent(WiFiEvent);

    pinMode(NRST, OUTPUT);
    digitalWrite(NRST, 0);
    delay(200);
    digitalWrite(NRST, 1);
    delay(200);
    digitalWrite(NRST, 0);
    delay(200);
    digitalWrite(NRST, 1);

    // max. 17 characters
    artnetnode.setShortName("PolySec Cam #0001");
    // max. 63 characters
    artnetnode.setLongName("PolySec Cam #0001 | Version 0.1");
    // set a starting universe if you wish, defaults to 0
    //artnetnode.setStartingUniverse(4);
    artnetnode.setNumPorts(4);
    artnetnode.enableDMXOutput(0);
    artnetnode.enableDMXOutput(1);
    artnetnode.enableDMXOutput(2);
    artnetnode.enableDMXOutput(3);

    // this will be called for each packet received
    artnetnode.setArtDmxCallback(onDmxFrame);

    LEDS.addLeds<WS2812,DATA_PIN,RGB>(leds,NUM_LEDS);
	LEDS.setBrightness(84);

    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }


void loop()
{
    static uint8_t hue = 0;
	Serial.print("x");
	// First slide the led in one direction
	for(int i = 0; i < NUM_LEDS; i++) {
		// Set the i'th led to red 
		leds[i] = CHSV(hue++, 255, 255);
		// Show the leds
		FastLED.show(); 
		// now that we've shown the leds, reset the i'th led to black
		// leds[i] = CRGB::Black;
		fadeall();
        if (eth_connected) {
            artnetnode.read();
        }
		// Wait a little bit before we loop around and do it again
		delay(10);
	}
	Serial.print("x");

	// Now go in the other direction.  
	for(int i = (NUM_LEDS)-1; i >= 0; i--) {
		// Set the i'th led to red 
		leds[i] = CHSV(hue++, 255, 255);
		// Show the leds
		FastLED.show();
		// now that we've shown the leds, reset the i'th led to black
		// leds[i] = CRGB::Black;
		fadeall();
        if (eth_connected) {
            artnetnode.read();
        }
		// Wait a little bit before we loop around and do it again
		delay(10);
	}
}