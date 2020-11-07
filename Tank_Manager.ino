
#include <stdlib.h>
#include <stdio.h>
#include <bits/stdc++.h> 
#include <WiFi.h>          
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>         
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include "index.h"


//Library for Sensor and RTC
#include "DHT.h"
#include <OneWire.h>
#include <DS18B20.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "RTClib.h"
//Library for SD card
#include <ESPmDNS.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"


#include<EEPROM.h>
#define LED 2        

#define SD_CS 2
//DHT Sensor
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN,DHTTYPE);

//DS18B20 Sensor temperature
#define ONE_WIRE_BUS 5
#define TEMPERATURE_PRECISION 12
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tankThermometer, outsideThermometer;

//SSID and Password of ESP
const char* ssid = "Tank_Mannager";
const char* password = "12345678";
 
WebServer server(80); //Server on port 80
WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;


//Biến thể hiện trạng thái của các cảm biến
String DeviceState;
String Sensor1State, Sensor2State;
String SDCardState;
String MQTTState= "Disconnected";
File testfile;
String filename = "/test.csv";

//Biến về các thông số của MQTT
String  MQTT,Port, Username, Password, Topic;
String WiFi_name, WiFiPassword, IP_address, gateway, subnet;
//Biến thể hiện chuỗi dữ liệu sẽ được lưu vào thẻ nhớ.
String data_Storage;


//Ghi dữ liệu vào thẻ nhớ.
void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        SDCardState="SD Card Unavailable";
        return;
    }
    if(file.println(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
        SDCardState="SD Card Unavailable";
    }
    file.close();
}
//-------------------------------------------------------------------/
//Hàm được thực hiện khi người dùng nhấn vào nút dowload file
void File_Download(){
  Serial.println(server.arg("file_name"));
  File download = SD.open("/"+filename);
    if (download) {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename="+filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    }
    else server.send(200,"text/html", "File does not exist");
    
    }
  

//------------------------DS18B20 function-------------------------------------------/
// Hàm dùng để in ra địa chỉ của cảm biến nhiệt độ DS18B20
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


// Hàm thực hiện in ra thông tin độ phân giải của cảm viến
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}

// Hàm trả về giá trị nhiệt độ của cảm biến
float getData(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return 0xFF;
  }
  return tempC;
}

//-------------------------------------------------------------------/
//Hàm để trả về tràn giao diện khi người dùng truy cập vào webserver.
void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}

float humidity, temperature, tank_temp, room_temp;
String timeStamp;
RTC_DATA_ATTR int ID = 0;
DateTime datetime;
RTC_DS3231 RTC;

//-------------------------------------------------------------------/
//Hàm thực hiện đọc giá trị Real time clock
void ReadRTC(){
  datetime =  RTC.now();
    timeStamp = String(datetime.year())+"/"+String(datetime.month())+"/"+String(datetime.day())+" "+String(datetime.hour())+":"+String(datetime.minute())+":"+String(datetime.second());
}


//-------------------------------------------------------------------/
//Hàm thực hiện nhiệm vụ ghi dữ liệu vào thẻ nhớ
void logSDCard(){
    data_Storage=timeStamp+","+String(tank_temp)+","+String(room_temp)+","+String(temperature)+","+String(humidity);

    Serial.print("Message append: ");
    Serial.println(data_Storage);
    appendFile(SD,"/test.csv", data_Storage.c_str());
  
  
}

//-------------------------------------------------------------------/
//Hàm để tao đổi với webserver gửi thông tin về nhiệt độ, độ ẩm lên web
void handleADC() {
  
  Wire.begin();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  String data = "{\"Tank_temp\":\""+String(tank_temp)+"\", \"Room_temp\":\""+ String(room_temp) +"\", \"Outdoor_temp\":\""+ String(temperature) +"\", \"Outdoor_humi\":\""+ String(humidity) +"\"}";
  Serial.println(data);
  int n= data.length();
  char datapublish[n+1];
  strcpy(datapublish, data.c_str());

  n = Topic.length();
  char _Topic[n+1];
  strcpy(_Topic, Topic.c_str());
  if(MQTTState == "Connected"){
    client.publish(_Topic,datapublish);
    }
  digitalWrite(LED,!digitalRead(LED));
  server.send(200, "text/plane", data); 
  ReadRTC();
  logSDCard();
  tank_temp =0;
  room_temp =0;
}

//-------------------------------------------------------------------/
//Thực hiện việc kết nối MQTT broker
bool connectMQTT(char *MQTT_Broker, char *username, char *password, char *topic, uint16_t Port) {
  client.disconnect();
  bool a;
  // Chờ tới khi kết nối
  client.setServer(MQTT_Broker,Port);
  while(!client.connected()){
    Serial.print("Attempting MQTT connection...");
    if(client.connect("ESP32", username, password)){
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish(topic, "Hello world. I'm connected with the broker");
      a=1;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      a=0;
    }
      
    }
  return a = 1?:true|false;
}


//-------------------------------------------------------------------/
//Hàm sẽ được thực hiện khi người dùng Submit các thông số của MQTT broker
void handleMQTT(){
  client.disconnect();
  MQTTState = "Disconnected";
  int n ;
  MQTT = server.arg("mqtt_broker");
  Port = server.arg("port");
  Username = server.arg("username");
  Password = server.arg("password");
  Topic = server.arg("topic"); 


  //Lưu dữ liệu về cấu hình IP vào EEPROM từ ô nhớ 0 đến ô nhớ 30
    if(MQTT.length()>0 && Port.length()>0 &&  Username.length()>0 && Password.length()>0 && Topic.length()>0){
      Serial.println("Clearing eeprom");
      for (int i = 45; i < 180; ++i) {
        EEPROM.write(i, NULL);
      }
      Serial.println("Writing MQTT:");
      for (int i = 0; i < MQTT.length(); ++i)
      {
        EEPROM.write(45+i, MQTT[i]);
        Serial.print("Wrote: ");
        Serial.println(MQTT[i]);
      }
      
      Serial.println("Writing port:");
      for (int i = 0; i < Port.length(); ++i)
      {
        EEPROM.write(100 + i, Port[i]);
        Serial.print("Wrote: ");
        Serial.println(Port[i]);
      }
  
      Serial.println("Writing username:");
      for (int i = 0; i < Username.length(); ++i)
      {
        EEPROM.write(105 + i, Username[i]);
        Serial.print("Wrote: ");
        Serial.println(Username[i]);
      }

      Serial.println("Writing password:");
      for (int i = 0; i < Password.length(); ++i)
      {
        EEPROM.write(130 + i, Password[i]);
        Serial.print("Wrote: ");
        Serial.println(Password[i]);
      }

      Serial.println("Writing topic:");
      for (int i = 0; i < Topic.length(); ++i)
      {
        EEPROM.write(150 + i, Topic[i]);
        Serial.print("Wrote: ");
        Serial.println(Topic[i]);
      }
      
      EEPROM.commit();
      
      }

  
    n = MQTT.length();
    char _MQTT[n+1];
    strcpy(_MQTT, MQTT.c_str());
    
    n = Port.length();
    char _Port[n+1];
    strcpy(_Port, Port.c_str());
    int Port1 = atoi(_Port);
    
    n = Password.length();
    char _Password[n+1];
    strcpy(_Password, Password.c_str());
    
    n = Username.length();
    char _Username[n+1];
    strcpy(_Username, Username.c_str());
    
    n = Topic.length();
    char _Topic[n+1];
    strcpy(_Topic, Topic.c_str());
     
    if(connectMQTT(_MQTT,_Username,_Password,_Topic,Port1))
    MQTTState = "Connected";
    else MQTTState = "Disconnected";
  
  Serial.print("MQTT: ");
  Serial.println(MQTT);
  Serial.print("Port: ");
  Serial.println(Port);
  Serial.print("Username: ");
  Serial.println(_Username);
  Serial.print("Password: ");
  Serial.println(Password);
  Serial.print("Topic: ");
  Serial.println(Topic);
  String s ;
  s= "{\"MQTTState\":\""+MQTTState+"\"}";
  
  server.send(200, "text/plane",s = MQTTState); //Send web page
  }

  //-------------------------------------------------------------------/
  
  //Hàm sẽ được thực hiện khi người dùng Submit các thông số của WiFi
  void handleWiFi(){   
      int n;
      WiFi_name = server.arg("wifi");
      WiFiPassword = server.arg("password");
      IP_address = server.arg("ip");
      gateway = server.arg("gateway");
      subnet = server.arg("subnet");
      Serial.print("WiFi: ");
      Serial.println(WiFi_name);
      Serial.print("Password: ");
      Serial.println(WiFiPassword);
      Serial.print("IP Address: ");
      Serial.println(IP_address);
      Serial.print("Gateway: ");
      Serial.println(gateway);
      Serial.print("Subnet: ");
      Serial.println(subnet);
      
      //Lưu dữ liệu về cấu hình IP vào EEPROM từ ô nhớ 0 đến ô nhớ 30
      if(IP_address.length()>0 && gateway.length()>0 &&  subnet.length()>0){
        Serial.println("clearing eeprom");
        for (int i = 0; i < 45; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println("Writing IP:");
        for (int i = 0; i < IP_address.length(); ++i)
        {
          EEPROM.write(i, IP_address[i]);
          Serial.print("Wrote: ");
          Serial.println(IP_address[i]);
        }
        
        Serial.println("Writing gateway:");
        for (int i = 0; i < gateway.length(); ++i)
        {
          EEPROM.write(15 + i, gateway[i]);
          Serial.print("Wrote: ");
          Serial.println(gateway[i]);
        }

        Serial.println("Writing subnet:");
        for (int i = 0; i < subnet.length(); ++i)
        {
          EEPROM.write(30 + i, subnet[i]);
          Serial.print("Wrote: ");
          Serial.println(subnet[i]);
        }
        EEPROM.commit();
        
        }
      
      
      n = WiFi_name.length();
      char _WiFi_name[n+1];
      strcpy(_WiFi_name, WiFi_name.c_str());
      
      n = WiFiPassword.length();
      char _WiFiPassword[n+1];
      strcpy(_WiFiPassword, WiFiPassword.c_str());

      n = IP_address.length();
      char _IP_address[n+1];
      strcpy(_IP_address, IP_address.c_str());

      
      
      n = gateway.length();
      char _gateway[n+1];
      strcpy(_gateway, gateway.c_str());
      
      n = subnet.length();
      char _subnet[n+1];
      strcpy(_subnet, subnet.c_str());

      
      char *IP= strtok(_IP_address,".");
      uint8_t _IP[4];
      int i=0;
      while(IP != NULL){
        sscanf(IP, "%d", &_IP[i]);
        IP = strtok(NULL, ".");
        i++;
        }
        
      char *subnetconfig= strtok(_subnet,".");
      uint8_t _subnetconfig[4];
      i=0;
      while(subnetconfig != NULL){
        sscanf(subnetconfig, "%d", &_subnetconfig[i]);
        subnetconfig = strtok(NULL, ".");
        i++;
        }

      char *gatewayconfig= strtok(_gateway,".");
      uint8_t _gatewayconfig[4];
      i=0;
      while(gatewayconfig != NULL){
        sscanf(gatewayconfig, "%d", &_gatewayconfig[i]);
        gatewayconfig = strtok(NULL, ".");
        i++;
        }

      if(WiFi_name!=""){
        wifiManager.erase();
        Serial.print("Connecting to ");
        Serial.println(WiFi_name);
        WiFi.begin(_WiFi_name, _WiFiPassword);
        while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        }
        Serial.println("");
      
        Serial.println("WiFi connected");        
        }

      Serial.println();
      IPAddress _localip = IPAddress(_IP[0], _IP[1], _IP[2], _IP[3]);
      IPAddress _localgateway = IPAddress(_gatewayconfig[0], _gatewayconfig[1], _gatewayconfig[2], _gatewayconfig[3]);

      IPAddress _localsubnet=IPAddress(_subnetconfig[0], _subnetconfig[1], _subnetconfig[2], _subnetconfig[3]);
      IPAddress primaryDNS(8, 8, 8, 8);   //optional
      IPAddress secondaryDNS(8, 8, 4, 4); //optional
      WiFi.config(_localip,_localgateway, _localsubnet, primaryDNS, secondaryDNS);
      
      //wifiManager.setSTAStaticIPConfig(_localip,_localgateway, _localsubnet);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  //-------------------------------------------------------------------/
  //Hàm sẽ được thực hiện khi người dùng muốn update trạng thái của các thiết bị đang hoạt động 
void handleDeviceState(){
  if((MQTT!=NULL)&(Port!=NULL)&(Username!=NULL)&(Password!=NULL)&(Topic!=NULL)){
    }
    else {
      MQTT = "NULL";
      Port = "NULL";
      Topic = "NULL";
    }

//    if((IP_address!=NULL)&(gateway!=NULL)&(subnet!=NULL)){
//      }
//      else {
//        IP_address="NULL";
//        gateway = "NULL";
//      }
  String DeviceState = 
  "{\"MQTTState\":\""+MQTTState+"\", \"Sensor1State\":\""+ Sensor1State +"\",\"Sensor2State\":\""+ Sensor2State +"\", \"SDCardState\":\""+ SDCardState +"\", \"MQTT_Broker\":\""+String(MQTT) +"\", \"Port\":\""+ String(Port) +"\", \"Topic\":\""+ String(Topic) +"\", \"IP_address\":\""+ IP_address +"\", \"gateway\":\""+ gateway +"\", \"wifi_name\":\""+ wifiManager.getWiFiSSID(true) +"\"}";                  
  server.send(200, "text/plane", DeviceState);
}

//Hàm được gọi khi người dùng bấm nút reser Setting.
void handleresetSetting(){
  server.send(200, "text/plane", "Reset device successfully");
  delay(50);
  wifiManager.resetSettings();
  Serial.println("Deleting EEPROM");
    // write a 0 to all 512 bytes of the EEPROM
    for (int i = 0; i < 180; i++) {
      EEPROM.write(i, NULL);
    }
    EEPROM.commit();
    Serial.println("DONE");
    Serial.println(); 
  }

  
void configModeCallback (WiFiManager *myWiFiManager) {
   Serial.println("Deleting EEPROM");
    // write a 0 to all 512 bytes of the EEPROM
    for (int i = 0; i < 180; i++) {
      EEPROM.write(i, NULL);
    }
    EEPROM.commit();
    Serial.println("DONE");
    Serial.println(); 
    
}


void setup()
{ 
  Serial.begin(9600);
  Serial.println();
  dht.begin();
  delay(10);
  Wire.begin();
  delay(10);
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
   
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect(ssid, password);
  wifiManager.setCleanConnect(true);
  Serial.println("connected...yeey :)");
  Serial.println("");
  //Đọc dữ liệu về IP, gateway, subnet trong EEPROM và cấu hình cho thiết bị

  //Reading EEPROM IP
  for (int i = 0; i < 15; ++i)
  {
    if (EEPROM.read(i)!=NULL)
      IP_address += char(EEPROM.read(i));
  }
  //Reading EEPROM gateway
  for (int i = 15; i < 30; ++i)
  {
    if (EEPROM.read(i)!=NULL)
      gateway += char(EEPROM.read(i));
  }
  //Reading EEPROM subnet
  for (int i = 30; i < 45; ++i)
  {
    if (EEPROM.read(i)!=NULL)
      subnet += char(EEPROM.read(i));
  }
  
  if((IP_address!=NULL)&(gateway!=NULL)&(subnet!=NULL)){
    Serial.print("IP: ");
    Serial.println(IP_address);
    Serial.print("Gateway: ");
    Serial.println(gateway);
    Serial.print("Subnet: ");
    Serial.println(subnet);
    int n;     
    n = IP_address.length();
    char _IP_address[n+1];
    strcpy(_IP_address, IP_address.c_str());
     
    n = gateway.length();
    char _gateway[n+1];
    strcpy(_gateway, gateway.c_str());
        
    n = subnet.length();
    char _subnet[n+1];
    strcpy(_subnet, subnet.c_str());
  
        
    char *IP= strtok(_IP_address,".");
    uint8_t _IP[4];
    int i=0;
    while(IP != NULL){
      sscanf(IP, "%d", &_IP[i]);
      IP = strtok(NULL, ".");
      i++;
     }
          
    char *subnetconfig= strtok(_subnet,".");
    uint8_t _subnetconfig[4];
    i=0;
    while(subnetconfig != NULL){
      sscanf(subnetconfig, "%d", &_subnetconfig[i]);
      subnetconfig = strtok(NULL, ".");
      i++;
      }
  
    char *gatewayconfig= strtok(_gateway,".");
    uint8_t _gatewayconfig[4];
    i=0;
    while(gatewayconfig != NULL){
      sscanf(gatewayconfig, "%d", &_gatewayconfig[i]);
      gatewayconfig = strtok(NULL, ".");
      i++;
     }
  
    IPAddress _localip = IPAddress(_IP[0], _IP[1], _IP[2], _IP[3]);
    IPAddress _localgateway = IPAddress(_gatewayconfig[0], _gatewayconfig[1], _gatewayconfig[2], _gatewayconfig[3]);
  
    IPAddress _localsubnet=IPAddress(_subnetconfig[0], _subnetconfig[1], _subnetconfig[2], _subnetconfig[3]);
    IPAddress primaryDNS(8, 8, 8, 8);   //optional
    IPAddress secondaryDNS(8, 8, 4, 4); //optional
    WiFi.config(_localip,_localgateway, _localsubnet,primaryDNS, secondaryDNS);
    } 
    else Serial.println("No IP config data found. Obtain IP automatically.");
      

//------------------------------------------------------------------------------------//

 //Đọc dữ liệu về MQTT trong EEPROM và cấu hình cho thiết bị

  //Reading EEPROM MQTT
  for (int i = 45; i < 100; ++i)
  {
    if (EEPROM.read(i)!=NULL)
      MQTT += char(EEPROM.read(i));
  }
  //Reading EEPROM Port
  for (int i = 100; i < 105; ++i)
  {
    if (EEPROM.read(i)!=NULL)
    Port += char(EEPROM.read(i));
  }
  //Reading EEPROM username
  for (int i = 105; i < 130; ++i)
  {
    if (EEPROM.read(i)!=NULL)
    Username += char(EEPROM.read(i));
  }
  //Reading EEPROM password
  for (int i = 130; i < 150; ++i)
  {
    if (EEPROM.read(i)!=NULL)
    Password += char(EEPROM.read(i));
  } 
  //Reading EEPROM topic
  for (int i = 150; i < 180; ++i)
  {
    if (EEPROM.read(i)!=NULL)
      Topic += char(EEPROM.read(i));
  }
  

  if((MQTT!=NULL)&(Port!=NULL)&(Username!=NULL)&(Password!=NULL)&(Topic!=NULL)){
    Serial.print("MQTT: ");
    Serial.println(MQTT);
    Serial.print("Port: ");
    Serial.println(Port);
    Serial.print("Username: ");
    Serial.println(Username);
    Serial.print("Password: ");
    Serial.println(Password);
    Serial.print("Topic: ");
    Serial.println(Topic);
    int n;
    n = MQTT.length();
    char _MQTT[n+1];
    strcpy(_MQTT, MQTT.c_str());
    
    n = Port.length();
    char _Port[n+1];
    strcpy(_Port, Port.c_str());
    int Port1 = atoi(_Port);
    
    n = Password.length();
    char _Password[n+1];
    strcpy(_Password, Password.c_str());
    
    n = Username.length();
    char _Username[n+1];
    strcpy(_Username, Username.c_str());
    
    n = Topic.length();
    char _Topic[n+1];
    strcpy(_Topic, Topic.c_str());
     
    if(connectMQTT(_MQTT,_Username,_Password,_Topic,Port1))
    MQTTState = "Connected";
    else MQTTState = "Disconnected";
   }
    else  Serial.println("No MQTT Config data found");
 //--------------------------------------------------------------------------------------//
  
  pinMode(LED,OUTPUT); 
  
  //Cấu hình với SD Card
  SD.begin(SD_CS);  
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    SDCardState="SD Card Unavailable";
    //return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    
    //return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
//    return; 
  }


  if(SD.exists(filename)){
    SDCardState="SD Card Available";
    Serial.println(filename+" exist, deleting and starting new");
    SD.remove(filename);
  }
  testfile = SD.open(filename,FILE_WRITE);
  if(testfile){
    testfile.println("Timestamp,Tank_temp, Room_temp, Outdoor_temp, Outdoor_humi");
    testfile.close();
  }
  else
  {
    Serial.println("Error open file");
  }
  
  delay(20);

  // Thực hiện cấu hình với các cảm biến DS18B20
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");



  // Search for devices on the bus and assign based on an index. Ideally,
  if (!sensors.getAddress(tankThermometer, 0))
  {Serial.println("Unable to find address for Device 0");
  
    Sensor1State ="Unavailable";
  } else Sensor1State="Available" ;
  if (!sensors.getAddress(outsideThermometer, 1)) {
    Serial.println("Unable to find address for Device 1");
    Sensor2State="Unavailable";
    }
    else Sensor2State="Available";


  // Hiển thị địa chỉ của các cảm biến có trong bus
  Serial.print("Device 0 Address: ");
  printAddress(tankThermometer);
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(outsideThermometer);
  Serial.println();

  // Set độ phan giải cho các cảm biến
  sensors.setResolution(tankThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(outsideThermometer, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(tankThermometer), DEC);
  Serial.println();

  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(outsideThermometer), DEC);
  Serial.println();

  
  //Đợi cho đến khi thiết bị kết nối wifi thành công và bắt đầu chạy webserver
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 
  Serial.println(WiFi.gatewayIP());
  IP_address = WiFi.localIP().toString();
  gateway = WiFi.gatewayIP().toString();
  
  server.on("/", handleRoot); 
  server.on("/readADC", handleADC);
  server.on("/getMQTT", handleMQTT);
  server.on("/getWiFi", handleWiFi);
  server.on("/download", File_Download);
  server.on("/deviceState", handleDeviceState);
  server.on("/resetSetting", handleresetSetting);
  
  server.begin();                  
  Serial.println("HTTP server started");
}


void loop()
{
  server.handleClient();
  if(WiFi.status()!=WL_CONNECTED){
    ESP.restart();
    }
    sensors.requestTemperatures();
 
  tank_temp=getData(tankThermometer);
  if (tank_temp == 0xFF){
    Sensor1State = "Unavailable";
    tank_temp= NULL;
    }
  else Sensor1State = "Available";
    
  room_temp=getData(outsideThermometer);
  if (room_temp == 0xFF){
    Sensor2State = "Unavailable";
    room_temp= NULL;
    }
  else Sensor2State = "Available";
}
