
#include "mqtt.h"
#include "ibm_iot.h"
#include "rht03-humidity-temperature-sensor.h"

/*
TO DO:
 - Org and device_type from config file
 - self-register device - store token in EEPROM
 - Proper JSON generation
 - memory management for payload
*/

#define INTERVAL 300*1000       // every 10 sec

#define MQTT_SERVER "us.messaging.internetofthings.ibmcloud.com"
#define MQTT_PORT 1883
#define MQTT_PUBLISH_TOPIC "iot-2/evt/event/fmt/json"
#define MQTT_SUBSCRIBE_TOPIC "iot-2/cmd/+/fmt/json"

#define RED 101
#define GREEN 102
#define BLUE 103

char payload[256];     // holds the MQTT message payload

String clientID="";    // stores global clientID

bool motionDetected=false;

// callback declaration for subsciption messages
void callback(char* topic, byte* payload, unsigned int length);

// set MQTT client adresse
MQTT client(MQTT_SERVER, MQTT_PORT, callback);

/*
  This creates a RHT03HumidityTemperatureSensor object
  on input pin D3
*/
RHT03HumidityTemperatureSensor sensor(D3);

/*
  Set ClientID according to IBM IoT spec
*/
void setClientID(){

  clientID="d:l58oai:Photon:";

  clientID.concat(System.deviceID());

  Serial.println(clientID);
}

/*
  Get current temperatur from sensor
*/
double getTemperatur() {
  /*
    fake implementation
    return random value between 15-25
  */
    return random (15,25);
}
/*
  Simple LED blink
*/
void blinkLED(const unsigned int color){
  for (int i=0; i<3; i++) {
    RGB.color(0,0,0);
    delay(100);
    switch (color) {
      case RED :
        RGB.color(255,0,0);
        break;
      case GREEN :
        RGB.color(0,255,0);
        break;
      case BLUE :
        RGB.color(0,0,255);
        break;
      default :
        RGB.color(255,0,0);
        break;
    }
    delay(100);
  }
  RGB.color(0,0,0);
}

/*
  Callback function for MQTT subscriptions
*/
void callback(char* topic, byte* payload, unsigned int length) {
    // MQTT messages received here in topic and payload
    blinkLED(BLUE);

    Serial.println("Subscription event received!");

    // Publish message on request
    sendMessage();
}

/*
  Callback function for regular timer triggered messages
  i.e. keep alive, temperatur, ambient light, etc
*/
void sendMessage()
{
  if (client.isConnected()) {

    // get latest data from temp/humidity sensor
    sensor.update();

    // compile message payload in JSON format
    sprintf(payload,
      "{\"d\":{\"temperatur\":%f,\"humidity\":%f,\"ambient\":%d}}",
        sensor.getTemperature(), sensor.getHumidity(), analogRead(A0));

    // publish MQTT message
    if (client.publish(MQTT_PUBLISH_TOPIC, payload)) {
      // blink LED if successful
      blinkLED(GREEN);

      Serial.print("Publish: "); Serial.println(payload);
    }
  }
}

/*
  Publish motion event
*/
void sendMotionMessage()
{
  if (client.isConnected()) {

    // compile message payload in JSON format
    sprintf(payload, "{\"d\":{\"motion\":%d}}", motionDetected);

    // publish MQTT message
    if (client.publish(MQTT_PUBLISH_TOPIC, payload)) {
      // blink LED if successful
      blinkLED(GREEN);

      Serial.print("Publish: "); Serial.println(payload);
    }
  }
}

/*
  create a software timer for regular messages
*/
Timer temperaturTimer(INTERVAL, sendMessage);

/*
  call on interupt
*/
void motionDetectedFunc() {
  motionDetected = true;
}

/*
  Called at Photon startup
*/
void setup()
{
  bool init = false;

  // take control of the Photon LED
  RGB.control(true);
  RGB.color(255,0,0);
  delay(1000);

  while (!init) {
    // Wait it out until we have a network to talk to
    if (WiFi.ready()) {
      // set clientID for MQTT connection
      setClientID();

      // connect to IBM IoT as clientID with username and token
      client.connect(clientID, AUTHORIZATION, TOKEN);

      if (client.isConnected()) {
        init = true;
      }
      else {
        // wait 1 sec before next connection attempt
        delay(1000);
      }
  	}
  }

  // start temperatur timer
  temperaturTimer.start();

  /*
    Set D2 as digital input for the motion detector
    and attach interupt on D2 to motionDetectedFunc
  */
  pinMode(D2, INPUT_PULLUP);
  attachInterrupt(D2, motionDetectedFunc, RISING);

  // set A0 is analog input for ambient light
  pinMode(A0, INPUT);

  // subscribe to MQTT commands
  client.subscribe(MQTT_SUBSCRIBE_TOPIC);

  // send first message
  sendMessage();
}

/*
  Main loop
*/
void loop() {

  // Listen for MQTT subscription events
  if (client.isConnected()) {

    // check for interupts
    if (motionDetected) {
      Serial.println ("Motion!!!");

      // Publish MQTT message
      sendMotionMessage();

      // reset motionDetected flag
      motionDetected = false;
    }

    // listen for MQTT subscription events
    client.loop();
  }
  else {
    // Set LED to RED
    RGB.color(255,0,0);

    // reconnect
    client.connect(clientID, AUTHORIZATION, TOKEN);
    delay(1000);

    Serial.println("Reconnect");
  }
}
