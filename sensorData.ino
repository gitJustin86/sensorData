/*
  Developer: Justin N. Pilon
*/

// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

// Replace with your network credentials
const char* ssid     = "8255B4";
const char* password = "241542254";

// current temperature & humidity, updated in loop()
#define ONE_WIRE_BUS D5
float t = 0.0;

#define DHTPIN D6
float h = 0.0;

#define SensorPin A0
float m = 0.0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

//define jspn string for sensors
String jsonSensors ="";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings
const long interval = 1000;  

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
html {
  font-family: Arial;
  display: inline-block;
  margin: 0px auto;
  text-align: center;
}
   
h2 {
  font-size: 3.0rem;
}
   
p {
  font-size: 3.0rem;
}
   
.units {
  font-size: 1.2rem;
}
   
.dht-labels{
  font-size: 1.5rem;
  vertical-align:middle;
  padding-bottom: 15px;
}
   
td {
  padding: 10px;
}
   
table {
  font-family: arial, sans-serif;
  border-collapse: collapse;
  width: 100%;
}

#masterTable th {
  padding-top: 12px;
  padding-bottom: 12px;
  text-align: left;
  background-color: #000000;
  color: white;
  }

td, th {
  border: 1px solid #dddddd;
  text-align: left;
  padding: 8px;
}

tr:nth-child(even) {
  background-color: #dddddd;
}
</style>
  
</head>
<body>

    <table id="masterTable">
      <tr>
        <th>Sensor type</th>
        <th>Data</th>
      </tr>
      <tr>
        <td>Temperature</td>
        <td><span id="temperature"></span>&#x2103;</td>
      </tr>
      <tr>
        <td>Moisture</td>
        <td><span id="moisture"></span>&#37;</td>
      </tr>
      <tr>
        <td>Humidity</td>
        <td><span id="humidity"></span>&#37;</td>
      </tr>
    </table>

</body>
<script>

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 500 );

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("moisture").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/moisture", true);
  xhttp.send();
}, 500 );

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 500 );

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("jsonObject").innerHTML = JSON.parse(this.responseText).moisture;
    }
  };
  xhttp.open("GET", "/sensors", true);
  xhttp.send();
}, 500 );

</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "MOISTURE"){
    return String(m);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });

  server.on("/moisture", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(m).c_str());
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });
  
  server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", jsonSensors.c_str());
  });

  // under soil temp
  sensors.begin();

  // Start server
  server.begin();
}
 
void loop(){
  sensors.requestTemperatures();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    
    // Read temperature
  t = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if (t != DEVICE_DISCONNECTED_C)
  {
    Serial.print("Temperature: ");
    Serial.println(t);
  }
  else
  {
    Serial.println("Error to reading data");
  }
    
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.print("Humidity: ");
      Serial.println(h);
    }

    // Read Moisture
    m = analogRead(SensorPin);
    float newM = m;
    // if moisture read failed, don't change m value 
    if (isnan(newM)) {
      Serial.println("Failed to read from Moisture sensor!");
    }
    else {
      m = newM;
      Serial.print("Moisture: ");
      // map to percent
      m = map(m,640,260,0,100);
      Serial.println(m);
    }
    
    String T = String(t).c_str();
    String M = String(m).c_str();
    String H = String(h).c_str();
    
    jsonSensors = "{\"temperature\":" + T + ", \"moisture\":" + M +  ", \"humidity\":" + H +  "}";
    Serial.println("jsonSensors: " + jsonSensors);

  }
}
