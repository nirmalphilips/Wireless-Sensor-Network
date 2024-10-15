#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32_NTNK";
const char* password = "Password";

const char* htmlContent = R"html(
<!DOCTYPE html>
<html>
<head>
  <title>WSN Readings</title>
  <style>
    table {
      border-collapse: collapse;
      width: 100%;
    }
    th, td {
      padding: 8px;
      text-align: left;
      border-bottom: 1px solid #ddd;
    }
  </style>
</head>
<body>
  <h1 id="title">NTNK WSN READINGS</h1>
  <style>
  #title {
    font-size: 28px; 
    text-align: center;
  }
</style>


  <table>
    <tr>
      <th>Accelerometer</th>
      <th>Temperature</th>
      <th>Distance</th>
    </tr>
    <tr>
      <td id="accelData">%ACCEL_DATA%</td>
      <td id="tempData">%TEMP_DATA%</td>
      <td id="distanceData">%DISTANCE_DATA%</td>
    </tr>
  </table>
  <script>
    function updateData() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          document.getElementById('accelData').textContent = data.accelData;
          document.getElementById('tempData').textContent = data.tempData;
          document.getElementById('distanceData').textContent = data.distanceData;
        })
        .catch(error => console.error('Error:', error));
    }

    setInterval(updateData, 1000);
  </script>
</body>
</html>
)html";

Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);
const int temperaturePin = 32;
const float calibrationFactor = 20.0;  // Adjust this value to calibrate the LM35 sensor

const int trigPin = 12;
const int echoPin = 13;

WebServer server(80);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.print("Setting up access point...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(" Done.");
  Serial.print("Access Point IP address: ");
  Serial.println(IP);

  Serial.println("Initializing sensors...");
  if (!accel.begin()) {
    Serial.println("Failed to initialize LSM303 accelerometer!");
    while (1);
  }
  pinMode(temperaturePin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleDataRequest);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("Server started");
}

void loop() {
  server.handleClient();
  printSensorValues();
}

void handleRoot() {
  String htmlResponse = htmlContent;
  htmlResponse.replace("%ACCEL_DATA%", "N/A");
  htmlResponse.replace("%TEMP_DATA%", "N/A");
  htmlResponse.replace("%DISTANCE_DATA%", "N/A");
  server.send(200, "text/html", htmlResponse);
}

void handleDataRequest() {
  float accelX, accelY, accelZ;
  getAccelerometerData(accelX, accelY, accelZ);

  float temperature = getTemperature();
  float distance = getDistance();

  String data = "{\"accelData\":\"X: " + String(accelX, 2) + " Y: " + String(accelY, 2) + " Z: " + String(accelZ, 2) + "\",";
  data += "\"tempData\":\"" + String(temperature, 2) + " °C\",";
  data += "\"distanceData\":\"" + String(distance, 2) + " cm\"}";

  server.send(200, "application/json", data);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void getAccelerometerData(float& x, float& y, float& z) {
  sensors_event_t event;
  accel.getEvent(&event);
  x = event.acceleration.x;
  y = event.acceleration.y;
  z = event.acceleration.z;
}

float getTemperature() {
  int rawValue = analogRead(temperaturePin);
  float voltage = rawValue * (3.3 / 4095.0);
  float temperature = voltage * calibrationFactor;
  return temperature;
}

float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;
  return distance;
}

void printSensorValues() {
  float accelX, accelY, accelZ;
  getAccelerometerData(accelX, accelY, accelZ);

  Serial.print("Accelerometer: X=");
  Serial.print(accelX);
  Serial.print(" Y=");
  Serial.print(accelY);
  Serial.print(" Z=");
  Serial.println(accelZ);

  float temperature = getTemperature();
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");

  float distance = getDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  delay(500);
}
