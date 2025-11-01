# Wireless Home Sensor Network
Internet of Things (IoT) project to have "things" that collect sensor data and send it to a centralized server over a WiFi network.
The server logs the readings in a SQLite database.

## Hardware Used
* [Arduino Nano RP2040 Connect](https://docs.arduino.cc/hardware/nano-rp2040-connect/)
* [Arduino Nano Connector Carrier](https://docs.arduino.cc/hardware/nano-connector-carrier/)
* [Arduino Modulino Thermo](https://docs.arduino.cc/hardware/modulino-thermo/)
* [USB power adapter](https://www.amazon.com/dp/B0C8HHV9DK)

## Instructions:
1. Start the server in the `asyncio-server` directory:
   ```bash
   $ python3 main.py --host 0.0.0.0 --port 2000 --dbname sensor_readings.sqlite
   ```
1. Assemble a sensor package and connect to your computer.
1. Load the sketch in the `client` directory to the device in the Arduino IDE.
1. Set `debug` to `true` and put your WiFi's SSID and password and server's IP address and port in the appropriate variables.
1. Upload the sketch.
1. Open the serial monitor and check the output of the server.  Wait a few minutes to ensure the device can connect to the server.
1. Set `debug` to `false` and re-upload the sketch
1. Put the sensor device where you want it, powered by being connected to the USB power adapter.
1. Wait a few minute and then check the server output to ensure that it is properly transmitting.
