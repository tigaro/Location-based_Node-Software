#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Adafruit_Sensor.h>

char test[1024];
char buffer; 
String input = "";
char input2[1024] = "";
int j_son = 1;
char sssid[37];
DynamicJsonDocument incomming(1024);
DynamicJsonDocument outgoing(1024);

HardwareSerial SerialNode2(1);

String hostname = "ESP32 Webserver";
const char* ssid = "FRITZ!Box 7590 KS";
const char* password = "67839252783532526727";

AsyncWebServer server(80);
AsyncEventSource events("/events");
String nodeforwebserver = "";
String url = "";

String currentNode = "2125752025";
char nodeIDs[10][10] = {"0"}; 

boolean found = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>An Interconnected Air Quality Sensor Network with Webserver capabilities for Indoor Applications </title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Calibri; display: inline-block; text-align: center;}
    p {  font-size: 1.1rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #777799; color: black; font-size: 2rem; }
    .content { padding: 30px; }
    .content2 { padding: 0px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .cards2 { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.nodeID { color: #DC143C}
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    .card.pressure { color: #3fca6b; }
    .card.gas { color: #d62246; }
    .card.altitude {color: #8A2BE2}
    .card.activenodes {color: #8A2BE1}
  </style>
</head>
<body>
  <div class="topnav">
    <h3>Location-based Air Quality System </h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card nodeID">
          <h4><i class="fab fa-node"></i> NODEID</h4><p><span class="reading"><span id="node">...</p>
        </div>
        <div class="card altitude">
          <h4><i class="fas fa-mountain"></i> ALTITUDE</h4><p><span class="reading"><span id="alt">...</span> m</span></p>
        </div>
        <div class="card temperature">
          <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p><span class="reading"><span id="temp">...</span> &deg;C</span></p>
        </div>
        <div class="card humidity">
          <h4><i class="fas fa-tint"></i> HUMIDITY</h4><p><span class="reading"><span id="hum">...</span> &percnt;</span></p>
        </div>
        <div class="card pressure">
          <h4><i class="fas fa-angle-double-down"></i> PRESSURE</h4><p><span class="reading"><span id="pres">...</span> mPa</span></p>
        </div>
        <div class="card gas">
          <h4><i class="fas fa-wind"></i> GAS</h4><p><span class="reading"><span id="gas">...</span> k&ohm;</span></p>
       	</div>
	  </div>
   </div> 
   <div class="content2">
    <div class="cards2">
    	<div class="card activenodes">
       		<h4>ACTIVE NODES</h4><p style="color:black"><span class="reading"><span id="activenodes">...</span></p>
      	</div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);

 source.addEventListener('nodeid', function(e) {
  console.log("nodeid", e.data);
  document.getElementById("node").innerHTML = e.data;
 }, false);
 
 source.addEventListener('altitude', function(e) {
  console.log("altitude", e.data);
  document.getElementById("alt").innerHTML = e.data;
 }, false);

 source.addEventListener('temperature', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp").innerHTML = e.data;
 }, false);
 
 source.addEventListener('humidity', function(e) {
  console.log("humidity", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);
 
 source.addEventListener('pressure', function(e) {
  console.log("pressure", e.data);
  document.getElementById("pres").innerHTML = e.data;
 }, false);
 
 source.addEventListener('gas', function(e) {
  console.log("gas", e.data);
  document.getElementById("gas").innerHTML = e.data;
 }, false);

 source.addEventListener('activenodes', function(e) {
  console.log("activenodes", e.data);
  document.getElementById("activenodes").innerHTML = e.data;
 }, false);
}
</script>
</body>
</html>)rawliteral";

const char index_html_processor[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>An Interconnected Air Quality Sensor Network with Webserver capabilities for Indoor Applications </title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #777799; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.nodeID { color: #DC143C}
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    .card.pressure { color: #3fca6b; }
    .card.gas { color: #d62246; }
    .card.altitude {color: #8A2BE2}
  </style>
</head>
<body>
  <div class="topnav">
    <h3>An Interconnected Air Quality Sensor Network with Webserver capabilities for Indoor Applications </h3>
  </div>
  <div class="content">
    <div class="cards">
    <div class="card nodeID">
        <h4><i class="fab fa-node"></i> NODEID</h4><p><span class="reading"><span id="node">%NODEID%</p>
      </div>
      <div class="card altitude">
        <h4><i class="fas fa-mountain"></i> ALTITUDE</h4><p><span class="reading"><span id="alt">%ALTITUDE%</span> m</span></p>
      </div>
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p><span class="reading"><span id="temp">%TEMPERATURE%</span> &deg;C</span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> HUMIDITY</h4><p><span class="reading"><span id="hum">%HUMIDITY%</span> &percnt;</span></p>
      </div>
      <div class="card pressure">
        <h4><i class="fas fa-angle-double-down"></i> PRESSURE</h4><p><span class="reading"><span id="pres">%PRESSURE%</span> mPa</span></p>
      </div>
      <div class="card gas">
        <h4><i class="fas fa-wind"></i> GAS</h4><p><span class="reading"><span id="gas">%GAS%</span> k&ohm;</span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);

 source.addEventListener(%nodeID%, function(e) {
  console.log("nodeid", e.data);
  document.getElementById("node").innerHTML = e.data;
 }, false);
 
 source.addEventListener(%altitude%, function(e) {
  console.log("altitude", e.data);
  document.getElementById("alt").innerHTML = e.data;
 }, false);

 source.addEventListener(%temperature%, function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp").innerHTML = e.data;
 }, false);
 
 source.addEventListener(%humidity%, function(e) {
  console.log("humidity", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);
 
 source.addEventListener(%pressure%, function(e) {
  console.log("pressure", e.data);
  document.getElementById("pres").innerHTML = e.data;
 }, false);
 
 source.addEventListener(%gas%, function(e) {
  console.log("gas", e.data);
  document.getElementById("gas").innerHTML = e.data;
 }, false);
}
</script>
</body>
</html>)rawliteral";

String processor(const String& var){
  if(var == "event"){
    return "'/event2'";
  }else if(var == "nodeID"){
    return "\'" + currentNode + "\'";
  }else if(var == "temperature"){
    return "\'temperature" + currentNode + "\'";
  }else if(var == "humidity"){
    return "\'humidity" + currentNode + "\'";
  } else if(var == "altitude"){
    return "\'altitude" + currentNode + "\'";
  } else if(var == "gas"){
    return "\'gas" + currentNode + "\'";
  } else if(var == "altitude"){
    return "\'altitude" + currentNode + "\'";
  } else if(var == "pressure"){
    return "\'pressure" + currentNode + "\'";
  }
  Serial.println(var);
  return "0";
}
void setup() {

  uint64_t chipid = ESP.getEfuseMac(); 
  uint16_t chip = (uint16_t)(chipid >> 32);

  snprintf(sssid, 37, "WS-MCUDEVICE-%04X%08X", chip, (uint32_t)chipid);

  Serial.begin(115200);
  SerialNode2.begin(115200, SERIAL_8N1, 16, 17);
  SerialNode2.setRxBufferSize(1024);

  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(sssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
 
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}

void loop() {

  Serial.println("Running"); 
  while(SerialNode2.available() > 0)
  {
      buffer = char (SerialNode2.read());
      input += buffer;
      strncat(input2, &buffer, 1);
  }

  if(input.length() > 1 && j_son == 0){
    Serial.println("Node 1: Message received: ");
    Serial.println(input);
    input = "";
  }

    if(input.length() > 1 && j_son == 1){
    Serial.println("Node 1: Dezirialized Message received: ");

    deserializeJson(incomming, input);
    int nodeID = incomming["nodeID"];
    String nodeIDforCheck = incomming["nodeID"];
    float temperature = incomming["temperature"];
    float humidity = incomming["humidity"];
    float pressure = incomming["pressure"];
    float altitude = incomming["altitude"];
    float gas = incomming["gas"];

    Serial.println(nodeID);
    Serial.println(temperature);
    Serial.println(humidity);
    Serial.println(pressure);
    Serial.println(altitude);
    Serial.println(gas);

    events.send("ping",NULL,millis());
    events.send(String(temperature).c_str(),"temperature",millis());
    events.send(String(humidity).c_str(),"humidity",millis());
    events.send(String(pressure).c_str(),"pressure",millis());
    events.send(String(gas).c_str(),"gas",millis());
    events.send(String(nodeID).c_str(),"nodeid",millis());
    events.send(String(altitude).c_str(),"altitude",millis());
    events.send(String(input).c_str(),"sensor",millis());

    for(int j = 0; j < sizeof(nodeIDs); j++){
      String checker = "";
      for(int y = 0; y < sizeof(nodeIDs[j]); y++){ 
        checker += nodeIDs[j][y];
        if(checker.equals(nodeIDforCheck)){
          found = true;
          String temperatureID = "temperature" + nodeIDforCheck;
          String humidityID = "humidity" + nodeIDforCheck;
          String altitudeID = "altitude" + nodeIDforCheck;
          String pressureID = "pressure" + nodeIDforCheck;
          String gasID = "gas" + nodeIDforCheck;
          String NodeIDID = nodeIDforCheck;
          events.send(String(temperature).c_str(),temperatureID.c_str(),millis()); 
          events.send(String(humidity).c_str(),humidityID.c_str(),millis()); 
          events.send(String(altitude).c_str(),altitudeID.c_str(),millis()); 
          events.send(String(pressure).c_str(),pressureID.c_str(),millis()); 
          events.send(String(gas).c_str(),gasID.c_str(),millis()); 
          events.send(String(NodeIDID).c_str(),NodeIDID.c_str(),millis()); 
          break;
        }
      }
      if(found == true){
        Serial.println("List:");
        Serial.println(checker);
        break;
      }
      
    }

    String activenodes = "";
    for(int j = 0; j < sizeof(nodeIDs); j++){
      if(nodeIDs[j][0] == '\0'){
        break;
      }else{
        activenodes += "http://";
        activenodes += WiFi.localIP().toString();
        activenodes += "/";
        for(int y = 0; y < sizeof(nodeIDs[j]); y++){ 
        activenodes += nodeIDs[j][y]; 
      }
      activenodes += "\n";
      }
    }

    if(!activenodes.equals("")){
      events.send(activenodes.c_str(),"activenodes",millis());
    }

    if(found == false){
      int counterholder = 0;
     for(int k = 0; k < sizeof(nodeIDs); k++){
       //Serial.println(nodeIDs[k]);
       if(nodeIDs[k][1] == '\0'){
        for(int h = 0; h < sizeof(nodeIDs[k]) || nodeIDforCheck.charAt(h) != '\0'; h++){
          nodeIDs[k][h] = nodeIDforCheck.charAt(h);
          counterholder = h;
         } 
         nodeIDs[k][counterholder+1] = '\0';
         Serial.println("Added to list and Starting webserver");
         currentNode = nodeIDforCheck;
         nodeforwebserver = nodeIDforCheck;
         url = "/" + nodeIDforCheck;
         Serial.println("URL:");
         Serial.println(url);

         server.on(url.c_str(), HTTP_GET, [](AsyncWebServerRequest *request){
         
          Serial.print("This is the request URL:");
          Serial.println(request->url().c_str());
          
          String requestedUrl = request->url().c_str();
         
          requestedUrl.remove(0,1);

          Serial.print("New requested String to forward: ");
          Serial.println(requestedUrl);

          currentNode = requestedUrl;

          request->send_P(200, "text/html", index_html_processor, processor);
         });
         break;
       }
     }
    }else {
      Serial.println("Already in list");
    } 
    
    found = false;
    input = "";
    input2[0] = '\0';
  }

    if(j_son == 0){
    Serial.println("Node 1: Sending message...");
    Serial.println(SerialNode2.write("Node 2: This is the message from the other node"));
    }

    if(j_son == 1){
    Serial.println("Node 1: Sending message...");
    outgoing["nodeID"] = 2125752025;
    //(float)rand()/(float)(RAND_MAX)) * 25;
    outgoing["temperature"] = (float)rand()/(float)(RAND_MAX) * 25;
    outgoing["humidity"] = (float)rand()/(float)(RAND_MAX) * 45;
    outgoing["pressure"] = (float)rand()/(float)(RAND_MAX) * 1000;
    outgoing["altitude"] = (float)rand()/(float)(RAND_MAX) * 270;
    outgoing["gas"] = (float)rand()/(float)(RAND_MAX) * 60;

    serializeJson(outgoing, test);
    Serial.println(test);
    SerialNode2.write(test);
    }
  delay(3000);
}
