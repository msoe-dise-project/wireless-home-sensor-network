/*
  Sensor logger and transmitter

  Reads temperature and humidity sensor readings and sends them to a server over WiFi.
*/

#include <SPI.h>
#include <WiFiNINA.h>
#include <Modulino.h>

#include "config.h"

// sensor readings
float temperatureC;
float temperatureF;
float humidity;

// record last timestamps of these events
// to use for determining when to perform them next
unsigned long lastSendTime = 0;
unsigned long lastBlinkTime = 0;

// used to flip blink
int LED_STATE = HIGH;

ModulinoThermo thermo;

WiFiClient client;

// the MAC address of the WiFi Module
// used to uniquely identify ourselves to the server
byte mac[6];                     

void blink() {
  digitalWrite(LED_BUILTIN, LED_STATE);
  if(LED_STATE == HIGH) {
    LED_STATE = LOW;
  } else {
    LED_STATE = HIGH;
  }
}

// when we fail, just loop infinitely
// and blink the LED quickly
void failLoop() {
  while(true) {
    blink();
    delay(100);
  }
}

// reconnects the WiFi and TCP connections if needed
void tryConnectIfNeeded() {
  if(WiFi.status() != WL_CONNECTED && DEBUG) {
    Serial.println("Not connected to WiFi network");
  }

  // connect to WiFi if needed
  for(int i = 0; i < N_CONNECTION_ATTEMPTS && WiFi.status() != WL_CONNECTED; i++) {
    if(DEBUG) {
      Serial.print("Attempting to connect to WPA network, SSID: ");
      Serial.println(ssid);
    }

    WiFi.begin(ssid, pass);

    if(WiFi.status() != WL_CONNECTED && DEBUG) {
      Serial.println("Failed to connect to wifi");
    } else if(DEBUG) {
      Serial.println("You're connected to the network");
    }
  }

  // grab the MAC address to use as a unique identifier
  if(WiFi.status() == WL_CONNECTED) {
    WiFi.macAddress(mac);
  } else {
    if(DEBUG) {
      Serial.print("Failed to connect to wifi after ");
      Serial.print(N_CONNECTION_ATTEMPTS);
      Serial.println(" attempts");
    }

    failLoop();
  }

  if(!client.connected() && DEBUG) {
    Serial.println("TCP/IP connection is not open");
  }

  // establish TCP/IP connection to server if needed
  for(int i = 0; i < N_CONNECTION_ATTEMPTS && !client.connected(); i++) {
    if(DEBUG) {
      Serial.println("Attempting to open TCP/IP connection to server");
    }

    if(!client.connect(SERVER_IP, SERVER_PORT)) {
      if(DEBUG) {
        Serial.println("Failed to open TCP/IP connection");
      }
    } else if(DEBUG) {
        Serial.println("Successfully opened TCP/IP connection");
    }
  }

  if(!client.connected()) {
    if(DEBUG) {
      Serial.print("Failed to open TCP/IP connection after ");
      Serial.print(N_CONNECTION_ATTEMPTS);
      Serial.println(" attempts");
    }

    failLoop();
  }
}

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  if(DEBUG) {
    Serial.begin(9600);
    // wait for serial port to connect
    while(!Serial);
    Serial.println("Hello world!");
  }

  // returns void -- can't check status
  Modulino.begin();

  if(!thermo.begin()) {
    if(DEBUG) {
      Serial.println("Could not start thermometer!");
    }

    failLoop();
  } else {
    if(DEBUG) {
      Serial.println("Thermometer started!");
    }
  }
}

bool enoughTimeElapsedSinceLastSend() {
  unsigned long elapsedMS = millis() - lastSendTime;
  return elapsedMS >= SEND_MIN_ELAPSED_MS;
}

bool enoughTimeElapsedSinceLastBlink() {
  unsigned long elapsedMS = millis() - lastBlinkTime;
  return elapsedMS >= BLINK_MIN_ELAPSED_MS;
}

void loop() {
  if(enoughTimeElapsedSinceLastBlink()) {
    blink();
    lastBlinkTime = millis();
  }

  if(enoughTimeElapsedSinceLastSend()) {
    tryConnectIfNeeded();

    temperatureC = thermo.getTemperature();
    temperatureF = temperatureC * 9.0 / 5.0 + 32.0;
    humidity = thermo.getHumidity();

    if(DEBUG) {
      Serial.print("The mac address is: ");
      Serial.print(mac[5], HEX);
      Serial.print(":");
      Serial.print(mac[4], HEX);
      Serial.print(":");
      Serial.print(mac[3], HEX);
      Serial.print(":");
      Serial.print(mac[2], HEX);
      Serial.print(":");
      Serial.print(mac[1], HEX);
      Serial.print(":");
      Serial.print(mac[0], HEX);
      Serial.println();
      Serial.print("Temperature is: ");
      Serial.println(temperatureF, 4);
      Serial.print("Humidity is: ");
      Serial.println(humidity, 4);
    }

    client.print("mac address\t");
    client.print(mac[5], HEX);
    client.print(":");
    client.print(mac[4], HEX);
    client.print(":");
    client.print(mac[3], HEX);
    client.print(":");
    client.print(mac[2], HEX);
    client.print(":");
    client.print(mac[1], HEX);
    client.print(":");
    client.print(mac[0], HEX);
    client.println();

    client.print("temperature\t");
    client.print(temperatureF, 4);
    client.println();
    client.print("humidity\t");
    client.print(humidity, 4);
    client.println();
    client.print("end");
    client.println();

    lastSendTime = millis();
  }

  // maybe saves energy?
  delay(100);
}
