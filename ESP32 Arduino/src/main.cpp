#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

DynamicJsonDocument doc(1024); // khai báo bộ nhớ động cho json

// WiFi
// const char* ssid = "luctien";
// const char* password = "th28012001";

const char* ssid = "Fairy's Galaxy A22 5G";
const char* password = "rgps2001";

WiFiClient espClient;

// MQTT
PubSubClient client(espClient);
// const char* mqtt_server = "192.168.0.102";
const char* mqtt_server = "192.168.123.183";
const char* mqtt_user = "";
const char* mqtt_password = "";


char count_string = 0;
char array_receive1[50], count_string1 = 0;
unsigned int Count_time = 0, Split_count = 0;
char temp_char, array_receive[30], *temp[30];


void Send_ControlRelay(unsigned char Relay1_Stt, unsigned char Relay2_Stt); // khai báo hàm gửi dữ liệu điều khiển relay
void callback(char *topic, byte *payload, unsigned int length); // khai báo hàm callback

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial2.begin(9600);

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  while(!client.connected()){
    Serial.println("Connecting to MQTT...");
    if(client.connect("ESP32Client", mqtt_user, mqtt_password)){
      Serial.println("connected");
    }
    else{
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  client.subscribe("esp32/control");
}

void callback(char *topic, byte *payload, unsigned int length){

  for (int i = 0; i < length; i++)  
  {
    char temp_char1 =  (char)payload[i]; // doi tu char sang string
    array_receive1[count_string1++] = temp_char1; // them vao chuoi
  }

  if(array_receive1[1] == '0' && array_receive1[3] == '0') 
  {
    Serial.println("0 0");
    Send_ControlRelay(0,0);
  }
  else  if(array_receive1[1] == '1' && array_receive1[3] == '0')
  {
    Serial.println("1 0");
    Send_ControlRelay(1,0);
  }
  else  if(array_receive1[1] == '0' && array_receive1[3] == '1')
  {
    Serial.println("0 1");
    Send_ControlRelay(0,1);
  }
  else  if(array_receive1[1] == '1' && array_receive1[3] == '1')
  {
    Serial.println("1 1");
    Send_ControlRelay(1,1);
  }
  memset(array_receive1, 0, sizeof(array_receive1));
  count_string1 = 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial2.available()){
    char temp_char = Serial2.read();
    // Serial.print(temp_char);
    if(temp_char == '!'){ // kiểm tra kí tự kết thúc chuỗi
      char Buffer[1000];  // khai báo bộ nhớ đệm
      unsigned int lenBuffer; // khai báo độ dài chuỗi json
      String esp32_data = ""; // khai báo chuỗi json
      Split_count = 0;
      count_string = 0;
      temp[0] = strtok(array_receive, " "); // split string
      while(temp[Split_count] != NULL){ 
        Split_count++;
        temp[Split_count] = strtok(NULL, " "); // split string
      }

      esp32_data = "{\"Temp1\":\""+String(temp[0])+"\"," "\"Humi1\":\""+String(temp[1])+"\"," "\"Temp2\":\""+String(temp[2])+"\"," "\"Humi2\":\""+String(temp[3])+"\"," "\"Soil1\":\""+String(temp[4])+"\"," "\"Soil2\":\""+String(temp[5])+"\"}"; // format json
      lenBuffer = esp32_data.length()+1; // độ dài chuỗi json
      esp32_data.toCharArray(Buffer, lenBuffer); // chuyển chuỗi json sang mảng
      Serial.println(esp32_data);

      client.connect("ESP32Client"); // connect to MQTT
      client.publish("esp32/sensor", Buffer); // publish data to MQTT
      memset(array_receive, 0, sizeof(array_receive));// xóa bộ nhớ đệm
    }
    else{
      array_receive[count_string++] = temp_char; // them vao chuoi
    }
    // Serial.println(array_receive);
  }
  client.loop();
}

void Send_ControlRelay(unsigned char Relay1_Stt, unsigned char Relay2_Stt)
{
  char Array_Send[10]; // khai báo mảng gửi dữ liệu điều khiển relay
  sprintf(Array_Send, "%d %d!", Relay1_Stt, Relay2_Stt); // định dạng chuỗi gửi dữ liệu điều khiển relay
  Serial.println(Array_Send); // in ra mảng gửi dữ liệu điều khiển relay
  Serial2.write(Array_Send); // gửi dữ liệu điều khiển relay
}