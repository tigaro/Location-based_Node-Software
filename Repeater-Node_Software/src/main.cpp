#include "painlessMesh.h"
#include <Arduino.h>
#include "string.h"


#define   MESH_PREFIX     "AirQualityLocationSystem"
#define   MESH_PASSWORD   "7ATMTnyg#!t8!NT&"
#define   MESH_PORT       5555

#define LEDR 26   
#define LEDG 27   
#define LEDB 25   
#define R_channel 0  
#define G_channel 1  
#define B_channel 2  
#define pwm_Frequency 5000   
#define pwm_resolution 8 

#define   CHANNEL         0
#define   FREQ            2000
#define   RES             8
#define   BUZZER          19

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

DynamicJsonDocument ipincomming(1024);

unsigned long prevTime = millis();
unsigned long currentTime = millis();

void mesh_init();
void rgb_init();
void setColor(int R, int G, int B);
void buzzerInit();

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  prevTime = currentTime;
  setColor(0,255,0);
  deserializeJson(ipincomming, msg);
  String ipreceived = ipincomming["ip"];
  if(ipreceived.equals("unknown")){
     ledcWrite(CHANNEL, 255);
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
  mesh_init();
  rgb_init();
}

void loop() {
  
  currentTime = millis();

  if(currentTime - prevTime > 10000){
    setColor(255,0,0);
    Serial.println("Toggle LED");
    prevTime = currentTime;
  }else if(currentTime - prevTime < 1000){
    setColor(0,0,0);
    ledcWrite(CHANNEL, 0);
  }
  mesh.update();
}

void rgb_init(){
  ledcAttachPin(LEDR, R_channel);  
  ledcAttachPin(LEDG, G_channel);  
  ledcAttachPin(LEDB, B_channel);   
  ledcSetup(R_channel, pwm_Frequency, pwm_resolution);  
  ledcSetup(G_channel, pwm_Frequency, pwm_resolution);  
  ledcSetup(B_channel, pwm_Frequency, pwm_resolution); 
}

void setColor(int R, int G, int B) {
  ledcWrite(R_channel, R);   
  ledcWrite(G_channel, G);  
  ledcWrite(B_channel, B);  
}

void buzzerInit(){
  ledcSetup(CHANNEL, FREQ, RES);
  ledcAttachPin(19, CHANNEL);
  ledcWriteTone(CHANNEL, FREQ);
  ledcWrite(CHANNEL, 0);
  }

void mesh_init(){
  //mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
}
