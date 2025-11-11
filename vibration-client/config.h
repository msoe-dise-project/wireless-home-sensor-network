#ifndef _CONFIG_H
#define _CONFIG_H

//SSID of your network
const char ssid[] = "";

//password of your WPA Network
const char pass[] = "";

// event periods
const float SEC_TO_MS = 1000; // ms
const float MIN_TO_SEC = 60; // sec

const long MAX_SAMPLE_RATE_HZ = 10; // Hz
const unsigned long SAMPLE_MIN_ELAPSED_MS = 1.0 / MAX_SAMPLE_RATE_HZ * SEC_TO_MS;

const float SEND_PERIOD_MIN = 3; // minutes
const unsigned long SEND_MIN_ELAPSED_MS = SEND_PERIOD_MIN * MIN_TO_SEC * SEC_TO_MS;

// 128 is for buffer
const unsigned long N_SAMPLES = (SEND_PERIOD_MIN * MIN_TO_SEC * MAX_SAMPLE_RATE_HZ) + 128;

const float BLINK_PERIOD_MS = 1000; // ms
const unsigned long BLINK_MIN_ELAPSED_MS = BLINK_PERIOD_MS;

// server
const IPAddress SERVER_IP(192, 168, 12, 123);
const int SERVER_PORT = 2000;

// network connection parameters
const int N_CONNECTION_ATTEMPTS = 5;
const unsigned long CONNECTION_WAIT_TIME_MS = 3000;

// send log data to serial console?
// needs to be off if you want to use the device
// without being connected to the computer
#define DEBUG false

#endif // _CONFIG_H
