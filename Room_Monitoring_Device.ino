

/* Room Monitor Devices (RMD)
 *  An RMD reads environmental conditions (temperature, humidity, pressure, luminance, motion) and communicates
 *  the data/events to the MQTT server defined.  RMD's also have an LED on-board to assist in identifying the device.
 *  
 *  MQTT Topics:
 *    Device Management Requests:
 *      /manage/[Device Name]/[Operation]
 *  
 *    Device Status Responses:
 *      /client/[Device Name]
 *      
 *    Sensor Data:
 *      /sensor/[Device Name]/[Peripheral] [Value]
 *      
 *    Note, MAC addresses have no spaces, dashes, or colons.  MAC AA:BB:CC:DD:EE:00 = AABBCCDDEE00.
 *  
 *  (C) 2018, P5 Software, LLC
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h> //https://arduinojson.org/
#include <ESP8266HTTPClient.h>
#include <BME280I2C.h> //https://github.com/finitespace/BME280
#include <BH1750FVI.h> //https://github.com/PeterEmbedded/BH1750FVI
#include <Wire.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>


const int firmwareVersion = 1;


enum enumLEDMode {
  off = 0,
  on = 1,
  blink = 2
};


/* Define a structure to hold all of the settings */
typedef struct structSettings{
  bool deviceIsProvisioned;
  String ssidName;
  String wpaKey;
  String machineMacAddress;
  String deviceName;
  bool forceSensorUpdate;

  struct {
    String serverName;
    int port;
    String username;
    String password;
    String manageTopic;
    String clientTopic;
    String sensorTopic;
  }mqttServer;

  struct{
    String url;
  }firmwareServer;

  struct {
    bool enabled;
    int lampStatus;
    enumLEDMode mode;
    int pin;
    long blinkInterval;
    unsigned long previousHandledTime;
  }onboardLED;

  struct {
    bool enabled;
    float lastReportedValue;
  }temperaturePeripheral;

  struct {
    bool enabled;
    float lastReportedValue;
  }humidityPeripheral;

  struct {
    bool enabled;
    float lastReportedValue;
  }atmosphericPressurePeripheral;

  struct {
    bool enabled;
    int pin;
    int lastReportedValue;
    bool forceSensorUpdate;
  }motionPeripheral;

  struct {
    bool enabled;
    int pin;
    float lastReportedValue;
  }luminancePeripheral;
};

const int EEPROMLength = 512;
const int EEPROMDataBegin = 10; //Reserving the first 10 addresses for provision statuses

const int observeSensorsEveryMilliseconds = 5000; //Every 5 seconds
const unsigned long healthUpdateEveryMilliseconds = 1680000; //Every 28 minutes
const unsigned long firmwareUpdateEveryMilliseconds = 3600000; //Every sixty minutes

unsigned long previousSensorHandledTime = 0;
unsigned long previousHealthHandledTime = 0;
unsigned long previousFirmwareHandledTime = 0;


struct structSettings settings;

WiFiClient espClient;
ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdater;
PubSubClient mqttClient(espClient);
WiFiServer telnetServer(23);
WiFiClient telnetClient;
WiFiClient tmpTelnetClient;
BME280I2C atmosphereSensor; //Define the BME280 device, I2C
BH1750FVI lightSensor(-1, BH1750FVI::k_DevAddress_L, BH1750FVI::k_DevModeContHighRes); //Define the BH1750 light sensor with address 0x5C, which only uses I2C; Set the device to continuous high resolution.

void setup(){

  Serial.begin(115200);

  //Start SPI
  Wire.begin();

  delay(5000);

  broadcastLine("");
  broadcastLine(padRight("", 23, ">") + " System Restarted " + padRight("", 23, "<"));

  //Get the MAC address
  settings.machineMacAddress = WiFi.macAddress();
  settings.machineMacAddress.toUpperCase();

  //Set the device name from the MAC address
  settings.deviceName = settings.machineMacAddress;
  settings.deviceName.replace(":","");

  //Set the hostname
  WiFi.hostname("RMD-" + settings.deviceName);  
  broadcastLine("Hostname is " +  WiFi.hostname()); 

  //Check the provisioning status
  checkDeviceProvisioned();
  
  //Set the device's current mode depending on if it is provisioned or not
  if(settings.deviceIsProvisioned == false){

    //Configure for provisioning mode
    setupProvisioningMode();

  }else{

    //The device is provisioned, get the settings from the EEPROM
    broadcastLine("Device has been provisioned.");

    //Read the data from EEPROM
    readEEPROMToRAM();
    
    //Setup the WiFi Connection
    //Attempt to connect to the WiFi Network as a client
    WiFi.mode(WIFI_STA);
  
    WiFi.begin(settings.ssidName.c_str(), settings.wpaKey.c_str());
  
    broadcastLine("Attempting to connect to network " + settings.ssidName); 
    
    //Print output while waiting for WiFi to connect
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  
    Serial.print("connected.\n");
    broadcastLine("IP address: " + WiFi.localIP().toString() + "\nMAC Address: " + WiFi.macAddress() + "\n");

    //Start the telent server
    telnetServer.begin();
  
    //Configure the mqtt Client
    mqttClient.setServer(settings.mqttServer.serverName.c_str(), settings.mqttServer.port);
    mqttClient.setCallback(mqttCallback);

    //Setup the peripherals for use
    setupPeripherals();
  }

}


void loop() {

  //See which loop we should execute
  if(settings.deviceIsProvisioned == false){

      //Setup a client handler
      webServer.handleClient();
    
  }else{


    //The device has been provisioned, run the normal processes
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
  
    mqttClient.loop();

    //See if there are telnet client connections waiting.  If there are not, this will be false.  If there are, an object is returned.
    tmpTelnetClient = telnetServer.available();
  
    if(tmpTelnetClient){
  
      broadcastLine("A new telnet client connected.  Dropping any existing connections.");
  
      //Copy the object over to the actual telnetClient, which will kill off any existing telnet clients
      telnetClient = tmpTelnetClient;
  
   }

    //Handle any LED blink requests
    handleOnboardLEDBlink();

    //Always handle the motion peripheral
    handleMotionPeripheral();


    //Handle peripheral updates if enough time has passed
    if((unsigned long)(millis() - previousSensorHandledTime) > (unsigned long)observeSensorsEveryMilliseconds){

      //Create a header for the table of data we're going to output on the console
      broadcastLine(padRight("", 64, "*"));
      formatAndBroadcastSensorDebug("Sensor","Reported","Observed","Delta","Minimum Delta");
      formatAndBroadcastSensorDebug("-----------", "----------", "----------", "--------", "-------------");

      //Go read each sensor
      handleTemperaturePeripheral();
      handleHumidityPeripheral();
      handleAtmosphericPressurePeripheral();
      handleLuminancePeripheral();

      //Update the last handled time to be now
      previousSensorHandledTime = millis();

      //Clear the flag to force updates
      settings.forceSensorUpdate = false;

    }
 
    //Handle health updates if enough time has passed
    if((unsigned long)(millis() - previousHealthHandledTime) > (unsigned long)healthUpdateEveryMilliseconds){

      //We want to ignore the first attempt to run the health check, otherwise it will fire on startup, which is abusive
      if(previousHealthHandledTime > 0) {
        
        broadcastLine(padRight("", 64, "#"));
        broadcastLine("Health Update");

        //Time to send a health status update
        handleHealth();

        broadcastLine(padRight("", 64, "#"));

      }

      //Update the last handled time to be now
      previousHealthHandledTime = millis();
    }

    //Handle firmware updates if enough time has passed
    if((unsigned long)(millis() - previousFirmwareHandledTime) > (unsigned long)firmwareUpdateEveryMilliseconds){

      checkFirmwareUpgrade();

      //Update the last handled time to be now
      previousFirmwareHandledTime = millis();
      
    }
    
  }

}


void checkDeviceProvisioned(){
  /*
   * This function will check the EEPROM to see if the device has been provisioned.  If the EEPROM is not "true", it will assume the device has not been provisioned and will
   * set the setting accordingly.
   */

   //Read the EEPROM starting at memory position 0
   if(EEPROMRead(0) == "true"){

      //The device has been provisioned, set the setting to true
      settings.deviceIsProvisioned = true;
      
   }else{

      //The device has not been provisioned, set the setting to false
      settings.deviceIsProvisioned = false;
    
   }
  
}


void setupProvisioningMode(){
  /*
   * Configures necessary items for when the device has not been provisioned.
   */

  //Set the default IP address, gateway, and subnet to use when in AP mode
  IPAddress ip(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(ip, gateway, subnet);
  
  WiFi.softAP(("RMD-" + settings.deviceName).c_str());

  webServer.on("/", wwwHandleRoot);
  webServer.onNotFound(wwwHandleNotFound);

  MDNS.begin(settings.deviceName.c_str());

  httpUpdater.setup(&webServer);

  MDNS.addService("http", "tcp", 80);

  webServer.begin();
  
  broadcastLine("Device requires provisioning.");
  broadcastLine("Connect to Access Point RMD-" + settings.deviceName + " and point your browser to http://192.168.1.1 to begin.  To update firmware, go to http://192.168.1.1/update.");
  
}


void attemptProvision(String SSID, String wpaKey, String bootstrapURL){

  //Kill the soft AP
  WiFi.softAPdisconnect(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID.c_str(), wpaKey.c_str());

  //Remember the start time
  unsigned long timeStartConnect = millis();

  broadcastLine("Attempting to connect to network " + SSID);
  
  //Print output while waiting for WiFi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    //Set a timer for 30 seconds to connect
    if(millis() - timeStartConnect > 30000){

      //Disconnect the WiFi
      WiFi.disconnect();

      //Write an error
      broadcastLine("\nAttempting to provision failed!  Unable to connect to AP specified.");

      //Restart the provisioning server
      setupProvisioningMode();

      //Exit this
      return;
      
    }
  }

  Serial.print("connected.");
  broadcastLine("");
  broadcastLine("IP address: " + WiFi.localIP().toString() + "\nMAC Address: " + WiFi.macAddress() + "\n");

  if(retrieveBootstrap(bootstrapURL) == true){
      broadcastLine("Retrieved configuration successfully.  Restarting.");

      //Ensure the EEPROM has time to finish writing and closing the client
      delay(1000);

      //Restart
      ESP.restart();
      
    } else {
      
      //Restart on failure
      ESP.restart();

    }

}

boolean retrieveBootstrap(String bootstrapURL){
  
  HTTPClient httpClient;

  httpClient.begin(bootstrapURL);

  //See if we were successful in retrieving the bootstrap
  if (httpClient.GET() == HTTP_CODE_OK) {

    //Write the response to the EEPROM
    EEPROMWrite(httpClient.getString(), EEPROMDataBegin);

    //Set the provision status
    EEPROMWrite("true", 0);

    httpClient.end();

    //Re-Read the EEPROM into RAM
    readEEPROMToRAM();

    return true;
    
  }else{

    broadcastLine("Failed retrieving bootstrap from " + bootstrapURL + ": " + (String)httpClient.errorToString(httpClient.GET()) + ".");
    
    return false;
  }

}


void setupPeripherals(){
  /*
   * Sets up the peripherals on boot.
   */

  //Setup the BME280 atmospheric sensor (I2C)
  atmosphereSensor.begin();

  //Sleep 500ms, the sensor needs some time to start up
  delay(500);


  //Make sure the BME280 chip was found
  switch(atmosphereSensor.chipModel())
  {
    case BME280::ChipModel_BME280:
      
      //Enable the sensors
      settings.temperaturePeripheral.enabled = true;
      settings.humidityPeripheral.enabled = true;
      settings.atmosphericPressurePeripheral.enabled = true;

      break;

    default:

      //Fire a notice to the console
      broadcastLine("Unable to Find Sensor BME280 Disabling Peripherals");

      //Disable the sensors
      settings.temperaturePeripheral.enabled = false;
      settings.atmosphericPressurePeripheral.enabled = false;
      settings.humidityPeripheral.enabled = false;

      //Send zero values
      publishMQTT(settings.mqttServer.sensorTopic + "/temperature", "0");
      publishMQTT(settings.mqttServer.sensorTopic + "/pressure", "0");
      publishMQTT(settings.mqttServer.sensorTopic + "/humidity", "0");
  }

  //Setup the onboard LED (GPIO)
  settings.onboardLED.enabled = true;
  settings.onboardLED.pin = 15; //D8
  pinMode(settings.onboardLED.pin, OUTPUT); 
  settings.onboardLED.lampStatus = LOW;
  settings.onboardLED.mode = off;
  settings.onboardLED.blinkInterval = 500;
  settings.onboardLED.previousHandledTime = 0;

  //Setup the passive infrared sensor (GPIO)
  settings.motionPeripheral.enabled = true;
  settings.motionPeripheral.pin = 12; //D6
  pinMode(settings.motionPeripheral.pin, INPUT);
  
  //Setup the luminance light sensor (I2C)
  lightSensor.begin();
  settings.luminancePeripheral.enabled = true;

  //Force the first reading to happen by setting the values very low
  settings.forceSensorUpdate = true;
  settings.motionPeripheral.forceSensorUpdate = true;
 
}


void reconnectMQTT() {

  
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    
    broadcastLine("Attempting to connect to MQTT server " + settings.mqttServer.serverName + ": ");
        
    // Attempt to connect
    if (mqttClient.connect(settings.machineMacAddress.c_str(), settings.mqttServer.username.c_str(), settings.mqttServer.password.c_str())) {
      
      broadcastLine("connected.");
      
      // Publish an announcement on the client topic
      publishMQTT(settings.mqttServer.clientTopic + "/status", "Started");

      broadcastLine(padRight("", 64, "#"));
      
      //Publish the health
      handleHealth();

      broadcastLine(padRight("", 64, "#"));
      
      // Subscribe to the management topic wildcard
      mqttClient.subscribe((settings.mqttServer.manageTopic + "/#").c_str());
      
    } else {
      
      broadcastLine("failed, rc=" + (String)mqttClient.state());
      broadcastLine("Try again in 5 seconds");
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {
  /* This function will handle all incoming requests for subscribed topics */

  String receivedPayload = "";
 
  //Assemble the payload data
  for (int i = 0; i < length; i++) {
    receivedPayload = String(receivedPayload + (char)payload[i]);
  }

  receivedPayload.trim();

  handleMQTTManagementMessage(String(topic), receivedPayload);

}


void publishMQTT(String topic, String payload){
  
  /*
   * Sends the requested payload to the requested topic, and also broadcasts the message on the serial line.
   */

  //Clean the strings
  topic.trim();
  payload.trim();

  mqttClient.publish(String(topic).c_str(), String(payload).c_str());
  broadcastLine("<=> " + padRight(topic, 40, " ") + payload);
  
}


void handleMQTTManagementMessage(String topic, String payload){
  /* Handles incoming management requests from MQTT.  Payloads must exactly match.  Output is echoed on serial and MQTT /client/# topic. */

  //Copy the payload so we can modify it
  String payloadToLowerCase = payload;

  //Make the payload lower
  payloadToLowerCase.toLowerCase();

  //See if the payload requests a restart
  if(topic == settings.mqttServer.manageTopic + "/restart" && payloadToLowerCase == "request"){

    publishMQTT(settings.mqttServer.clientTopic + "/restart", "OK");
    
    delay(500);
    
    ESP.restart();
    
  }

  //See if the payload requests a status message
  if(topic == settings.mqttServer.manageTopic + "/status"){
    handleHealth();
  }

  //Force the system to attempt a firmware update
  if(topic == settings.mqttServer.manageTopic + "/firmwareUpdate"){
    checkFirmwareUpgrade();
  }

  //See if the payload requests an updated bootstrap
  if(topic == settings.mqttServer.manageTopic + "/bootstrap" && payload != ""){

    if(retrieveBootstrap(payload) == true){

      broadcastLine("Bootstrapped from " + payload);
      publishMQTT(settings.mqttServer.clientTopic + "/bootstrap", "OK");
      broadcastLine(EEPROMRead(EEPROMDataBegin));
      
    }else{
      
      publishMQTT(settings.mqttServer.clientTopic + "/bootstrap", "Failed");
      
    }

  }

  //Clear the EEPROM and reset provisioning
  if(topic == settings.mqttServer.manageTopic + "/provisionReset" && payloadToLowerCase == "request"){

    EEPROMWrite("false", 0);

    publishMQTT(settings.mqttServer.clientTopic + "/status", "Provision Reset");
    publishMQTT(settings.mqttServer.clientTopic + "/restart", "OK");

    //Ensure the EEPROM has time to finish writing and closing the client
    delay(1000);

    //Restart
    ESP.restart();
  }

  //Turn on the onboard LED
  if(topic == settings.mqttServer.manageTopic + "/onboardLED"){

    if(payloadToLowerCase == "on"){
      setLEDMode(on);     
    }

    if(payloadToLowerCase == "off"){
      setLEDMode(off); 
    }


    if(payloadToLowerCase == "blink"){
      setLEDMode(blink);
    }

    //If the payload is blank, return the status
    if(payload == ""){
      
      String strMode = "";
      
      switch (settings.onboardLED.mode){

        case on:
          strMode = "on";
          break;
        case off:
          strMode = "off";
          break;
        case blink:
          strMode = "blink";
          break;
        default:
          strMode = "unknown";
          break;        
      }

      publishMQTT(settings.mqttServer.clientTopic + "/onboardLED", strMode);
    }
  }  
}


void EEPROMWrite(String stringToWrite, int startPosition){
  /*
   * This function writes a string of data to the EEPROM beginning at a specified address.  It is sandwiched between two non-printable ASCII characters (2/STX and 3/ETX)
   * to deliniate where the data begins and ends, which is used when reading the data.
   */

  //Sandwich the data with STX and ETX so we can keep track of the data
  stringToWrite = char(02) + stringToWrite + char(03);

  //Set the EEPROM size
  EEPROM.begin(EEPROMLength);

  //Write a start of text and end of text around the actual text
  for (int i = 0; i < stringToWrite.length(); ++i)
  {
    //Write the data to the EEPROM buffer
    EEPROM.write((startPosition + i), stringToWrite[i]);
  }

  //Commit the EEPROM
  EEPROM.commit();
  
}

String EEPROMRead(int startPosition){
  /*
   * This function reads EEPROM data.  It is sandwiched between two non-printable ASCII characters (2/STX and 3/ETX) to deliniate where the data begins and ends.
   * Becasuse the EEPROM length is longer than the data contained, we can only reliably read the data between the two non-printable characters, as there could be junk data left
   * after the data string in the EEPROM.  After the end of the data feed is found, we automatically advance to the end to force the function to stop reading (we could have another
   * STX later on that we don't want to pick up).
   */

  byte value;
  String strReturnValue = "";
  bool isProcessingData = false;

  EEPROM.begin(EEPROMLength);

  //Start reading the EEPROM where the data begins until the EEPROM length
  for(int i=startPosition; i<EEPROMLength; i++){

    // read a byte from the current address of the EEPROM
    value = EEPROM.read(i);

    //See what the value of this byte is
    switch(value){

      case 2:
        //This byte is the beginning of the data, start saving information at this point
        isProcessingData = true;
        break;

      case 3:
         //This byte is the end of the data, stop saving information at this point
        isProcessingData = false;

        //Set the address to the end of the EEPROM so we will break out of the WHILE loop
        i = EEPROMLength;
        break;

      default:

        //If the character is printable AND we are processing a word, store the character to the string
        if(value >= 32 && value <= 127 && isProcessingData == true){
          strReturnValue = strReturnValue + String(char(value));
        }   
      }
    }

  return strReturnValue;

}

void readEEPROMToRAM(){

  //Read the data from EEPROM
    DynamicJsonBuffer jsonBuffer(1500);

    //Read the EEPROM
    JsonObject& root = jsonBuffer.parseObject(EEPROMRead(EEPROMDataBegin));
    
    if (!root.success()) {
      broadcastLine("parseObject() failed");
      EEPROMWrite("false",0);
      return;
    }

    //Store the settings
    settings.ssidName = root["SSID"].asString();
    settings.wpaKey = root["wpaKey"].asString();

    JsonObject& mqtt = root["mqtt"];
    settings.mqttServer.serverName = mqtt["server"].asString();
    settings.mqttServer.port = mqtt["port"];
    settings.mqttServer.username = settings.deviceName;
    settings.mqttServer.password = settings.deviceName;
    settings.mqttServer.manageTopic = "/manage/" + settings.deviceName;
    settings.mqttServer.clientTopic = "/client/" + settings.deviceName;
    settings.mqttServer.sensorTopic = "/sensor/" + settings.deviceName;

    JsonObject& firmware = root["firmware"];
    settings.firmwareServer.url = firmware["url"].asString();
  
}


void broadcastLine(String data){

  /*
   * Prints the data input on the serial console and also on the telnet client.
   */

  Serial.println(data);
  telnetClient.println(data);
  
}


void formatAndBroadcastSensorDebug(String Sensor, String Reported, String Observed, String Delta, String MinDelta){

  /*
   * Formats and then broadcasts debug sensor data.
   */

   const int sensorLen = 12;
   const int reportedLen = 11;
   const int deltaLen = 9;
   const int minDeltaLen = 14;

   //Trim the incoming data
   Sensor.trim();
   Reported.trim();
   Observed.trim();
   Delta.trim();
   MinDelta.trim();

  //Broadcast the completed line
  broadcastLine(padRight(Sensor, sensorLen, " ") + "| " + padRight(Reported, reportedLen, " ") + "| " + padRight(Observed, reportedLen, " ") + "| " + padRight(Delta, deltaLen, " ") + "| " + padRight(MinDelta, minDeltaLen, " "));
  
}


String padRight(String data, int toLength, String padValue){

  while(data.length() < toLength){
    data = data + padValue;    
  }

  return data;
    
}


void setLEDMode(enumLEDMode ledMode){
  /*
   * Sets the LED on, off, or in blink mode if the onboard LED is enabled.  Note that handleOnboardLEDBlink() handles the actual blinking routine.
   */

  //Make sure the LED is enabled
  if(settings.onboardLED.enabled == false){

    publishMQTT(settings.mqttServer.clientTopic + "/onboardLED", "disabled"); 
    
    return;
  }

  switch(ledMode){

    //Turn off the LED and turn off blinking
    case off:
      settings.onboardLED.mode = off;
      digitalWrite(settings.onboardLED.pin, LOW); 
      settings.onboardLED.lampStatus = LOW; 
      publishMQTT(settings.mqttServer.clientTopic + "/onboardLED", "off"); 
      break;

    //Turn off the LED and turn off blinking
    case on:
      settings.onboardLED.mode = on;
      digitalWrite(settings.onboardLED.pin, HIGH);
      settings.onboardLED.lampStatus = HIGH;
      publishMQTT(settings.mqttServer.clientTopic + "/onboardLED", "on");
      break;

    //Set the LED blink mode
    case blink:

      //If we send blink again, stop the blinking.
      if(settings.onboardLED.mode == blink){
        settings.onboardLED.mode = off;
        digitalWrite(settings.onboardLED.pin, LOW); 
        settings.onboardLED.lampStatus = LOW;
        publishMQTT(settings.mqttServer.clientTopic + "/onboardLED", "off");
        
      }else{

        //We are not currently blinking...so let's blink
        settings.onboardLED.lampStatus = LOW;
        settings.onboardLED.mode = blink;
        publishMQTT(settings.mqttServer.clientTopic + "/onboardLED", "blink");
      }
      break;

    default:
    
      //do nothing
      break;
  }
}


void handleOnboardLEDBlink(){
  /*
   * Makes the onboard LED blink if it is enabled and set to blink mode
   */

   //Make sure the LED is enabled
  if(settings.onboardLED.enabled == false){
    
    return;
  }

  //If we are set to blink mode then blink the LED
  if(settings.onboardLED.mode == blink){
  
    unsigned long currentMillis = millis();
    
    if (currentMillis - settings.onboardLED.previousHandledTime >= settings.onboardLED.blinkInterval) {
      
      // save the last time you blinked the LED
      settings.onboardLED.previousHandledTime = currentMillis;
      
      // if the LED is off turn it on and vice-versa:
      if (settings.onboardLED.lampStatus == LOW) {
        
        settings.onboardLED.lampStatus = HIGH;
        
      } else {
        
        settings.onboardLED.lampStatus = LOW;
      }
      
      // set the LED with the ledState of the variable:
      digitalWrite(settings.onboardLED.pin, settings.onboardLED.lampStatus);
    }
  } 
}


void handleTemperaturePeripheral(){

  float minimumDelta = 0.25; //Degrees
  float observedValue = 0;

  //Ensure that the peripheral is enabled
  if(settings.temperaturePeripheral.enabled != true){

    //Disabled, exit
    return;
  }

  //Get the observation
  observedValue = atmosphereSensor.temp(BME280::TempUnit_Celsius);
  
  //If the difference between the last reported value and the observed value is greater than the minimum delta, report the change
  float difference = (settings.temperaturePeripheral.lastReportedValue - observedValue);
 
  formatAndBroadcastSensorDebug("Temperature", String(settings.temperaturePeripheral.lastReportedValue), String(observedValue), String((float)abs(difference*100)/100,2), String(minimumDelta, 2));

  if(abs(difference*100) >= minimumDelta*100 || settings.forceSensorUpdate == true){

    //Publish the new value
    publishMQTT(settings.mqttServer.sensorTopic + "/temperature", String(observedValue, 2));

    //Store the new reported value
    settings.temperaturePeripheral.lastReportedValue = observedValue;
    
  }

}

void handleHumidityPeripheral(){

  float minimumDelta = 0.25; //Percent
  float observedValue = 0;

  //Ensure that the peripheral is enabled
  if(settings.humidityPeripheral.enabled != true){

    //Disabled, exit
    return;
  }

  //Get the observation
  observedValue = atmosphereSensor.hum();

  //If the difference between the last reported value and the observed value is greater than the minimum delta, report the change
  float difference = (settings.humidityPeripheral.lastReportedValue - observedValue);

  formatAndBroadcastSensorDebug("Humidity", String(settings.humidityPeripheral.lastReportedValue), String(observedValue), String((float)abs(difference*100)/100,2), String(minimumDelta, 2));

  if(abs(difference*100) >= minimumDelta*100){

    //Publish the new value
    publishMQTT(settings.mqttServer.sensorTopic + "/humidity", String(observedValue, 2));

    //Store the new reported humidity
    settings.humidityPeripheral.lastReportedValue = observedValue;
    
  }

}


void handleAtmosphericPressurePeripheral(){

  float minimumDelta = 0.01; //inHg
  float observedValue = 0;

  //Ensure that the peripheral is enabled
  if(settings.atmosphericPressurePeripheral.enabled != true){

    //Disabled, exit
    return;
  }

  //Get the observation
  observedValue = atmosphereSensor.pres(BME280::PresUnit_inHg);

  //If the difference between the last reported value and the observed value is greater than the minimum delta, report the change
  float difference = (settings.atmosphericPressurePeripheral.lastReportedValue - observedValue);

  formatAndBroadcastSensorDebug("Pressure", String(settings.atmosphericPressurePeripheral.lastReportedValue), String(observedValue), String((float)abs(difference*100)/100,2), String(minimumDelta, 2));

  if(abs(difference*100) >= minimumDelta*100 || settings.forceSensorUpdate == true){

    //Publish the new value
    publishMQTT(settings.mqttServer.sensorTopic + "/pressure", String(observedValue, 2));

    //Store the new reported value
    settings.atmosphericPressurePeripheral.lastReportedValue = observedValue;
    
  }

}


void handleMotionPeripheral(){

  /*
  Note the absence of a timer in this function.  The AM312 has a built-in 2 second timer.
  */

  //Read the PIR sensor value
  int observedValue = digitalRead(settings.motionPeripheral.pin);
  
  //See if the last reported value is different than the last observed value
  if(settings.motionPeripheral.lastReportedValue != observedValue || settings.motionPeripheral.forceSensorUpdate == true){

    //Publish the new value
    publishMQTT(settings.mqttServer.sensorTopic + "/motion", String(observedValue));

    //Store the new reported motion value
    settings.motionPeripheral.lastReportedValue = observedValue;
    
  }

    //Clear the force update flag
   settings.motionPeripheral.forceSensorUpdate = false;
  
}


void handleLuminancePeripheral(){

  float minimumDelta = 7; //Lux
  float observedValue = 0;

  //Ensure that the luminance peripheral is enabled
  if(settings.luminancePeripheral.enabled != true){

    //Disabled, exit
    return;
  }

  //Get the observation
  observedValue = lightSensor.GetLightIntensity();

  //If the device is not connected, it will return 54612
  if(observedValue == 54612){

    //Disable the device
    settings.luminancePeripheral.enabled = false;

    //Fire a message to the console
    broadcastLine("Unable to Find Sensor BH1750 Disabling Peripheral");

    //Send zero value
    publishMQTT(settings.mqttServer.sensorTopic + "/luminance", "0");
  
    //Disable the device
    return;
  }

  //Give greater resolution below the minimum delta
  if(observedValue <= minimumDelta){
    minimumDelta = 2;
  }

  //If the difference between the last reported value and the observed value is greater than the minimum delta, report the change
  float difference = (settings.luminancePeripheral.lastReportedValue - observedValue);

  formatAndBroadcastSensorDebug("Luminance", String(settings.luminancePeripheral.lastReportedValue, 0), String(observedValue, 0), String((float)abs(difference*100)/100,0), String(minimumDelta, 0));

  if((abs(difference) >= minimumDelta) || settings.forceSensorUpdate == true){

    //Publish the new value
    publishMQTT(settings.mqttServer.sensorTopic + "/luminance", String(observedValue,0));

    //Store the new reported value
    settings.luminancePeripheral.lastReportedValue = observedValue;
    
  }

}


void handleHealth(){

  //Publish the uptime
  publishMQTT(settings.mqttServer.clientTopic + "/uptime", String(millis()));

  String disabledPeripherals = "";

  if(settings.onboardLED.enabled == false){
    disabledPeripherals.concat("onboardLED; ");
  }
  
  if(settings.temperaturePeripheral.enabled != true){
    disabledPeripherals.concat("temperature; ");
  }

  if(settings.humidityPeripheral.enabled != true){
    disabledPeripherals.concat("humidity; ");
  }

  if(settings.atmosphericPressurePeripheral.enabled != true){
    disabledPeripherals.concat("pressure; ");
  }

  if(settings.luminancePeripheral.enabled != true){
    disabledPeripherals.concat("luminance; ");
  }

  disabledPeripherals.trim();

  //See if we have any disabled peripherals
  if(disabledPeripherals != ""){

      //Publish the status with a warning
      publishMQTT(settings.mqttServer.clientTopic + "/status", "Disabled Peripherals: " + disabledPeripherals);
      
    }else{
       
      //Publish the OK status
      publishMQTT(settings.mqttServer.clientTopic + "/status", "OK");
  }

  //Publish the IP Address
  publishMQTT(settings.mqttServer.clientTopic + "/ip", WiFi.localIP().toString());

  //Publish the Firmware version
  publishMQTT(settings.mqttServer.clientTopic + "/firmware", (String)firmwareVersion);

  //Publish the device name
  publishMQTT(settings.mqttServer.clientTopic + "/deviceName", settings.deviceName);

  //Ensure the onboard LED is set
  setLEDMode(settings.onboardLED.mode);

  //Force the peripherals to send updates
  settings.forceSensorUpdate = true;
  
}


void wwwHandleRoot(){
  if (webServer.hasArg("cmdProvision")) {
    wwwHandleSubmit();
  }
  else {

    char temp[2000];

    snprintf(temp, 2000,
    "<!DOCTYPE html>\
    <html>\
      <head>\
        <meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\
        <title>Device Provisioning</title>\
        <style>\
          * {font-family: \"Lucida Sans Unicode\", \"Lucida Grande\", sans-serif; }\
          body {font-size: 1em; color: #81D8D0; background-color: #363636;}\
          a {color: #81D8D0;}\
          input {width: 100%%; height: 30px; font-size: 1em; color: #363636;}\
          #firmwareVersion {font-size: 0.5em; position: fixed; bottom: 0; right: 0; }\
        </style>\
      </head>\
      <body>\
        <p style=\"text-align: center; font-size: 1.5em;\">Room Monitoring Device<p><p style=\"font-size: 1em; text-align: center; color: #d881b1;\">Device Name: %s<br>MAC Address: %s</p>\
        <form name=\"frmMain\" action=\"/\" method=\"POST\">\
        <label>SSID</label><input name=\"txtSSID\" type=\"text\"><br>\
        <label>WPA Key</label><input name=\"txtWpaKey\" type=\"text\"><br>\
        <label>Bootstrap URL (use FQDN)</label><input name=\"txtBootstrapURL\" value=\"http://server/%s.json\" type=\"text\"><br>\
        <br>\
        <center><input name=\"cmdProvision\" value=\"Provision\" type=\"submit\" style=\"width: 50%%; background-color: #81D8D0;\"></center>\
        </form>\
        <div id=\"firmwareVersion\"><a href=\"./update\">Firmware Version: %d</a></div>\
      </body>\
    </html>", settings.deviceName.c_str(), settings.machineMacAddress.c_str(), settings.deviceName.c_str(), firmwareVersion);

    webServer.send(200, "text/html", temp);
  }
}


void wwwHandleSubmit(){

  //See if we're trying to submit for the provisioning page
  if(webServer.hasArg("cmdProvision")){

    String SSID = webServer.arg("txtSSID");
    String wpaKey = webServer.arg("txtWpaKey");
    String bootstrapURL = webServer.arg("txtBootstrapURL");

    //Trim each of the inputs
    SSID.trim();
    wpaKey.trim();
    bootstrapURL.trim();
      
    broadcastLine("Attempting SSID " + SSID + " with WPA Key " + wpaKey + ".\nBootstrap URL: " + bootstrapURL);
    
    webServer.sendHeader("Connection", "close");
    webServer.sendHeader("Access-Control-Allow-Origin", "*");
    webServer.send(200, "text/plain", "OK\r\n");

    //Give the system time to send the data back to the caller before attempting to provision
    delay(1000);
    
    //Attempt to bootstrap
    attemptProvision(SSID, wpaKey, bootstrapURL);
    
    return;
  }

  //if...some other post page


  //If we made it here, we have an unhandled error
  webServer.sendHeader("Connection", "close");
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(400, "text/plain", "Unknown Request\r\n");
    
}
  

void wwwHandleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  for (uint8_t i=0; i<webServer.args(); i++){
    message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }
  webServer.send(404, "text/plain", message);
}


void checkFirmwareUpgrade(){

  //Build the firmware URL
  String firmwareVersionURL = settings.firmwareServer.url;

  //Replace $DEVICENAME$ with the actual device name
  firmwareVersionURL.replace("$DEVICENAME$", settings.deviceName);

  //Create an http client to connect to the firmware server
  HTTPClient httpClient;
  httpClient.begin(firmwareVersionURL);

  int httpCode = httpClient.GET();

  //See if we get an HTTP 200/OK response
  if( httpCode == 200 ) {

    String jsonServerResponse = httpClient.getString();

    //Read the data from the client response
    DynamicJsonBuffer jsonBuffer(1500);

    //Read the EEPROM
    JsonObject& root = jsonBuffer.parseObject(jsonServerResponse);
    
    if (!root.success()) {
      broadcastLine("Unable to parse firmware server response.");
      broadcastLine(jsonServerResponse);
      return;
    }

    //Get the firmware version
    String newFirmwareVersion = root["version"].asString();
    String firmwareBinURL = root["url"].asString();

    broadcastLine(padRight("", 64, "%"));
    broadcastLine("Current firmware version: " + (String)firmwareVersion);
    broadcastLine( "Available firmware version: " + (String)newFirmwareVersion);

    //Check if the version requested is not the same as the one loaded (yes, we allow back flashing)
    if( (String)newFirmwareVersion != (String)firmwareVersion ) {
      broadcastLine( "Preparing to update firmware using " + firmwareBinURL);
      publishMQTT(settings.mqttServer.clientTopic + "/firmwareUpdate", "Updating");
      publishMQTT(settings.mqttServer.clientTopic + "/status", "Updating Firmware");

      //Attempt to retrieve the firmware and perform the update
      t_httpUpdate_return firmwareUpdateResponse = ESPhttpUpdate.update(firmwareBinURL);

      switch(firmwareUpdateResponse) {
        case HTTP_UPDATE_FAILED:
          broadcastLine("HTTP Firmware Update Error: " + (String)ESPhttpUpdate.getLastErrorString().c_str());
          publishMQTT(settings.mqttServer.clientTopic + "/firmwareUpdate", "Failed");
          publishMQTT(settings.mqttServer.clientTopic + "/status", "Firmware Update Failed");
          publishMQTT(settings.mqttServer.clientTopic + "/firmware", (String)firmwareVersion);
          break;

        case HTTP_UPDATE_NO_UPDATES:
          broadcastLine("HTTP_UPDATE_NO_UPDATES");
          break;
      }
    }
    else {
      broadcastLine("Firmware is up-to-date.");
      publishMQTT(settings.mqttServer.clientTopic + "/firmware", (String)firmwareVersion);
    }
  }
  else {
    broadcastLine("Firmware version check failed, got HTTP response code " + (String)httpCode + " while trying to retrieve firmware version from " + (String)firmwareVersionURL);
    publishMQTT(settings.mqttServer.clientTopic + "/firmwareUpdate", "Failed");
    publishMQTT(settings.mqttServer.clientTopic + "/status", "Firmware Update Failed");
    publishMQTT(settings.mqttServer.clientTopic + "/firmware", (String)firmwareVersion);
  }
  
  broadcastLine(padRight("", 64, "%"));
  
  httpClient.end();
}
