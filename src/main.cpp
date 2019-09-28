#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <FirebaseArduino.h>
#include <ezTime.h>
#include <PubSubClient.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#define FIREBASE_HOST "jspyn-39604.firebaseio.com"
#define FIREBASE_AUTH "mjEYJ6hQCMh2Ahj04RQpeqL9TXoQZ1hIM2eBNP7c"
#define ONE_WIRE_BUS D6
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
uint16_t count = 59;
HTTPClient http;
WiFiClient espClient;
ESP8266WebServer server(80);
PubSubClient client(espClient);
const char* mqtt_server = "35.200.218.116";

struct myObject{
  bool flag = false;
  char API_KEY_CHAR[50];
  char SSID_CHAR[50];
  char PASSWORD_CHAR[50]; 
  char SENSOR_TYPE_CHAR[50];
  char UID_CHAR[100];
  char SENSOR_NAME_CHAR[50];
};

myObject objectOne, objectTwo;

void handleLogin(){  
  if (server.hasArg("SSID") && server.hasArg("PASSWORD") && server.hasArg("API_KEY")){
      String API_KEY, SENSOR_TYPE, UID, SENSOR_NAME, SSID, PASSWORD ;
      SSID = server.arg("SSID");
      PASSWORD = server.arg("PASSWORD");
      API_KEY = server.arg("API_KEY");
      SENSOR_TYPE = server.arg("SENSOR_TYPE");
      UID = server.arg("UID");
      SENSOR_NAME = server.arg("SENSOR_NAME");
      

      SSID.toCharArray(objectOne.SSID_CHAR, SSID.length()+1);
      PASSWORD.toCharArray(objectOne.PASSWORD_CHAR, PASSWORD.length()+1);
      API_KEY.toCharArray(objectOne.API_KEY_CHAR, API_KEY.length()+1);
      SENSOR_TYPE.toCharArray(objectOne.SENSOR_TYPE_CHAR, SENSOR_TYPE.length()+1);
      UID.toCharArray(objectOne.UID_CHAR, UID.length()+1);
      SENSOR_NAME.toCharArray(objectOne.SENSOR_NAME_CHAR, SENSOR_NAME.length()+1);
      objectOne.flag = true;

      //WiFi.begin(objectOne.SSID_CHAR,objectOne.PASSWORD_CHAR);
      
      EEPROM.put(100, objectOne);
      EEPROM.commit();
      server.send(200, "text/plain", "success");
  }
}

uint8_t getEspPin(int pin){
  switch (pin){
    case 0:
      return D0;
      break;

    case 1:
      return D1;
      break;

    case 2:
      return D2;
      break;

    case 3:
      return D3;
      break;

    case 4:
      return D4;
      break;

    case 5:
      return D5;
      break;

    case 6:
      return D6;
      break;

    case 7:
      return D7;
      break;

    case 8:
      return D8;
      break;

    case 9:
      return D9;
      break;

    case 10:
      return D10;
      break;

    default:
      return pin;
      break;
  }

}

void callback(char* topic, byte* payload, int length) {
  String response = "";
  for (int i = 0; i < length; i++) {
    response += (char)payload[i];
  }
  Serial.println(response);
  if (response[0] == 'R') {
    uint8_t pinMqtt = response[6] - 48;
    int state = response[7]-48;
    uint8_t pin = getEspPin(pinMqtt);
    digitalWrite(pin,!state);
  }
  else if(response[0] == 'V')
  {
    uint8_t pinMqtt = response[4] - 48;
    int state = (response[5]-48)*100;
        state += (response[6]-48) * 10;
        state += response[7]-48;
    uint8_t pin = getEspPin(pinMqtt+4);
    analogWrite(pin, state);
  }
  //client.publish(objectTwo.API_KEY_CHAR, "REC");
  response = "";
}

void reconnect(char* API) {
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    String clientId = objectTwo.API_KEY_CHAR;
    clientId += String(random(0xfff),HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(API);  
    } 
    else {
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup(){
  EEPROM.begin(512);
  Serial.begin(9600);
  sensors.begin(); 
  pinMode(D0, OUTPUT);//reserved        /|/\/\/
  pinMode(D1, OUTPUT);//relay_1        /|/   \/
  pinMode(D2, OUTPUT);//relay_2       /|/   |/
  pinMode(D3, OUTPUT);//relay_3      /|/|/|/
  pinMode(D4, OUTPUT);//relay_4     /|/
  pinMode(D5, OUTPUT);//PWM        /|/
  pinMode(D6, OUTPUT);//PWM       /|/
  pinMode(D7, OUTPUT);//reserved /|/
  pinMode(D8, INPUT);//reserved //Setup Mode Button
  pinMode(A0, INPUT);//Sensor 
  digitalWrite(D0, LOW);
  digitalWrite(D1, HIGH);
  digitalWrite(D2, HIGH);
  digitalWrite(D3, HIGH);
  digitalWrite(D4, HIGH);
  digitalWrite(D5, LOW);
  digitalWrite(D6, LOW);
  digitalWrite(D7, HIGH);
  delay(3000);
  digitalWrite(D7, LOW);

  if(digitalRead(D8) == HIGH){
    WiFi.mode(WIFI_AP);

    WiFi.softAP("JSPYN_IOT", "");
    server.on("/login", handleLogin);
    const char * headerkeys[] = {"User-Agent","Cookie"} ;
    size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
    server.collectHeaders(headerkeys, headerkeyssize );
    server.begin();
    delay(3000); 
    digitalWrite(D7, HIGH);
  }
  else if (digitalRead(D8) == LOW){
    WiFi.mode(WIFI_STA);
    WiFi.hostname("jspyniot");
    EEPROM.get(100,objectTwo);
    Serial.println(objectTwo.SSID_CHAR);
    Serial.println(objectTwo.PASSWORD_CHAR);
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    delay(3000);
    WiFi.begin(objectTwo.SSID_CHAR,objectTwo.PASSWORD_CHAR);
    while(WiFi.status() != WL_CONNECTED)
    {
      digitalWrite(D7, HIGH);
      delay(1000);
      digitalWrite(D7, LOW);
      delay(1000);
    }
    digitalWrite(D7, LOW);
    digitalWrite(D0, HIGH);
    Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
    //Firebase.stream("/GPIO/"+String(objectTwo.API_KEY_CHAR));  
    delay(1000);
    
    String to = Firebase.getString("/smartHome/"+String(objectTwo.UID_CHAR)+"/FCMID/result/token");
    delay(1000);
    http.begin("http://fcm.googleapis.com/fcm/send");
    http.addHeader("Authorization"," key=AIzaSyBikTXTTJSIH_PBfz7IakN8RKaTwfB4ULk");
    http.addHeader("Content-Type","application/json");
    http.POST("{\"notification\":{\"title\":\"JSPYN-IOT Board\",\"body\":\"Hey, I am configured to network "+String(objectTwo.SSID_CHAR)+"\"},\"to\":\""+to+"\"}");
    http.end();
    delay(1000);
    http.begin("http://us-central1-jspyn-39604.cloudfunctions.net/jspynio/transmit/sensor");
    http.addHeader("Content-Type","application/json");
    // http.begin("http://us-central1-jspyn-39604.cloudfunctions.net/jspynio/sensor");
    // http.addHeader("Content-Type","application/json");
  
  
    Serial.println(objectTwo.API_KEY_CHAR);
    Serial.println(objectTwo.SENSOR_NAME_CHAR);
    Serial.println(objectTwo.SSID_CHAR);
    Serial.println(objectTwo.PASSWORD_CHAR);
    Serial.println(objectTwo.SENSOR_TYPE_CHAR); 
    Serial.println(objectTwo.UID_CHAR);
    Serial.println("_______________________________________________________________");

    
  }    
}

void loop(){
  server.handleClient();
  while(WiFi.status() == WL_CONNECTED)
  {  
    if (!client.connected()) {
      reconnect(objectTwo.API_KEY_CHAR);
    }
    client.loop();

    if (String(objectTwo.UID_CHAR)!="Select Sensor") {
        if(count == 60){
          Timezone India;
          India.setPosix("IST-5:30");
          waitForSync();
          //sensorCloud(India.dateTime("d-m-Y"), India.dateTime("H:i"), analogRead(A0));
          if (String(objectTwo.SENSOR_TYPE_CHAR)=="Flame-Sensor") {
            http.POST("{\"uid\":\""+String(objectTwo.UID_CHAR)+"\",\"sensorName\":\""+String(objectTwo.SENSOR_TYPE_CHAR)+"\",\"sensorLocation\":\""+String(objectTwo.SENSOR_NAME_CHAR)+"\",\"date\":\""+India.dateTime("d-m-Y")+"\",\"time\":\""+India.dateTime("H:i")+"\",\"value\":\""+digitalRead(A0)+"\"}");
          }
          else if(String(objectTwo.SENSOR_TYPE_CHAR)=="Temperature-Sensor-18B20"){
            digitalWrite(D5, HIGH);
            sensors.requestTemperatures();
            http.POST("{\"uid\":\""+String(objectTwo.UID_CHAR)+"\",\"sensorName\":\""+String(objectTwo.SENSOR_TYPE_CHAR)+"\",\"sensorLocation\":\""+String(objectTwo.SENSOR_NAME_CHAR)+"\",\"date\":\""+India.dateTime("d-m-Y")+"\",\"time\":\""+India.dateTime("H:i")+"\",\"value\":\""+(int)sensors.getTempCByIndex(0)+"\"}");
          }
          else
          {
            http.POST("{\"uid\":\""+String(objectTwo.UID_CHAR)+"\",\"sensorName\":\""+String(objectTwo.SENSOR_TYPE_CHAR)+"\",\"sensorLocation\":\""+String(objectTwo.SENSOR_NAME_CHAR)+"\",\"date\":\""+India.dateTime("d-m-Y")+"\",\"time\":\""+India.dateTime("H:i")+"\",\"value\":\""+analogRead(A0)+"\"}");
            Serial.println(analogRead(A0));
          }
          //http.end();
          count = 0;
        }

      delay(1000);
      count++;
    }
  }  
}