#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <STM32FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <HardwareSerial.h>

#define DHT_1_PIN PA5
#define DHT_2_PIN PA6
#define DHTTYPE DHT11

#define SOIL_MOISTURE_1_PIN PA0
#define SOIL_MOISTURE_2_PIN PA1

#define CONTROL_RELAY_1_PIN PA4
#define CONTROL_RELAY_2_PIN PA15

xQueueHandle xQueue; // khai báo queue


DHT dht_1(DHT_1_PIN, DHTTYPE);
DHT dht_2(DHT_2_PIN, DHTTYPE);

HardwareSerial Serial_1(PA10, PA9); // cài đặt UART1
HardwareSerial Serial_2(PA3, PA2); // cài đặt UART2

float h_1, t_1, h_2, t_2;

float soilMoisture_1, soilMoisture_2;
float soilMoisturePercent_1, soilMoisturePercent_2;

typedef struct
{
	uint8_t Sensor_1_Temperature;
  uint8_t Sensor_1_Humidity;
  uint8_t Sensor_2_Temperature;
  uint8_t Sensor_2_Humidity;
  u_int8_t Sensor_1_SoilMoisture;
  u_int8_t Sensor_2_SoilMoisture;
}sensor_DataStruct_t; // khai báo struct

static void vTaskReadSensor(void *p); // khai báo task  đọc sensor
static void vTaskSendSensorData(void *p); // khai báo task gửi dữ liệu sensor
static void vTaskControlRelay(void *p); // khai báo task điều khiển relay


void setup() {
  // put your setup code here, to run once:
  Serial_1.begin(9600); // khởi tạo baudrate cho UART1
  Serial_2.begin(9600); // khởi tạo baudrate cho UART2
  pinMode(DHT_1_PIN, INPUT_PULLUP); // khai báo chân DHT11_1
  dht_1.begin(); // khởi tạo DHT11_1  
  pinMode(DHT_2_PIN, INPUT_PULLUP); // khai báo chân DHT11_2
  dht_2.begin(); // khởi tạo DHT11_2
  pinMode(SOIL_MOISTURE_1_PIN, INPUT); // khai báo chân đọc độ ẩm đất 1
  pinMode(SOIL_MOISTURE_2_PIN, INPUT); // khai báo chân đọc độ ẩm đất 2
  pinMode(CONTROL_RELAY_1_PIN, OUTPUT); // khai báo chân điều khiển relay 1
  pinMode(CONTROL_RELAY_2_PIN, OUTPUT); // khai báo chân điều khiển relay 2
  digitalWrite(CONTROL_RELAY_1_PIN, LOW); // tắt relay 1
  digitalWrite(CONTROL_RELAY_2_PIN, LOW); // tắt relay 2

  Serial_1.println("Show data");
  Serial_2.println("uart to esp32");
}

void loop() {
  // put your main code here, to run repeatedly:
  xQueue = xQueueCreate(10, sizeof(sensor_DataStruct_t)); // tạo queue 
  xTaskCreate(vTaskReadSensor, "Read Sensor", 128, NULL, 1, NULL); // tạo task đọc sensor 
  xTaskCreate(vTaskSendSensorData, "Send Sensor Data", 128, NULL, 1, NULL); // tạo task gửi dữ liệu sensor
  xTaskCreate(vTaskControlRelay, "Control Relay", 128, NULL, 1, NULL);  // tạo task điều khiển relay
  vTaskStartScheduler();
}

static void vTaskReadSensor(void *p)
{
  sensor_DataStruct_t getSensorData = {0, 0, 0, 0, 0, 0}; // khai báo struct
  while (1)
  {
    getSensorData.Sensor_1_Temperature = dht_1.readTemperature(); // đọc nhiệt độ sensor 1
    getSensorData.Sensor_1_Humidity = dht_1.readHumidity(); // đọc độ ẩm sensor 1
    getSensorData.Sensor_2_Temperature = dht_2.readTemperature(); // đọc nhiệt độ sensor 2
    getSensorData.Sensor_2_Humidity = dht_2.readHumidity(); // đọc độ ẩm sensor 2
    
    soilMoisture_1 = 0; // khởi tạo biến độ ẩm đất sensor 1
    soilMoisture_2 = 0; // khởi tạo biến độ ẩm đất sensor 2

    for( int i = 0; i < 50; i++) // đọc độ ẩm đất 50 lần
    {
      soilMoisture_1 += analogRead(SOIL_MOISTURE_1_PIN); // đọc độ ẩm đất sensor 1
      soilMoisture_2 += analogRead(SOIL_MOISTURE_2_PIN); // đọc độ ẩm đất sensor 2
      delay(1);
    }

    soilMoisture_1 /= 50; // lấy trung bình độ ẩm đất sensor 1
    soilMoisture_2 /= 50; // lấy trung bình độ ẩm đất sensor 2

    // getSensorData.Sensor_1_SoilMoisture = map(soilMoisture_1, 410, 110, 0, 100); // chuyển độ ẩm đất sensor 1 sang %
    // getSensorData.Sensor_2_SoilMoisture = map(soilMoisture_2, 410, 110, 0, 100); // chuyển độ ẩm đất sensor 2 sang %
    // getSensorData.Sensor_1_SoilMoisture = soilMoisture_1; // chuyển độ ẩm đất sensor 1 sang %
    // getSensorData.Sensor_2_SoilMoisture = soilMoisture_2; // chuyển độ ẩm đất sensor 2 sang %

    if(soilMoisture_1 > 410) // nếu độ ẩm đất sensor 1 < 410
    {
      getSensorData.Sensor_1_SoilMoisture = 0; // độ ẩm đất sensor 1 = 0
    }
    else if(soilMoisture_1 < 110) // nếu độ ẩm đất sensor 1 > 110
    {
      getSensorData.Sensor_1_SoilMoisture = 100; // độ ẩm đất sensor 1 = 100
    }
    else // nếu độ ẩm đất sensor 1 nằm trong khoảng 110 < x < 410
    {
      getSensorData.Sensor_1_SoilMoisture = map(soilMoisture_1, 410, 110, 0, 100); // chuyển độ ẩm đất sensor 1 sang %
    }

    if(soilMoisture_2 > 410) // nếu độ ẩm đất sensor 2 < 410
    {
      getSensorData.Sensor_2_SoilMoisture = 0; // độ ẩm đất sensor 2 = 0
    }
    else if(soilMoisture_2 < 110) // nếu độ ẩm đất sensor 2 > 110
    {
      getSensorData.Sensor_2_SoilMoisture = 100; // độ ẩm đất sensor 2 = 100
    }
    else // nếu độ ẩm đất sensor 2 nằm trong khoảng 110 < x < 410
    {
      getSensorData.Sensor_2_SoilMoisture = map(soilMoisture_2, 410, 110, 0, 100); // chuyển độ ẩm đất sensor 2 sang %
    }

    if (xQueue != 0){ // kiểm tra queue có tồn tại hay không 
      xQueueSend(xQueue, (void*) &getSensorData, (portTickType)0); // gửi dữ liệu sensor vào queue
    }
    vTaskDelay(1000); // delay 1s
  }
}

static void vTaskSendSensorData(void *p)
{
  char buffer[100]; // khai báo buffer để gửi dữ liệu
  while (1)
  {
    sensor_DataStruct_t getSensorData = {0, 0, 0, 0, 0, 0}; // khai báo struct
    if(xQueueReceive(xQueue,(void*) &getSensorData, (portTickType)0)){ // kiểm tra queue có tồn tại hay không
      sprintf(buffer, "%d %d %d %d %d %d !", getSensorData.Sensor_1_Temperature, getSensorData.Sensor_1_Humidity, getSensorData.Sensor_2_Temperature, getSensorData.Sensor_2_Humidity, getSensorData.Sensor_1_SoilMoisture, getSensorData.Sensor_2_SoilMoisture); // gán dữ liệu vào buffer
      Serial_1.write(buffer); // gửi dữ liệu qua UART1
      // int lenBuffer = strlen(buffer); // lấy độ dài của buffer
      // Serial_2.println("Length of buffer");
      // Serial_2.println(lenBuffer);
      // Serial_2.println("Sensor Data");
      // Serial_2.println(buffer);
      delay(1000);
    }
  }
}

char array_receive[1000], *temp[100]; // khai báo mảng để nhận dữ liệu từ ESP32
int Split_count = 0, count_string = 0; // khai báo biến đếm


static void vTaskControlRelay(void *p)
{
  // flag receive via serial port from ESP8266
  while (1)
  {
    if (Serial_1.available()) // kiểm tra dữ liệu gửi từ ESP32
    {
      char tmp = Serial_1.read(); // đọc dữ liệu từ UART1
      if(tmp == '!') // kiểm tra ký tự kết thúc
      {
        char Buffer[1000];  // khai báo mảng để nhận dữ liệu từ ESP32
        Split_count = 0; // khởi tạo biến đếm
        count_string = 0; // khởi tạo biến đếm
        temp[0] = strtok(array_receive, " "); // split string
        while(temp[Split_count] != NULL){ // kiểm tra dữ liệu
          Split_count++; // tăng biến đếm
          temp[Split_count] = strtok(NULL, " "); // split string
        }
        
        Serial_2.println("Data from ESP8266"); 
        Serial_2.println(temp[0]);  
        Serial_2.println(temp[1]);

        if (String(temp[0]) == "1") // kiểm tra dữ liệu
        {
          digitalWrite(CONTROL_RELAY_1_PIN, HIGH); 
        }
        else
        {
          digitalWrite(CONTROL_RELAY_1_PIN, LOW); 
        }

        if (String (temp[1]) == "1")
        {
          digitalWrite(CONTROL_RELAY_2_PIN, HIGH);
        }
        else
        {
          digitalWrite(CONTROL_RELAY_2_PIN, LOW);
        }

        memset(array_receive, 0, sizeof(array_receive)); // xóa mảng

      }
      else{ 
        array_receive[count_string++] = tmp; // gán dữ liệu vào mảng
      }
    } 
    vTaskDelay(1000);
  }
}

// Path: src\main.cpp