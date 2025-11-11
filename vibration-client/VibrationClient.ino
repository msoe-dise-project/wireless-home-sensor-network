/*
  Vibration sensor logger and transmitter.

  Reads acceleration in X, Y, and Z and computes the magnitude every 100 ms.
  Computes summary statistics for 3 min of data and sends it to a server over WiFi.
  Standard deviation is kind of a poor man's measure of amplitude.
*/

#include <SPI.h>
#include <WiFiNINA.h>
#include <Arduino_LSM6DSOX.h>

#include "config.h"

// sensor readings
float accelX, accelY, accelZ;
float accelMagnSamples[N_SAMPLES];
unsigned long sampleIdx = 0;

// record last timestamps of these events
// to use for determining when to perform them next
unsigned long lastSendTime = 0;
unsigned long lastBlinkTime = 0;
unsigned long lastSampleTime = 0;

// used to flip blink
int LED_STATE = HIGH;

WiFiClient client;

// the MAC address of the WiFi Module
// used to uniquely identify ourselves to the server
byte mac[6];

typedef struct {
  float mean;
  float stddev;
  float max;
  float min;
} SummaryStats;

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

void clearBuffer() {
  for(int i = 0; i < N_SAMPLES; i++) {
    accelMagnSamples[i] = 0.0f;
  }

  sampleIdx = 0;
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

  if(!IMU.begin()) {
    if(DEBUG) {
      Serial.println("Could not start IMU!");
    }

    failLoop();
  } else {
    if(DEBUG) {
      Serial.println("Sucessfully started IMU!");
    }
  }

  clearBuffer();

  tryConnectIfNeeded();
}

bool enoughTimeElapsedSinceLastSample() {
  unsigned long elapsedMS = millis() - lastSampleTime;
  return elapsedMS >= SAMPLE_MIN_ELAPSED_MS;
}

bool enoughTimeElapsedSinceLastSend() {
  unsigned long elapsedMS = millis() - lastSendTime;
  return elapsedMS >= SEND_MIN_ELAPSED_MS;
}

bool enoughTimeElapsedSinceLastBlink() {
  unsigned long elapsedMS = millis() - lastBlinkTime;
  return elapsedMS >= BLINK_MIN_ELAPSED_MS;
}

bool readSensors() {
  // prevent writing past array or wrapping around
  if(IMU.accelerationAvailable() and sampleIdx < N_SAMPLES) {
      IMU.readAcceleration(accelX, accelY, accelZ);
      float accelMagn = sqrt(sq(accelX) + sq(accelY) + sq(accelZ));
      accelMagnSamples[sampleIdx++] = accelMagn;
    return true;
  }

  return false;
}

bool calculateSummaryStats(SummaryStats &stats) {
  if(sampleIdx < 2) {
    return false;
  }

  float sum = 0.0;
  stats.min = 1000.0;
  stats.max = 0.0;
  for(int i = 0; i < sampleIdx; i++) {
    sum += accelMagnSamples[i];
    if (accelMagnSamples[i] < stats.min) {
      stats.min = accelMagnSamples[i];
    }
    if(accelMagnSamples[i] > stats.max) {
      stats.max = accelMagnSamples[i];
    }
  }

  stats.mean = sum / (sampleIdx + 1);
  
  float diff_sq_sum = 0.0;
  for(int i = 0; i < sampleIdx; i++) {
    diff_sq_sum += sq(accelMagnSamples[i] - stats.mean);
  }

  // corrected for bias
  // N = sample_idx + 1
  // N - 1 = sample_idx + 1 - 1 = sampleIdx
  stats.stddev = sqrt(diff_sq_sum / (sampleIdx - 1));

  return true;
}

void loop() {
  if(enoughTimeElapsedSinceLastBlink()) {
    blink();
    lastBlinkTime = millis();
  }

  if(enoughTimeElapsedSinceLastSample()) {
    if(readSensors()) {
      lastSampleTime = millis();
    }
  }

  if(enoughTimeElapsedSinceLastSend()) {
    tryConnectIfNeeded();

    SummaryStats winStats;
    if(calculateSummaryStats(winStats)) {
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
        Serial.print("Mean is: ");
        Serial.println(winStats.mean, 4);
        Serial.print("Std dev is: ");
        Serial.println(winStats.stddev, 4);
        Serial.print("Max is: ");
        Serial.println(winStats.max, 4);
        Serial.print("Min is: ");
        Serial.println(winStats.min);
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

      client.print("win_accel_magn_mean\t");
      client.print(winStats.mean, 4);
      client.println();
      client.print("win_accel_magn_min\t");
      client.print(winStats.min, 4);
      client.println();
      client.print("win_accel_magn_max\t");
      client.print(winStats.max, 4);
      client.println();
      client.print("win_accel_magn_stddev\t");
      client.print(winStats.stddev, 4);
      client.println();
      client.print("end");
      client.println();

      lastSendTime = millis();

      clearBuffer();
    }
  }

  // maybe saves energy?
  delay(10);
}
