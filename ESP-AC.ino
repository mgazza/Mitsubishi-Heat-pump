#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <MrSlim.h>
#include <FS.h>
#include <ArduinoJson.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define VERSION 1
#define LED_PIN BUILTIN_LED


IPAddress apIP(192, 168, 1, 1);
IPAddress netMsk(255, 255, 255, 0);

WiFiClientSecure _espClient; 

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void logDebug(String message, const char* level){
  ws.textAll(getLogJson(message,level));
}

MrSlim _mrSlim(logDebug,&Serial);

PubSubClient _mqttClient(_espClient);

String apNamePass = String("ESP8266-")+String(ESP.getChipId(),HEX);

void handleControl(JsonObject& root){
        if (root.containsKey("power")){
          const char* power = root["power"];
          if (!_mrSlim.getPower().equalsIgnoreCase(power)){
            logDebug((String("setting power ")+=power).c_str(),"debug");
            _mrSlim.setPower(power);
          }
        }
        if (root.containsKey("mode")){
          const char* mode = root["mode"];
          if (!_mrSlim.getMode().equalsIgnoreCase(mode)){
            logDebug((String("setting mode ")+=mode).c_str(),"debug");
            _mrSlim.setMode(mode);
          }
        }
        if (root.containsKey("setPoint")){
          int setPoint = root["setPoint"];
          logDebug(String("setPoint ")+setPoint,"debug");
          if (_mrSlim.getSetPoint()!=(setPoint)){
            logDebug((String("setting set point ")+=setPoint).c_str(),"debug");
            _mrSlim.setSetPoint(setPoint);
          }
        }
        if (root.containsKey("fan")){
          const char* fanSpeed = root["fan"];
            if (!_mrSlim.getFanSpeed().equalsIgnoreCase(fanSpeed)){
              logDebug((String("setting fan speed ")+=fanSpeed).c_str(),"debug");
              _mrSlim.setFanSpeed(fanSpeed);
            }
        }
        if (root.containsKey("vane")){
          const char* vane = root["vane"];
          if (!_mrSlim.getVane().equalsIgnoreCase(vane)){
            logDebug((String("setting vane ")+=vane).c_str(),"debug");
            _mrSlim.setVane(vane);
          }
        }
        if (root.containsKey("wideVane")){
          const char* wideVane = root["wideVane"];
          if (!_mrSlim.getWideVane().equalsIgnoreCase(wideVane)){
            logDebug((String("setting wide vane ")+=wideVane).c_str(),"debug");
            _mrSlim.setWideVane(wideVane);
          }
        }
}

void handleJson(JsonObject& root){
  if (root.containsKey("control")) {
    JsonObject& control =root["control"];
    handleControl(control);
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->text(getControlJson());
    //client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      //Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info>opcode == WS_TEXT)?"text":"binary", info->len);
      

      
      if(info->opcode == WS_TEXT){
        logDebug("received text message","debug");
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
        logDebug(msg,"debug");
		
      	DynamicJsonBuffer jsonBuffer;
      	JsonObject& root = jsonBuffer.parseObject(msg);

        handleJson(root);
      } else {
        logDebug("received data message","debug");
       return;
      }    
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      logDebug("recieved multiple frames dropping data","debug");
    }
  }
}

void apMode(){
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  delay(100);
    
  char apNamePassBuff[255];

  apNamePass.toCharArray(apNamePassBuff,sizeof(apNamePassBuff));
  WiFi.softAP(apNamePassBuff, apNamePassBuff);

  logDebug("Started AP mode","debug");
}

void tryWiFi(){
 
  long timeout = millis()+(60*1000);

  if (String(_settings.WifiPassword).length()>0){
    WiFi.disconnect(true); 
    
    WiFi.mode(WIFI_STA);
    logDebug((String("Connecting to ")+=_settings.WifiName).c_str(),"debug");
    
    WiFi.begin(_settings.WifiName, _settings.WifiPassword);

    while(millis()<timeout && WiFi.waitForConnectResult() != WL_CONNECTED){
      delay(500);
      
    }
  }
  if (WiFi.status() != WL_CONNECTED){
    //
    logDebug("Failed to connect starting AP mode","debug");
        
    apMode();
  }else{
    logDebug((String("Connected IP:")+=WiFi.localIP()).c_str(),"debug");
    logDebug("Turning off AP mode", "debug");
    WiFi.softAPdisconnect(true);    
  }
  _mqttClient.setServer(_settings.MqttBroker, _settings.MqttPort);
  _mqttClient.setCallback(mqttCallback);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  logDebug(((String("Message arrived [")+=topic)+="] ").c_str(), "debug");
  char buffer[length+1];
  int i;
  for (i=0; i < length; i++) {
    
    buffer[i] = payload[i];
  }
  
  buffer[i]='\0';
  
  JsonObject& root = jsonBuffer.parseObject(json);

  handleJson(root);
}

long lastFailTime;

void tryMqttReconnect() { 
    if (millis()<lastFailTime)
      return;

    logDebug((((String("Attempting MQTT connection...")+=_settings.MqttBroker)+=":")+=_settings.MqttPort).c_str(),"debug");
  
    // Attempt to connect
    if (_mqttClient.connect(apNamePass.c_str(),_settings.MqttUser, _settings.MqttPassword)) {
      logDebug("Mqtt connected","debug");
      // ... and resubscribe
      _mqttClient.subscribe(_settings.MqttSub);
      // .. and publish the state
      /*
      switch(doorState){
        case Open:
          _mqttClient.publish(_settings.MqttPub, "open");
          break;
        case Closed:
          _mqttClient.publish(_settings.MqttPub, "closed");
          break;
      }
      */
    } else {
      logDebug((String("failed, rc="+_mqttClient.state())+" try again in 5 seconds").c_str(),"debug");
      // Wait 5 seconds before retrying
      lastFailTime = millis()+5000;
    }
}

void blink() {
  digitalWrite(LED_PIN, LOW);
  delay(100); 
  digitalWrite(LED_PIN, HIGH);
  delay(100);
}

void setup(){
  Serial.begin(9600);
  SPIFFS.begin();
  _mrSlim.start();
  blink();
  
  //Serial.setDebugOutput(true);

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });
  server.on("/wifiscan", HTTP_GET, handleWifiScan);

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  tryWiFi();
  
  logDebug("Dns server starting","debug");
  MDNS.begin((apNamePass+".local").c_str());
  MDNS.addService("http", "tcp", 80);
 
  
  server.begin();
  logDebug("Web server started","debug");

}

String getControlJson(){
     DynamicJsonBuffer jsonBuffer;
     JsonObject& root = jsonBuffer.createObject();

     JsonObject& control = root.createNestedObject("control");
     control["power"]= _mrSlim.getPower();
     control["mode"]= _mrSlim.getMode();
     control["fan"]= _mrSlim.getFanSpeed();
     control["setPoint"]= _mrSlim.getSetPoint();
     control["vane"]= _mrSlim.getVane();
     control["wideVane"]= _mrSlim.getWideVane();
     control["roomTemp"]= _mrSlim.getRoomTemperature();
     control["faultCode"]= _mrSlim.getFaultCode();
     
     String output;
     root.printTo(output);
     return output;
}

String getLogJson(String message, const char* level){
     DynamicJsonBuffer jsonBuffer;
     JsonObject& root = jsonBuffer.createObject();

     JsonArray& logArray = root.createNestedArray("log");
     JsonObject& item = logArray.createNestedObject();
     
     item["level"]= level;
     item["message"]= message;
    
     String output;
     root.printTo(output);
     return output;
}

void updateWifiStatus(){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& wifiStatus =root.createNestedObject("wifiStatus");
  
  wifiStatus["status"]=WiFi.status();
  wifiStatus["ssid"]=WiFi.SSID();
  wifiStatus["rssi"]=WiFi.RSSI();
  wifiStatus["bssid"]=WiFi.BSSIDstr();
  wifiStatus["channel"]=WiFi.channel();
  wifiStatus["secure"]=WiFi.encryptionType();
  wifiStatus["hidden"]=WiFi.isHidden();

  String output;
  root.printTo(output);
  ws.textAll(output);
}

void onScanComplete(int networksFound){
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& stations= root.createNestedArray("wifiStations");
  
  for(int i=0;i<networksFound;i++){

    JsonObject& item = stations.createNestedObject();
    item["ssid"]=WiFi.SSID(i);
    item["rssi"]=WiFi.RSSI(i);
    item["bssid"]=WiFi.BSSIDstr(i);
    item["channel"]=WiFi.channel(i);
    item["secure"]=WiFi.encryptionType(i);
    item["hidden"]=WiFi.isHidden(i);
  }
  
  String output;
  root.printTo(output);
  ws.textAll(output);
  
  WiFi.scanDelete();
}

void handleWifiScan(AsyncWebServerRequest *request){
  int n = WiFi.scanNetworksAync(onScanComplete);
  request->send(200, "text/plain", "OK");
}

long buttonChangeTime = 0;
bool wasPressed = false;

bool isPressed(){
  return !digitalRead(0); //low is pressed high is not
}

void loop() {

  bool pressed = isPressed();
  if (!wasPressed && pressed){
    wasPressed= true;
  }
  elese
  //a press ended
  if (wasPressed && !pressed){
    wasPressed = false;
  }
   
  _mrSlim.loop();
  if (_mrSlim.getHasChanged()){
    ws.textAll(getControlJson());
  }

  if (WiFi.getMode()==WIFI_AP){
    return;
  }
  
  if (!_mqttClient.connected()) {
    tryMqttReconnect();
  }
  _mqttClient.loop(); 
}

