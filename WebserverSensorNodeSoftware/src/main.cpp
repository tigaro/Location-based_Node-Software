#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "painlessMesh.h"
#include <Arduino.h>
#include "string.h"
#include "Adafruit_BME680.h"
#include <Arduino_JSON.h>
#include "ArduinoJson.h"
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <SPI.h>

#define   MESH_PREFIX     "AirQualityLocationSystem"
#define   MESH_PASSWORD   "7ATMTnyg#!t8!NT&"
#define   MESH_PORT       5555

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 
#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

#define SEALEVELPRESSURE_HPA (1013.25)

#define TEMPERATURE_THRESHHOLD 30
#define HUMIDITY_THRESHHOLD 50
#define PRESSURE_THRESHHOLD 1100
#define GAS_THRESHHOLD 250

#define BLE_INTERVAL_ONOFF 10000
#define MESH_SEND_INTERVAL 5000

#define   CHANNEL         0
#define   FREQ            2000
#define   RES             8
#define   BUZZER          19

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

HardwareSerial WebserverNode(1);
HardwareSerial BluetoothNode(2);
boolean bluetoothStandalone = true;

DynamicJsonDocument incomming(1024);

Adafruit_BME680 bme;

BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String StringForShortBluetoothName = "";
String ip = "unknown";
String receivedStringFromWebserverNode = "";
char placeHolder;

unsigned long prevTime = millis();
unsigned long alarmDuration = millis();

boolean BLEService = false;
boolean alarmStatus = false;
boolean alarmToggled = false;

void TemperatureSensorInit();
String checkPositivNegative(double value);
String prepareString(String value, String datatype, int numberofPositions, boolean negativeCheck);
String stringBuilder(String messageJSON);
void sendMessageTask( void * parameter );
void sendMessage();
void DisplayInit();
void mesh_init();
void bluetooth_init(String sensorDataFromStringBuilder);
float AlarmSetter(float value, int treshhold);
Task taskSendMessage(TASK_SECOND * 1 ,TASK_FOREVER, &sendMessage );
void buzzerInit();

void sendMessage() {

 String msg;
 JSONVar jsonReading;
 String nodeID = "";
 nodeID = mesh.getNodeId();

 if (! bme.performReading()) {
  Serial.println("Failed to perform reading :(");
  return;
 }else{
 alarmStatus = false;
 
 String temperature = String(AlarmSetter(bme.temperature, TEMPERATURE_THRESHHOLD), 5);
 String humidity = String(AlarmSetter(bme.humidity, HUMIDITY_THRESHHOLD), 5);
 String pressure = String(AlarmSetter(bme.pressure/100.0, PRESSURE_THRESHHOLD), 5);
 String altitude = String(bme.readAltitude(SEALEVELPRESSURE_HPA), 5);
 String gas = String(AlarmSetter(bme.gas_resistance/1000.0, GAS_THRESHHOLD), 5);

 jsonReading[nodeID]["nodeID"] = nodeID;
 jsonReading[nodeID]["temperature"] = temperature;
 jsonReading[nodeID]["humidity"] = humidity;
 jsonReading[nodeID]["pressure"] = pressure;
 jsonReading[nodeID]["altitude"] = altitude;
 jsonReading[nodeID]["gas"] = gas;
 jsonReading[nodeID]["ip"] = ip;

 display.clearDisplay();
 display.setTextSize(1);
 display.setTextColor(WHITE);
 display.setCursor(0,0);
 display.print("ID: ");
 display.print(nodeID);

 if(WiFi.RSSI() != 0){
   display.print(" | ");
   display.println(String(WiFi.RSSI()));
 }
 
 display.setCursor(0,9);
 display.print("Temp: ");
 display.println(temperature + " C");
 display.setCursor(0,18);
 display.print("Hum: ");
 display.println(humidity + " %");
 display.setCursor(0,27);
 display.print("Pres: ");
 display.println(pressure + " mPa");
 display.setCursor(0,36);
 display.print("Alt: ");
 display.println(altitude + " m");
 display.setCursor(0,45);
 display.print("Gas: ");
 display.println(gas + " kOhm");
 display.setCursor(0,54);
 display.print("IP: ");
 display.print(ip);
 display.display();

 msg = JSON.stringify(jsonReading[nodeID]);
 StringForShortBluetoothName = msg;

 BluetoothNode.print(msg.c_str());

 mesh.sendBroadcast(msg, true);
 }

 taskSendMessage.setInterval(MESH_SEND_INTERVAL);
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  String nodeID = "";
  nodeID = mesh.getNodeId();
  if(!String(from).equals(nodeID)){
    Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
      WebserverNode.print(msg.c_str());
      Serial.println("Writing to Webserver Node");
  
  }else{
    //Serial.println("Own Message received...sending to Webserver!");
      WebserverNode.print(msg.c_str());
      Serial.println("Writing to Webserver Node");
  }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}


void setup() {
  Serial.begin(115200);

  WebserverNode.begin(115200, SERIAL_8N1, 16, 17);
  WebserverNode.setRxBufferSize(1024);

  if(bluetoothStandalone == true){
    BluetoothNode.begin(115200, SERIAL_8N1, 26, 27);
    BluetoothNode.setRxBufferSize(1024);
  }
  
  buzzerInit();
  DisplayInit();
  TemperatureSensorInit();
  mesh_init();

  xTaskCreatePinnedToCore(sendMessageTask,"SendMessagesOverMesh", 10000, NULL, 0, NULL, 0);
}

void loop() {
  
   unsigned long currentTime = millis();

  while(WebserverNode.available() > 0){
      /*placeHolder = char (WebserverNode.read());
      receivedStringFromWebserverNode += placeHolder;*/
      receivedStringFromWebserverNode = WebserverNode.readStringUntil('\\');
      Serial.println("This is received from the Webserver Node: ?????????????????");
      Serial.println(receivedStringFromWebserverNode);
  }

  if(receivedStringFromWebserverNode.length() > 1){
    if(!ip.equals(receivedStringFromWebserverNode)){
          ip = receivedStringFromWebserverNode;
    }
  }

  if((currentTime - prevTime > BLE_INTERVAL_ONOFF) && bluetoothStandalone == false){
    if(!BLEService){
      Serial.println("Bluetooth starts advertising!");
      String toSend = stringBuilder(StringForShortBluetoothName);
      Serial.println("TO SEND STRING:");
      Serial.println(toSend);
      if(!toSend.equals("empty")){
       bluetooth_init(toSend);
       BLEService = true;
      }else{
        BLEService = false;
      } 
    }else if(BLEService){
      BLEDevice::stopAdvertising();
      Serial.println("Bluetooth stops advertising!");
      BLEService = false;
    }
    prevTime = currentTime;
  }

  if(alarmStatus && (currentTime - alarmDuration > 3000)){
    if(alarmToggled){
      ledcWrite(CHANNEL, 0);
      alarmToggled = false;
    }else{
      ledcWrite(CHANNEL, 255);
      alarmToggled = true;
    }
    alarmDuration = currentTime;
  }
  mesh.update();
}

void DisplayInit(){
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  display.clearDisplay();
}

void bluetooth_init(String sensorDataFromStringBuilder){//11T100H1000P10000A10000G1000
  String nodeID = "NodeID=";
  nodeID += mesh.getNodeId();
  Serial.println(mesh.getNodeId());
  BLEDevice::init(nodeID.c_str());
  BLEAdvertisementData oAdvertisementData1 = BLEAdvertisementData();
  BLEAdvertising *pAdvertising1 = BLEDevice::getAdvertising();
  oAdvertisementData1.setShortName(sensorDataFromStringBuilder.c_str());
  pAdvertising1->setAdvertisementData(oAdvertisementData1);
  BLEDevice::startAdvertising();
}

void mesh_init(){

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
   //userScheduler.addTask( taskSendMessage );
   //taskSendMessage.enable();
}

void sendMessageTask(void * parameter)
{
  for(;;){
      sendMessage();
      Serial.printf("Message sent?!\n");
      vTaskDelay(3000 /portTICK_PERIOD_MS);
  }
}

void buzzerInit(){
  ledcSetup(CHANNEL, FREQ, RES);
  ledcAttachPin(19, CHANNEL);
  ledcWriteTone(CHANNEL, FREQ);
  ledcWrite(CHANNEL, 0);
  }

void TemperatureSensorInit(){

    while (!Serial);
  Serial.println(("BME680 test"));

  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);

      // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
  }
}

float AlarmSetter(float value, int treshhold){
  if(value > treshhold || value < treshhold*-1){
    alarmStatus = true;
  }
  return value;
}


String prepareString(String value, String datatype, boolean negativeCheck){//11T100H1000P10000A10000G1000
    String  composedString = "";
    boolean temperatureNegative = false;
    boolean altitudeNegative = false;
    
    double  doubleValue = value.toDouble();
    
    //Checks the first two chars in the code
    if(datatype.equals("temperature")){ //works
        composedString += checkPositivNegative(doubleValue);
        if(composedString.equals("1")){
            temperatureNegative = true;
            if(negativeCheck == true){
                return composedString;
            }else{
                composedString = "";
            }
        }else{
          temperatureNegative = false;
            if(negativeCheck == true){
                return composedString;
            }else{
                composedString = "";
            }
        }
    }else if(datatype.equals("altitude")){ //works
        composedString += checkPositivNegative(doubleValue);
        if(composedString.equals("1")){
            altitudeNegative = true;
            if(negativeCheck == true){
                return composedString;
            }else{
                composedString = "";
            }
        }else{
          altitudeNegative = false;
            if(negativeCheck == true){
                return composedString;
            }else{
                composedString = "";
            }
        }
    }

    //checks the gas in the code
    if(datatype.equals("gas")){ //works
        int numbers = 4;
        if(doubleValue >= 100){ //works
          for(int i = 0; i <= numbers; i++){ 
                if(i != 3){
                    composedString += value.charAt(i);
                }
            }
            return "G" + composedString;
        }else if(doubleValue >= 10){//works
            for(int i = 0; i <= numbers; i++){ 
                if(i != 2){
                    composedString += value.charAt(i);
                }
            }
            return "g" + composedString;
        }else{
            for(int i = 0; i <= numbers; i++){ //works
                if(i != numbers-3){ 
                    composedString += value.charAt(i);
                }
            }
            return "u" + composedString;
        }
        return "G0000";
    }
      
    //checks the pressure in the code
    if(datatype.equals("pressure")){ //works
        int numbers = 5;
        if(doubleValue >= 1000){
          for(int i = 0; i <= numbers; i++){ //works
                if(i != 4){
                    composedString += value.charAt(i);
                }
            }
            return "P" + composedString;
        }else if(doubleValue >= 100){
          for(int i = 0; i <= numbers; i++){ //works
                if(i != 3){
                    composedString += value.charAt(i);
                }
            }
            return "p" + composedString;
        }else if(doubleValue >= 10){
          for(int i = 0; i <= numbers; i++){ //works
                if(i != 2){
                    composedString += value.charAt(i);
                }
            }
            return "c" + composedString;
        }else if(doubleValue < 10){
            for(int i = 0; i <= numbers; i++){ //works
                if(i != 1){
                    composedString += value.charAt(i);
                }
            }
            return "d" + composedString;
        }
        return "P00000";
    }
    
    //checks the humidity in the code
    if(datatype.equals("humidity")){ //works
        int numbers = 4;
        if(doubleValue >= 100){ //works
          return "X1000";
        }else if(doubleValue < 10){
            for(int i = 0; i <= numbers; i++){ //works
                if(i != 1){
                    composedString += value.charAt(i);
                }
            }
            return "h" + composedString;
        }else{
            for(int i = 0; i <= numbers; i++){ //works
                if(i != numbers-2){ 
                    composedString += value.charAt(i);
                }
            }
            return "H" + composedString;
        }
        return "H0000";
    }
  
    //checks temperature in the code T100/t100
    if(datatype.equals("temperature") && temperatureNegative == false){ //works
        int numbers = 3;
        if(doubleValue < 10){
            for(int i = 0; i <= numbers; i++){ //works
                if(i != 1){
                    composedString += value.charAt(i);
                }
            }
            return "t" + composedString;
        }else{
            for(int i = 0; i <= numbers; i++){ //works
                if(i != numbers-1){
                    composedString += value.charAt(i);
                }
            }
            return "T" + composedString;
        }
        return "T000";
    }
    
    if(datatype.equals("temperature") && temperatureNegative == true){ //works
        int numbers = 4;
        
        if(doubleValue > -10){
  
           for(int i = 1; i <= numbers; i++){ //works
              if(i != 2){
                  composedString += value.charAt(i);
              }
           }
           return "t" + composedString;
        }else{
            for(int i = 1; i <= numbers; i++){ //works
                if(i != 3){
                    composedString += value.charAt(i);
                }
            }
            return "T" + composedString;
        }
        return "T000";
    }
    
    //checks altitude in the code
   if(datatype.equals("altitude") && altitudeNegative == false){ //works
        int numbers = 5;
        if(doubleValue >= 1000){
          for(int i = 0; i <= numbers; i++){ //works
                if(i != 4){
                    composedString += value.charAt(i);
                }
            }
            return "A" + composedString;
        }else if(doubleValue >= 100){
          for(int i = 0; i <= numbers; i++){ //works
                if(i != 3){
                    composedString += value.charAt(i);
                }
            }
            return "a" + composedString;
        }else if(doubleValue >= 10){
          for(int i = 0; i <= numbers; i++){ //works
                if(i != 2){
                    composedString += value.charAt(i);
                }
            }
            return "y" + composedString;
        }else if(doubleValue < 10){
            for(int i = 0; i <= numbers; i++){ //works
                if(i != 1){
                    composedString += value.charAt(i);
                }
            }
            return "z" + composedString;
        }
        return "A00000";
    }
    
    if(datatype.equals("altitude") && altitudeNegative == true){ //works
        int numbers = 5;
        if(doubleValue <= -1000){
          for(int i = 1; i <= numbers+1; i++){ //works
                if(i != 5){
                    composedString += value.charAt(i);
                }
            }
            return "A" + composedString;
        }else if(doubleValue <= -100){
          for(int i = 1; i <= numbers+1; i++){ //works
                if(i != 4){
                    composedString += value.charAt(i);
                }
            }
            return "a" + composedString;
        }else if(doubleValue <= -10){
          for(int i = 1; i <= numbers+1; i++){ //works
                if(i != 3){
                    composedString += value.charAt(i);
                }
            }
            return "y" + composedString;
        }else if(doubleValue > -10){ //
            for(int i = 1; i <= numbers+1; i++){ //works
                if(i != 2){
                    composedString += value.charAt(i);
                }
            }
            return "z" + composedString;
        }
        return "A00000";
  }
  return "";
}

String checkPositivNegative(double value){
    if(value < 0){
          return "1";
    }else{
      return "0";
    }
}

String stringBuilder(String messageJSON){
  if(!messageJSON.equals("")){
    String bluetoothMessage = "";
    String testStringagainst = "11T100H1000P10000A10000G1000";
    deserializeJson(incomming, messageJSON);
    String temperature = incomming["temperature"];
    String humidity = incomming["humidity"];
    String pressure = incomming["pressure"];
    String altitude = incomming["altitude"];
    String gas = incomming["gas"];
    Serial.println("Check Temperature +-");
    Serial.println(bluetoothMessage += prepareString(temperature, "temperature", true));
    Serial.println("Check Altitude +-");
    Serial.println(bluetoothMessage += prepareString(altitude, "altitude", true));
    Serial.println("Temperature Reading:");
    Serial.println(bluetoothMessage += prepareString(temperature, "temperature", false));
    Serial.println("Humdity Reading:");
    Serial.println(bluetoothMessage += prepareString(humidity, "humidity", false));
    Serial.println("Pressure Reading:");
    Serial.println(bluetoothMessage += prepareString(pressure, "pressure", false));
    Serial.println("Altitude Reading:");
    Serial.println(bluetoothMessage += prepareString(altitude, "altitude", false));
    Serial.println("Gas Reading:");
    Serial.println(bluetoothMessage += prepareString(gas, "gas", false));
    Serial.println("FINAL STRING TO BLUETOOTH");
    Serial.println(bluetoothMessage);
    Serial.println("11T100H1000P10000A10000G1000");
    if(bluetoothMessage.length() == testStringagainst.length()){
      Serial.println("TRUEEEEEEEEEEEEEEEEEE");
    }
    return bluetoothMessage;
  }else{
    return "empty";
  }
}
