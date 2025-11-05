from machine import ADC
from machine import Pin
import network
import socket
import time

AP_NAME = "Nowling"
AP_KEY = "ForeverMine"

LED_ON = 1
LED_OFF = 0

SENSOR_DURATION = 10 * 60 * 1000 # 10 min in ms
LED_DURATION = 500 # ms

N_TRIES = 5

class Stopwatch:
    def __init__(self, duration_ms):
        self.duration_ms = duration_ms
        self.last_time = time.ticks_ms()
    
    def has_period_elapsed(self):
        current_time = time.ticks_ms()
        if (current_time - self.last_time) > self.duration_ms:
            self.last_time = current_time
            return True
        return False
    
class Connection:
    def __init__(self, ap_name, ap_key, server_name, port):
        self.ap_name = ap_name
        self.ap_key = ap_key
        self.server_name = server_name
        self.port = port
        
        self.nic = network.WLAN(network.WLAN.IF_STA)
        self.nic.active(False)
        time.sleep_ms(5000)
        self.nic.active(True)
        time.sleep_ms(5000)
        
        self.sock = None
    
    def try_connect_wifi(self):
        i = 0
        while not self.nic.isconnected():
            if i >= N_TRIES:
                return False
            
            self.nic.connect(AP_NAME, AP_KEY)
            time.sleep_ms(5000)
            i += 1
        
        return self.nic.isconnected()
    
    def try_connect(self):
        if not self.nic.isconnected():
            if self.try_connect_wifi():
                self.sock = None
            else:
                return False
        
        if self.sock is None:
            self.sock = socket.socket()
            sockaddr = socket.getaddrinfo(self.server_name, self.port)[0][-1]
            self.sock.connect(sockaddr)
        
        return True

def get_device_id(nic):
    return ":".join([hex(b)[2:] for b in nic.config("mac")])

def main_loop(conn, adc):
    device_id = get_device_id(conn.nic).encode("utf-8")
    led_stopwatch = Stopwatch(LED_DURATION)
    sensor_stopwatch = Stopwatch(SENSOR_DURATION)
    conn.try_connect()
    while True:
        if sensor_stopwatch.has_period_elapsed():
            if conn.try_connect():
                val = adc.read_u16() / 65535.0  # Read A0 port ADC value (65535~0)
                
                conn.sock.sendall(b"mac address\t")
                conn.sock.sendall(device_id)
                conn.sock.sendall(b"\n")
                conn.sock.sendall(b"soil capacitance\t")
                conn.sock.sendall(str(val).encode("utf-8"))
                conn.sock.sendall(b"\n")
                conn.sock.sendall(b"end\n")
        
        if led_stopwatch.has_period_elapsed():
            led.toggle()
        
        time.sleep_ms(100)
  
adc = ADC(0)  # ADC input (knob potentiometer) connected to A0
led = Pin("LED", Pin.OUT)
led.value(LED_OFF)
conn = Connection(AP_NAME, AP_KEY, '192.168.12.123', 2000)
led.value(LED_ON)
main_loop(conn, adc)

