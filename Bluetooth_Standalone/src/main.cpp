//This is the software of the Bluetooth-Module
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>
#include "string.h"
#include "ArduinoJson.h"
#include <Wire.h>
#include <SPI.h>

HardwareSerial NodeInformation(1);
DynamicJsonDocument incomming(1024);
String input = "";

String nodeID = "";
float temperature = 0;
float humidity = 0;
float pressure = 0;
float altitude = 0;
float gas = 0;

BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

String checkPositivNegative(double value);
String prepareString(String value, String datatype, int numberofPositions, boolean negativeCheck);
String stringBuilder(String messageJSON);
void bluetooth_init(String sensorDataFromStringBuilder, String nodeIDfromMessage);


void setup() {
  NodeInformation.begin(115200, SERIAL_8N1, 16, 17);
  NodeInformation.setRxBufferSize(1024);
 
  // put your setup code here, to run once:
}

void loop() {

while(NodeInformation.available() > 0){
  input = NodeInformation.readStringUntil('}');
}

if(input.length() > 1 && input[0] == '{'){
  Serial.print("Free Heap:");
  Serial.println(ESP.getFreeHeap());

  input += '}';
  Serial.println("Message from Sensor Node:");
  Serial.println(input);
  
  deserializeJson(incomming, input);
  nodeID = incomming["nodeID"].as<String>();

  Serial.println(nodeID);
  Serial.println(temperature);
  Serial.println(humidity);
  Serial.println(pressure);
  Serial.println(altitude);
  Serial.println(gas);
  
  BLEDevice::stopAdvertising();
  delay(100);
  String toSend = stringBuilder(input);
  bluetooth_init(toSend, nodeID);

  }
}

void bluetooth_init(String sensorDataFromStringBuilder, String nodeIDfromMessage){//11T100H1000P10000A10000G1000
  String nodeID = "NodeID=";
  nodeID += nodeIDfromMessage;
  BLEDevice::init(nodeID.c_str());
  BLEAdvertisementData oAdvertisementData1 = BLEAdvertisementData();
  BLEAdvertising *pAdvertising1 = BLEDevice::getAdvertising();
  oAdvertisementData1.setShortName(sensorDataFromStringBuilder.c_str());
  pAdvertising1->setAdvertisementData(oAdvertisementData1);
  BLEDevice::startAdvertising();
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