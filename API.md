# ESP32-C3 Air Quality Monitor - API Documentation

## Overview

This ESP32-C3 device provides a RESTful HTTP API and MQTT publishing for monitoring indoor air quality using DHT22 (temperature/humidity) and MQ-135 (gas/CO2) sensors. All HTTP endpoints return JSON responses with CORS enabled by default.

## Base URL

- **mDNS**: `http://smart-kosan-odor-detection.local/`
- **IP Address**: Use the IP printed on serial console during boot (check `/net` endpoint)
- **Default Port**: `80`

## MQTT Publishing

- **Protocol**: MQTT v3.1.1
- **Default Interval**: 5 seconds (configurable)
- **Topic Format**: `kosan/room{roomId}/sensors`
- **QoS**: 0 (at most once)
- **Retain**: false

---

## Endpoints

### 1. Root / Health Check

**GET /**

Returns a simple text message indicating the API is running.

**Response:**
```
200 OK
Content-Type: text/plain

OK. GET /state for JSON.
```

---

### 2. Get Sensor State

**GET /state**

Returns current sensor readings (temperature, humidity, heat index, CO2).

**Response:**
```json
200 OK
Content-Type: application/json

{
  "t": 32.7,      // Temperature in °C
  "h": 81.9,      // Humidity in %
  "hi": 47.9,     // Heat index in °C
  "co2": 450,     // CO2 concentration in ppm
  "ts": 12345     // Timestamp in seconds since boot
}
```

**Notes:**
- Values are updated every ~2.5 seconds from the main loop
- Invalid sensor readings are replaced with `0`
- CO2 is a gas concentration approximation from MQ-135

---

### 3. Get Network Information

**GET /net**

Returns network connection details and device information.

**Response:**
```json
200 OK
Content-Type: application/json

{
  "mode": "STA",              // Connection mode: "STA", "AP", or "NONE"
  "ssid": "morning",          // Connected SSID (STA) or AP SSID
  "rssi": -45,                // Signal strength in dBm (STA only, 0 for AP)
  "ip": "192.168.1.100",      // Current IP address
  "mac": "AA:BB:CC:DD:EE:FF"  // MAC address
}
```

**Connection Modes:**
- `STA`: Connected to WiFi as station (client)
- `AP`: Running as Access Point (fallback mode)
- `NONE`: No network connection

---

### 4. Get MQ-135 Calibration Value

**GET /mq/r0**

Returns the current R0 calibration value for the MQ-135 gas sensor.

**Response:**
```json
200 OK
Content-Type: application/json

{
  "r0": 10.234  // Current R0 value in kΩ
}
```

**Error Response:**
```json
501 Not Implemented
Content-Type: application/json

{
  "r0": 0.0
}
```

**Notes:**
- R0 is the sensor resistance in clean air, used as baseline for gas calculations
- Calibration happens automatically during boot (5s warm-up + 100 samples)
- R0 depends on temperature, humidity, and sensor condition

---

### 5. Get MQTT Status

**GET /mqtt**

Returns MQTT connection status and publishing information.

**Response:**
```json
200 OK
Content-Type: application/json

{
  "connected": true,           // MQTT broker connection status
  "broker": "connected",       // Connection state description
  "topic": "kosan/room204/sensors",  // Publish topic
  "lastPublish": 12345,        // Timestamp of last successful publish (ms since boot)
  "interval": 5000             // Publishing interval in milliseconds
}
```

**Notes:**
- `connected: false` indicates no MQTT broker configured or connection failed
- `lastPublish: 0` means no successful publish yet
- Check serial console for detailed MQTT connection logs

---

### 6. Recalibrate MQ-135 Sensor

**GET /mq/recalibrate**

Triggers a new calibration of the MQ-135 sensor. Place the sensor in clean air before calling this endpoint.

**Query Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `s` | unsigned long | 100 | Number of calibration samples |
| `i` | unsigned long | 100 | Interval between samples in milliseconds |
| `w` | unsigned long | 3000 | Warm-up time in milliseconds before sampling |

**Example Requests:**
```
GET /mq/recalibrate
GET /mq/recalibrate?s=150&i=100&w=5000
GET /mq/recalibrate?w=10000
```

**Success Response:**
```json
200 OK
Content-Type: application/json

{
  "ok": true,
  "r0": 10.567,  // New R0 value after recalibration
  "s": 150,      // Samples used
  "i": 100,      // Interval used (ms)
  "w": 5000      // Warm-up time used (ms)
}
```

**Error Response:**
```json
501 Not Implemented
Content-Type: application/json

{
  "ok": false,
  "err": "recalibrate not available"
}
```

**Important:**
- ⚠️ This endpoint is **blocking** and will take `(w + s*i)` milliseconds to complete
- Example: `w=5000, s=100, i=100` → 5s + 10s = 15 seconds total
- Sensor readings and loop execution pause during recalibration
- **Only calibrate in clean, well-ventilated air** for accurate results
- Poor calibration will cause incorrect CO2/gas readings

---

## MQTT Data Format

### Published JSON Payload

The device publishes sensor data to the configured MQTT topic in the following JSON format:

```json
{
  "roomId": "204",
  "timestamp": 1728641234000,
  "temperature": 32.7,
  "humidity": 81.9,
  "heatIndex": 47.9,
  "co2": 450
}
```

**Field Descriptions:**

| Field | Type | Unit | Description |
|-------|------|------|-------------|
| `roomId` | string | - | Room identifier (configurable) |
| `timestamp` | number | ms | Milliseconds since device boot |
| `temperature` | number | °C | Temperature from DHT22 sensor |
| `humidity` | number | % | Relative humidity from DHT22 sensor |
| `heatIndex` | number | °C | Calculated heat index (feels-like temperature) |
| `co2` | number | ppm | CO2 concentration approximation from MQ-135 |

**Publishing Behavior:**
- Automatic publishing at configured interval (default: 5 seconds)
- Auto-reconnect on connection loss (retry every 5 seconds)
- Invalid sensor values replaced with `0`
- Serial console logs all publish events

**Example Topic:**
- Room 204: `kosan/room204/sensors`
- Room 101: `kosan/room101/sensors`

---

## CORS Support

All endpoints include CORS headers by default:
```
Access-Control-Allow-Origin: *
Cache-Control: no-cache
```

Preflight OPTIONS requests are supported on all routes.

---

## Error Handling

### HTTP Status Codes

| Code | Meaning |
|------|---------|
| 200 | Success |
| 204 | No Content (OPTIONS preflight) |
| 404 | Endpoint not found |
| 500 | Internal server error (calibration failed) |
| 501 | Feature not available (callback not registered) |

### Common Error Responses

**404 Not Found:**
```
404 Not Found
Content-Type: text/plain

Not found
```

---

## Network Configuration

### WiFi Setup

Basic WiFi configuration in `Arduino.ino`:

```cpp
Net::Config cfg;
cfg.ssid = "YourWiFi";
cfg.pass = "YourPassword";
cfg.hostname = "smart-kosan-odor-detection";
```

### Static IP Setup

To configure a static IP, modify `Arduino.ino` before compilation:

```cpp
// Uncomment and set your static IP
cfg.sta_ip = IPAddress(192, 168, 1, 100);
cfg.sta_gw = IPAddress(192, 168, 1, 1);
cfg.sta_sn = IPAddress(255, 255, 255, 0);
cfg.sta_dns1 = IPAddress(8, 8, 8, 8);
```

### MQTT Configuration

Enable MQTT publishing by configuring the broker in `Arduino.ino`:

```cpp
// MQTT Configuration
cfg.mqtt_server = "broker.hivemq.com";  // Broker hostname or IP
cfg.mqtt_port = 1883;                   // Broker port
cfg.mqtt_user = nullptr;                // Username (nullptr if no auth)
cfg.mqtt_pass = nullptr;                // Password (nullptr if no auth)
cfg.mqtt_client_id = "esp32c3-room204"; // Unique client ID
cfg.mqtt_topic = "kosan/room204/sensors"; // Publish topic
cfg.room_id = "204";                    // Room identifier in JSON payload
cfg.mqtt_interval_ms = 5000;            // Publish every 5 seconds
```

**Public MQTT Brokers (for testing):**
- `broker.hivemq.com:1883`
- `test.mosquitto.org:1883`
- `broker.emqx.io:1883`

**Private Broker Setup:**
1. Install Mosquitto on your server
2. Configure broker IP and credentials
3. Update `cfg.mqtt_server` with your broker address

**Required Library:**
- **PubSubClient** by Nick O'Leary (install via Arduino Library Manager)

### Access Point Fallback

If WiFi connection fails, the device automatically creates an access point:
- **SSID**: `ESP32C3-AP`
- **Password**: `pass12345`
- **IP**: `192.168.4.1`

---

## Sensor Specifications

### DHT22
- **Pin**: GPIO4
- **Voltage**: 3.3V
- **Temperature Range**: -40°C to 80°C (±0.5°C accuracy)
- **Humidity Range**: 0-100% (±2-5% accuracy)
- **Update Rate**: ~2.5 seconds

### MQ-135
- **Analog Pin**: GPIO2 (ADC1_CH2)
- **Digital Pin**: GPIO1 (optional threshold detection)
- **Voltage**: Module at 5V, AOUT divided to ≤3.3V for ESP32-C3
- **ADC Resolution**: 12-bit (0-4095)
- **Detectable Gases**: CO2, NH3, NOx, Alcohol, Benzene, Smoke
- **Warm-up Time**: Minimum 5 seconds (24-48 hours for full accuracy)
- **Load Resistor (RL)**: 10 kΩ

### SH1106 OLED
- **Protocol**: I2C
- **Address**: 0x3C
- **Resolution**: 128x64 pixels
- **Pins**: Default I2C (usually GPIO6=SDA, GPIO7=SCL for ESP32-C3)
- **Voltage**: 3.3V

---

## Example Usage

### HTTP API

#### Python
```python
import requests

# Get current sensor state
response = requests.get('http://smart-kosan-odor-detection.local/state')
data = response.json()
print(f"Temperature: {data['t']}°C")
print(f"CO2: {data['co2']} ppm")

# Check MQTT status
response = requests.get('http://smart-kosan-odor-detection.local/mqtt')
mqtt_status = response.json()
print(f"MQTT Connected: {mqtt_status['connected']}")
print(f"Last Publish: {mqtt_status['lastPublish']}ms ago")

# Recalibrate sensor (blocking for ~13 seconds)
response = requests.get(
    'http://smart-kosan-odor-detection.local/mq/recalibrate',
    params={'s': 100, 'i': 100, 'w': 3000}
)
print(f"New R0: {response.json()['r0']}")
```

#### JavaScript (Browser/Node.js)
```javascript
// Fetch sensor state
fetch('http://smart-kosan-odor-detection.local/state')
  .then(res => res.json())
  .then(data => {
    console.log(`Temperature: ${data.t}°C`);
    console.log(`Humidity: ${data.h}%`);
    console.log(`CO2: ${data.co2} ppm`);
  });

// Get network info
fetch('http://smart-kosan-odor-detection.local/net')
  .then(res => res.json())
  .then(data => console.log('Device IP:', data.ip));

// Check MQTT status
fetch('http://smart-kosan-odor-detection.local/mqtt')
  .then(res => res.json())
  .then(data => console.log('MQTT:', data));
```

#### cURL
```bash
# Get sensor state
curl http://smart-kosan-odor-detection.local/state

# Get network information
curl http://smart-kosan-odor-detection.local/net

# Get MQTT status
curl http://smart-kosan-odor-detection.local/mqtt

# Get current R0
curl http://smart-kosan-odor-detection.local/mq/r0

# Recalibrate with custom parameters
curl "http://smart-kosan-odor-detection.local/mq/recalibrate?s=200&i=100&w=5000"
```

### MQTT Subscriber

#### Mosquitto CLI
```bash
# Subscribe to specific room
mosquitto_sub -h broker.hivemq.com -t "kosan/room204/sensors" -v

# Subscribe to all rooms
mosquitto_sub -h broker.hivemq.com -t "kosan/+/sensors" -v

# With authentication
mosquitto_sub -h your-broker.com -t "kosan/room204/sensors" -u username -P password
```

#### Python with Paho MQTT
```python
import paho.mqtt.client as mqtt
import json

def on_message(client, userdata, message):
    data = json.loads(message.payload.decode())
    print(f"Room {data['roomId']}:")
    print(f"  Temperature: {data['temperature']}°C")
    print(f"  Humidity: {data['humidity']}%")
    print(f"  CO2: {data['co2']} ppm")

client = mqtt.Client()
client.on_message = on_message
client.connect("broker.hivemq.com", 1883, 60)
client.subscribe("kosan/room204/sensors")
client.loop_forever()
```

#### Node.js with MQTT.js
```javascript
const mqtt = require('mqtt');
const client = mqtt.connect('mqtt://broker.hivemq.com');

client.on('connect', () => {
  client.subscribe('kosan/room204/sensors', (err) => {
    if (!err) console.log('Subscribed to room 204');
  });
});

client.on('message', (topic, message) => {
  const data = JSON.parse(message.toString());
  console.log(`Room ${data.roomId}:`);
  console.log(`  Temperature: ${data.temperature}°C`);
  console.log(`  CO2: ${data.co2} ppm`);
});
```

---

## Troubleshooting

### mDNS Not Resolving
- **Windows**: Install Bonjour Print Services or use the IP address directly
- **Linux**: Install `avahi-daemon` (usually pre-installed)
- **Android**: mDNS support varies; use IP address or install a mDNS-aware browser
- **Fallback**: Use the IP address from serial console or `/net` endpoint

### MQTT Not Connecting
- **Check broker accessibility**: Ping the broker from your network
- **Verify credentials**: Ensure username/password are correct (if required)
- **Port forwarding**: Check if port 1883 is open/forwarded
- **Serial console**: Look for MQTT connection errors and state codes
- **Test broker**: Try public brokers first (broker.hivemq.com)
- **Client ID conflict**: Ensure unique client IDs if running multiple devices

**MQTT State Codes:**
- `-4` : Connection timeout
- `-3` : Connection lost
- `-2` : Connect failed
- `-1` : Disconnected
- `0` : Connected
- `1` : Bad protocol
- `2` : Bad client ID
- `3` : Unavailable
- `4` : Bad credentials
- `5` : Unauthorized

### MQTT Publishing Issues
- Check `/mqtt` endpoint for connection status
- Verify topic permissions on broker
- Ensure sufficient memory (MQTT + HTTP can be memory-intensive)
- Check `mqtt_interval_ms` isn't too short (min 1000ms recommended)
- Monitor serial console for publish success/failure logs

### CO2 Reading Shows "inf" or "ERR"
- Check wiring: AOUT must be divided to ≤3.3V before GPIO2
- Ensure proper warm-up time (5-10 seconds minimum)
- Recalibrate in clean air using `/mq/recalibrate?w=10000`
- Check sensor power (heater needs 5V, stable supply)

### Calibration Takes Too Long
- Default: 3s warm-up + 10s sampling = 13 seconds
- Reduce samples: `/mq/recalibrate?s=50&i=100&w=2000` = 7 seconds
- For production: increase for better accuracy

### WiFi Connection Failed
- Check SSID and password in code
- Device will fallback to AP mode (connect to `ESP32C3-AP`)
- Check serial console for connection status
- Verify 2.4GHz WiFi (ESP32-C3 doesn't support 5GHz)

### Memory Issues
- If device crashes/resets frequently, reduce MQTT publish frequency
- Disable AP fallback if not needed: `cfg.enable_ap_fallback = false`
- Consider removing unused endpoints or features

---

## Integration Examples

### Home Assistant (MQTT Auto-Discovery)
```yaml
# configuration.yaml
mqtt:
  sensor:
    - name: "Room 204 Temperature"
      state_topic: "kosan/room204/sensors"
      unit_of_measurement: "°C"
      value_template: "{{ value_json.temperature }}"
      
    - name: "Room 204 Humidity"
      state_topic: "kosan/room204/sensors"
      unit_of_measurement: "%"
      value_template: "{{ value_json.humidity }}"
      
    - name: "Room 204 CO2"
      state_topic: "kosan/room204/sensors"
      unit_of_measurement: "ppm"
      value_template: "{{ value_json.co2 }}"
```

### Node-RED Flow
```json
[
  {
    "id": "mqtt-in",
    "type": "mqtt in",
    "topic": "kosan/+/sensors",
    "broker": "broker-config"
  },
  {
    "id": "json-parse",
    "type": "json"
  },
  {
    "id": "function",
    "type": "function",
    "func": "if (msg.payload.co2 > 1000) {\n  msg.payload.alert = 'High CO2';\n}\nreturn msg;"
  }
]
```

### InfluxDB + Grafana
```python
from influxdb_client import InfluxDBClient, Point
import paho.mqtt.client as mqtt
import json

influx = InfluxDBClient(url="http://localhost:8086", token="your-token", org="your-org")
write_api = influx.write_api()

def on_message(client, userdata, message):
    data = json.loads(message.payload.decode())
    point = Point("air_quality") \
        .tag("room", data["roomId"]) \
        .field("temperature", data["temperature"]) \
        .field("humidity", data["humidity"]) \
        .field("co2", data["co2"])
    write_api.write(bucket="sensors", record=point)

mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message
mqtt_client.connect("broker.hivemq.com", 1883)
mqtt_client.subscribe("kosan/+/sensors")
mqtt_client.loop_forever()
```

---

## Version Information

- **Device**: ESP32-C3
- **Firmware**: Custom Arduino sketch
- **Protocol Support**: HTTP/1.1, MQTT v3.1.1
- **Libraries**:
  - ESP32 Arduino Core
  - Adafruit GFX & SH110X
  - DHT sensor library
  - MQUnifiedsensor
  - ESPmDNS
  - WebServer
  - PubSubClient (MQTT)

---

## Security Notes

⚠️ **This API is designed for local network use only**

- No authentication or encryption on HTTP endpoints
- MQTT supports basic username/password authentication
- CORS allows all origins
- WiFi and MQTT credentials stored in plain text
- Suitable for home/lab environments
- **Do not expose directly to the internet**

For production deployment, consider:
- Adding API key authentication for HTTP endpoints
- Using HTTPS (ESP32 supports TLS)
- Using MQTTS (MQTT over TLS) on port 8883
- Restricting CORS origins
- Implementing rate limiting
- Storing credentials securely (ESP32 Preferences/NVS)
- Firewall rules to limit access
- VPN for remote access

---

## Performance Notes

### Resource Usage
- **Memory**: ~60-80KB used (varies with WiFi/MQTT buffers)
- **CPU**: Minimal (<5% with 5s intervals)
- **Network**: ~200 bytes per MQTT publish
- **Power**: ~150-200mA average (ESP32-C3 + sensors + OLED)

### Optimization Tips
- Increase `mqtt_interval_ms` to reduce network traffic
- Disable OLED if not needed to save ~20mA
- Use deep sleep between readings for battery operation
- Batch multiple readings before publishing

---

## License & Support

For issues, modifications, or questions about this API, refer to the project documentation or contact the development team.

**Project**: ESP32-C3 Air Quality Monitor  
**Last Updated**: October 22, 2025  
**API Version**: 2.0 (added MQTT support)
