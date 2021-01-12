#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Adafruit_SSD1306.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino.h>

//AC (for generall Configuration SSID/WiFI Password/ Server IP/ Sensor Nummer)
AsyncWebServer acserver(80);
String ssid;
String input_ssid;
String passwort;
String input_passwort;
String server_ip;
String input_server_ip;
String sensor_nr;
String input_sensor_nr;
const char* ac_ssid = "CO2-Sensor";
const char* ac_passwort = "1234567890";
const char* PARAM_INPUT_1 = "SSID";
const char* PARAM_INPUT_2 = "Passwort";
const char* PARAM_INPUT_3 = "ServerIP";
const char* PARAM_INPUT_4 = "SensorNummer";
int time_for_config = 1;
bool WiFi_Mode = true;

//IP
uint8_t ip_1;
uint8_t ip_2;
uint8_t ip_3;
uint8_t ip_4;
String ip_array[4];
IPAddress DBserver(0,0,0,0);


//Display-Configuration
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 32 
#define OLED_RESET    D0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiClient client;
SoftwareSerial co2Serial(0, 2); // RX|TX D3/D4

//Server
String urlString;
String concentrationToDB;
String tabel;

//Pins
int grueneLED= D6; 
int gelbeLED = D7; 
int roteLED  = D8; 
int speaker  = D5; 

//Piezo
int anz_alarm = 0;
int max_anz_alarm = 2;

//Preheat
int vorheiz_zeit = 20; //IdR. 3min/180s, zu Testwecken 20
int anz_balken = 0;

//Sensor auslesen
int concentration, temperature = 0;

//Configuration HTML Page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>CO2-Sensor Configuration</title>
<style>
table {
  font-family: arial, sans-serif;
  border-collapse: collapse;
}
td, th {
  text-align: left;
  padding: 8px;
}
</style>
</head>
<body>
<h2><u>CO2-Sensor Configuration</u></h2>

<meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
  
<table>
  <tr>
    <td>SSID: </td>
    <td><input type="text" name="SSID"></td>
  </tr>
  <tr>
    <td>Passwort: </td>
    <td><input type="text" name="Passwort"></td>
  </tr>
  <tr>
    <td>Server-IP: </td>
    <td><input type="text" name="ServerIP"><br></td>
  </tr>
  <tr>
    <td>Sensor Nummer: </td>
    <td><input type="text" name="SensorNummer"></td>
  </tr>
  <tr>
    <td></td>
    <td><input type="submit" value="Submit"></td>
  </tr>
</table>
</form> 
</body>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}


void setup() {

  //Start Serial Connection
  Serial.begin(115200); 

  //start OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  
      Serial.println(F("SSD1306 allocation failed"));
      for(;;); 
  }
  
  //Pin Definitions
  pinMode(grueneLED, OUTPUT);
  pinMode(gelbeLED, OUTPUT);
  pinMode(roteLED, OUTPUT);
  pinMode(speaker, OUTPUT);

  co2Serial.begin(9600); //start Serial Connection to the Sensor
  
  //--Configuration via AC--
  
  //Start AC
  Serial.print("Setting Access Point:");
  Serial.println(ac_ssid);
  WiFi.softAP(ac_ssid, ac_passwort);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
 
  Serial.println(WiFi.localIP());
 
  // Send web page for config to client
  acserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request 
  acserver.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) 
  { 
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2) & request->hasParam(PARAM_INPUT_3) & request->hasParam(PARAM_INPUT_4)) {
      ssid = request->getParam(PARAM_INPUT_1)->value();
      input_ssid = PARAM_INPUT_1;

      passwort = request->getParam(PARAM_INPUT_2)->value();
      input_passwort = PARAM_INPUT_2;

      server_ip = request->getParam(PARAM_INPUT_3)->value();
      input_server_ip = PARAM_INPUT_3;
      
      sensor_nr = request->getParam(PARAM_INPUT_4)->value();
      input_sensor_nr = PARAM_INPUT_4;

      if ((ssid == "") or  (passwort == "") or (server_ip == "") or (sensor_nr == "")){
        request->send(200, "text/html", "<br><h1><center>ERROR<br><a href=\"/\">Try again</a></center></h1>");
      }
      else {
        request->send(200, "text/html", "<br><h1><center>Configuration Complete!</center></h1>");
      }
    }
    else {
      Serial.println("No message sent");
      request->send(200, "text/html", "<br><h1><center>ERROR<br><a href=\"/\">Try again</a></center></h1>");
    }
    
    Serial.println(ssid);
    Serial.println(passwort);
    Serial.println(server_ip);
    Serial.println(sensor_nr);
    
  });

  acserver.onNotFound(notFound);
  acserver.begin(); //start Server for congig
  
  while((ssid == "") and (passwort == "") and (server_ip == "") and (sensor_nr == "") and (WiFi_Mode == true)){  //wait until config is complete
    display.clearDisplay();  
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(14, 8);
    display.print("Connect to WiFi:");
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(28, 20);
    display.print("CO2-Sensor");
    display.display();

    delay(2500);  //REPLAC WITH MILIS

    display.clearDisplay();  
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(14, 8);
    display.print("Open in Browser:");
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(26, 20);
    display.print("192.168.4.1");
    display.display();

    delay(2500);
   
    time_for_config++;
    
    //after 5 minutes the sensor starts without WiFI connection
    if (time_for_config == 60){  //60*5s = 5min
      acserver.end();
      WiFi.softAPdisconnect();
      WiFi_Mode = false;
    }   
  }

  if (WiFi_Mode == true) {
  
    // show Configuration completet on OLED
    display.clearDisplay();  
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(16, 8);
    display.print("Configuration");
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(28, 20);
    display.print("complete.");
    display.display();

    delay(2000);

    // end Server/ cloese Ac
    acserver.end();
    WiFi.softAPdisconnect();
  
  
    // Set IP for DBServer 
    server_ip = server_ip + ".";
  
    int r=0,t=0;
    for(int i=0;i<server_ip.length();i++)
    {
      if(server_ip[i] == '.')
      {
        if (i-r > 1)
        {
          ip_array[t] = server_ip.substring(r,i);
          t++;
        }
        r = (i+1);
      }
    }
    
    ip_1=atoi(ip_array[0].c_str());
    ip_2=atoi(ip_array[1].c_str());
    ip_3=atoi(ip_array[2].c_str());
    ip_4=atoi(ip_array[3].c_str());

    //IPAddress DBserver(ip_1,ip_2,ip_3,ip_4);
    //Serial.println(DBserver);

    //database setup
    tabel = "sensor"+sensor_nr;
   
    WiFi.begin(ssid, passwort); //connect to local WiFi  
    Serial.println("Connecting");

    //show Connecting to WiFi... on OLED
    display.clearDisplay();  
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 14);
    display.print("Connecting to WiFi...");
    display.display();
  
    for(int i = 0; i < 120;i++){
      delay(500);
      Serial.print(".");

      if(WiFi.status() == WL_CONNECTED) {
      i=120;
      }
    }

    //show ERROR Message when connection failed
    while(WiFi.status() != WL_CONNECTED) {
      display.clearDisplay();  
      display.setTextSize(2); 
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(28, 0);
      display.print("ERROR");
      display.setTextSize(1); 
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(10, 16);
      display.print("Can't reach WiFi");
      display.setTextSize(1); 
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(16, 25);
      display.print("Restart Sensor");
      display.display();
     }
   
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    //Show successfully connected on OLED
    display.clearDisplay();  
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(25, 8);
    display.print("Succesfully");
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(28, 20);
    display.print("connected.");
    display.display();
  
    delay(2000);
  }
  
   
  //Preaheat    
  display.clearDisplay();
  display.setTextSize(2); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 8);
  display.print("Preheating");
  display.setCursor(0, 22);
  display.setTextSize(2); 
  display.print("");
  display.display();

  for (int i = 0; i < vorheiz_zeit; i++)
  { 
    if (anz_balken < 10)
      {
        display.print("-");
        display.display();
        anz_balken++;
      }
      else
      {
        display.clearDisplay();
        display.setTextSize(2); 
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 8);
        display.print("Preheating");
        display.setCursor(0, 22);
        display.setTextSize(2); 
        display.print("");
        display.display();
          
        anz_balken = 0;
      }  
        
      readSensor(&concentration, &temperature);
      Serial.println("Preheating...");
    }
   
    display.clearDisplay();
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(1, 0);
    display.print("CO2-Concentration:");
    display.setTextSize(3); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 12);
    display.print(concentration);
    display.setTextSize(2); 
    display.println(" ppm");
    display.display(); 

    if (concentration >=  2000)
    {
      digitalWrite(grueneLED, LOW);
      digitalWrite(gelbeLED, LOW);
      digitalWrite(roteLED, HIGH);

      if (anz_alarm < max_anz_alarm) 
        {
        digitalWrite(speaker, HIGH);
        delay(1000);
        digitalWrite(speaker, LOW);
        anz_alarm++;
        }
    }  
    else if (concentration >= 1500)
    { 
      digitalWrite(grueneLED, LOW);
      digitalWrite(gelbeLED, HIGH);
      digitalWrite(roteLED, LOW);
      anz_alarm =0;
    }
    else
    {
      digitalWrite(grueneLED, HIGH);
      digitalWrite(gelbeLED, LOW);
      digitalWrite(roteLED, LOW);
      anz_alarm =0;
    }
}

void loop() {

  int concentrationToDB_gesamt = 0;

  //read Sensor for data to DBserver (60*10 times)-> 10min interval (for Testing 12*10 -> 2min interval)
  for (int i = 0; i < 60; i++)
  {
    int co2_gesamt = 0;

    //read Sensor for data on Display (10times, ~once evvery second)
    for (int i = 0; i < 10; i++) // 
    {
      readSensor(&concentration, &temperature);
      co2_gesamt = co2_gesamt + concentration;
    }

    concentration = co2_gesamt/10;
    concentrationToDB_gesamt = concentrationToDB_gesamt + concentration;

    Serial.print("PPM: ");
    Serial.println(concentration);
 
    display.clearDisplay();
    display.setTextSize(1); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(1, 0);
    display.print("CO2-Concentration:");
    display.setTextSize(3); 
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 12);
    display.print(concentration);
    display.setTextSize(2); 
    display.println(" ppm");
    display.display(); 

    //Ampel
    if (concentration >=  2000)
    {
      digitalWrite(grueneLED, LOW);
      digitalWrite(gelbeLED, LOW);
      digitalWrite(roteLED, HIGH);

      if (anz_alarm < max_anz_alarm) 
      {
        digitalWrite(speaker, HIGH);
        delay(1000);
        digitalWrite(speaker, LOW);
        anz_alarm++;
        }
    }  
    else if (concentration >= 1500)
    { 
      digitalWrite(grueneLED, LOW);
      digitalWrite(gelbeLED, HIGH);
      digitalWrite(roteLED, LOW);
      anz_alarm =0;
    }
    else
    {
      digitalWrite(grueneLED, HIGH);
      digitalWrite(gelbeLED, LOW);
      digitalWrite(roteLED, LOW);
      anz_alarm =0;
    }
  }

  //only send Data if WiFi Mode is enabled
  if (WiFi_Mode == true){
  
    //show ERROR Message when connection failed
    if (WiFi.status() != WL_CONNECTED and WiFi_Mode == true){
      display.clearDisplay();  
      display.setTextSize(2); 
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(28, 0);
      display.print("ERROR");
      display.setTextSize(1); 
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(10, 16);
      display.print("Can't reach WiFi");
      display.setTextSize(1); 
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(16, 25);
      display.print("Restart Sensor");
      display.display();
      delay(2000);
    }
  
    concentrationToDB = concentrationToDB_gesamt/12;
    urlString = "GET /processincomingco2.php?c=" + concentrationToDB + "&t=" + tabel + " HTTP/1.1";

    const char* host = "192.168.178.78";
    
    Serial.println(urlString);
    IPAddress DBserver(ip_1,ip_2,ip_3,ip_4);
      
    Serial.println(DBserver);
    
    if (client.connect(host, 80)) 
    {
      client.println(urlString);
      client.println("Host: "+ip_array[0]+"."+ip_array[1]+"."+ip_array[2]+"."+ip_array[3]);
      client.println("Connection: close");
      client.println();
    }
    else
    {
      Serial.println("connection failed");
    }
  }
}
