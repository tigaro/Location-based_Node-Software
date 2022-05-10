#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "painlessMesh.h"
#include <Arduino.h>
#include "string.h"


#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

DynamicJsonDocument incomming(1024);
String msg1 = "{\"nodeiD\":2090128309,\"temperature\":1.1982,\"humidity\":1.80025,\"pressure\":1.8243,\"altitude\":1.79082,\"gas\":7.638}";

BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

unsigned long prevTime = millis();

int i = 0;

String checkPositivNegative(double value);
String prepareString(String value, String datatype, int numberofPositions, boolean negativeCheck);
String stringBuilder(String messageJSON);
void sendMessageTask( void * parameter );
void sendMessage();
void mesh_init();
void bluetooth_init(String sensorDataFromStringBuilder);
Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() {

  String msg = "";
  String test = "T100H100P1000A1000G100";
  msg = "{\"nodeID\":2127837725,\"temperature\":25.12,\"humidity\":44.85,\"pressure\":981.82,\"altitude\":265.79\"gas\":67.68}"; 

  mesh.sendBroadcast( msg, true );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());

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
  Serial.println("Starting BLE work!");

  mesh_init();
  //bluetooth_init();

  xTaskCreatePinnedToCore(sendMessageTask,"SendMessagesOverMesh", 10000, NULL, 0, NULL, 0);
  //mesh.stop();


}

void loop() {
  
   unsigned long currentTime = millis();

  if(currentTime - prevTime > 10000){

    if(i == 0)
    {
      Serial.println("Bluetooth starts advertising!");
      //BLEDevice::startAdvertising();
      String toSend = stringBuilder(msg1);
      Serial.println("TO SEND STRING:");
      Serial.println(toSend);
      bluetooth_init(toSend);
      i = 1;
    }
    else if(i == 1){
      BLEDevice::stopAdvertising();
      if(msg1.equals("{\"nodeiD\":2090128309,\"temperature\":1.1982,\"humidity\":1.80025,\"pressure\":1.8243,\"altitude\":1.79082,\"gas\":7.638}")){
        msg1 = "{\"nodeiD\":2090128309,\"temperature\":-10.1982,\"humidity\":111.80025,\"pressure\":111.8243,\"altitude\":-101.79082,\"gas\":227.638}";
      }else{
        msg1 = "{\"nodeiD\":2090128309,\"temperature\":1.1982,\"humidity\":1.80025,\"pressure\":1.8243,\"altitude\":1.79082,\"gas\":7.638}";
      }
      Serial.println("Bluetooth stops advertising!");
      i = 0;
    }

    prevTime = currentTime;

  }

  mesh.update();
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
}




