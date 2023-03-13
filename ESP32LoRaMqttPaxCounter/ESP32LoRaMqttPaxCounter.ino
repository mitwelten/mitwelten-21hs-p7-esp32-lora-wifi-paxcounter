// https://fhnw.mit-license.org/

// based on https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/

// Based on https://github.com/mcci-catena/arduino-lmic/tree/master/examples
// Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
// Copyright (c) 2018 Terry Moore, MCCI
// Licensed under MIT License

// change according to: mcci-catena/arduino-lmic/issues/714
// add :
// #define hal_init LMICHAL_init
// to arduino-lmic/src/hal/hal.cpp

// TODO: Include MCCI LoRaWAN LMIC library, https://github.com/mcci-catena/arduino-lmic
// TODO: Enable CFG_eu868 in ARDUINO_HOME/libraries/arduino-lmic/project_config/lmic_project_config.h

#define SERIAL_DEBUG 0  // set to 1 to enable serial debug

#define DISABLE_PING     // https://github.com/mcci-catena/arduino-lmic#disabling-ping
#define DISABLE_BEACONS  // https://github.com/mcci-catena/arduino-lmic#disabling-beacons
#define LMIC_ENABLE_arbitrary_clock_error 1

#define MQTT_MAX_PACKET_SIZE 512

#include <BLEAdvertisedDevice.h>  // https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <CRC32.h>             // https://github.com/bakercp/CRC32
#include <PubSubClient.h>      // https://github.com/knolleary/pubsubclient
#include <SPI.h>               // https://github.com/espressif/arduino-esp32/tree/master/libraries/SPI
#include <WiFi.h>              // https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi
#include <WiFiClientSecure.h>  // https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure
#include <lmic.h>              // https://github.com/mcci-catena/arduino-lmic
#include <hal/hal.h>           // https://github.com/mcci-catena/arduino-lmic
#include <ssl_client.h>        // https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure

#include "WebConfig.h"

#define BATTERY_PIN A13  // GPIO #35 Feather ESP32,

#define ONBOARD_LED_PIN 13  // Feather ESP32

#define CONFIG_ENABLE_PIN 36  // A4

// LoRa Module Pin mapping, see https://github.com/tamberg/fhnw-iot/wiki/FeatherWing-RFM95W
const lmic_pinmap lmic_pins = {
    .nss = 14,  // E = CS
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 32,   // D = RST
    .dio = {33,  // B = DIO0 = IRQ
            15,  // C = DIO1
            LMIC_UNUSED_PIN},
};

#define CONNECT_TIMEOUT_MS 15000  // restart the ESP32 after CONNECT_TIMEOUT_MS if WiFi or MQTT Connection failed

#define MAC_BUFFER_SIZE 512    // maximum number of unique ble devices
#define BLE_SCAN_INTERVAL 200  // ble scan interval in ms
#define BLE_SCAN_WINDOW 200    // ble scan window in ms (must <= BLE_SCAN_INTERVAL)

WebConfig webConfig;

nodeConfig cnf;

BLEScan *pBLEScan;

WiFiClientSecure espClient_s;

WiFiClient espClient;

PubSubClient mqttClient;

static uint8_t payload[4];
static osjob_t sendjob;

RTC_DATA_ATTR uint32_t randomizedDeviceCrcs[MAC_BUFFER_SIZE];
RTC_DATA_ATTR uint16_t countedDevices = 0;
RTC_DATA_ATTR uint8_t scanCounter = 0;

bool config_mode = false;
ulong scanStartTime;
ulong nextTXMillis = 0;
uint32_t scanning_duration = 0;

void os_getArtEui(u1_t *buf) { memcpy(buf, cnf.appeui, 8); }

void os_getDevEui(u1_t *buf) { memcpy(buf, cnf.deveui, 8); }

void os_getDevKey(u1_t *buf) { memcpy(buf, cnf.appkey, 16); }

void setup() {
    if (SERIAL_DEBUG) {
        Serial.begin(115200);
        delay(2000);
    }

    pinMode(CONFIG_ENABLE_PIN, INPUT_PULLDOWN);
    pinMode(ONBOARD_LED_PIN, OUTPUT);

    loadConfiguration();  // load configuration from flash
    cnf = getNodeConfig();
    if (SERIAL_DEBUG) webConfig.printNodeConf();
    scanning_duration = cnf.scan_duration_ms;

    int stoptime = millis() + 2000;  // 2 seconds to press the button to enter config mode
    while (millis() < stoptime) {
        if (digitalRead(CONFIG_ENABLE_PIN) == HIGH) {  // button pressed
            config_mode = true;
            digitalWrite(ONBOARD_LED_PIN, HIGH);

            break;
        }
        digitalWrite(ONBOARD_LED_PIN, HIGH);
        delay(100);
        digitalWrite(ONBOARD_LED_PIN, LOW);
        delay(100);
    }

    if (config_mode) {
        if (SERIAL_DEBUG) Serial.println(F("Config_Mode, setting up server.."));
        webConfig.startConfigServer();
        webConfig.processDNSRequests();  // loop forever
    }

    setupBleScanner();

    if (SERIAL_DEBUG) Serial.print(F("Conn_Mode = "));

    if (cnf.use_mqtt) {
        if (SERIAL_DEBUG) Serial.print(F("WiFi / MQTT"));
        if (cnf.use_tls) {
            mqttClient.setClient(espClient_s);
            espClient_s.setInsecure();
        } else {
            mqttClient.setClient(espClient);
        }
        mqttClient.setServer(cnf.mqtt_broker_url.c_str(), cnf.mqtt_port);
        initWiFi();
        connectWiFi();
        connectMQTT(false);
        nextTXMillis = millis() + cnf.txInterval_s * 1000;
    } else {
        if (SERIAL_DEBUG) Serial.println(F("LoRa"));
        if (cnf.use_otaa) {
            setupLoRaOtaa();
        } else {
            setupLoRaAbp();
        }
        nextTXMillis = millis() + cnf.txInterval_s * 1000;
        periodicScanUntil(nextTXMillis);
        do_send(&sendjob);  // handles otaa join too
    }
}

void loop() {
    if (cnf.use_mqtt) {
        if (millis() >= nextTXMillis) {
            nextTXMillis = millis() + cnf.txInterval_s * 1000;
            checkMQTTConnection();
            if (!publishPAX()) {
                if (SERIAL_DEBUG) Serial.println(F("failed to publish!"));
                connectMQTT(true);
            }
            clearCRCs();
        } else {
            periodicScanUntil(nextTXMillis);
        }
    } else {  // LoRa
        os_runloop_once();
    }
}

// Scan for BLE devices until nextTXMillis is reached
void periodicScanUntil(ulong nextTXMillis) {
    while (millis() < nextTXMillis) {
        long time_until_tx = nextTXMillis - millis();
        if (time_until_tx > scanning_duration + 1) {  // enough time to scan
            scanPAX();
        }

        time_until_tx = nextTXMillis - millis();
        if (time_until_tx > 0) {
            if (time_until_tx >
                cnf.scan_interval_s * 1000 - scanning_duration) {
                waitUntil(cnf.scan_interval_s * 1000 - scanning_duration);
            } else {
                waitUntil(time_until_tx - 1);
            }
        }
    }
}

// Turn on WiFi and set STA mode
void initWiFi() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
}

// Connect to the WiFi network if not already connected, reboot ESP after CONNECT_TIMEOUT_MS
void connectWiFi() {
    long rebootMillis = millis() + CONNECT_TIMEOUT_MS;
    if (WiFi.status() != WL_CONNECTED) {
        if (SERIAL_DEBUG) Serial.print(F("connecting WiFi"));
        WiFi.begin(cnf.ssid.c_str(), cnf.wifi_password.c_str());

        while (WiFi.status() != WL_CONNECTED) {
            delay(100);
            if (SERIAL_DEBUG) Serial.print(".");
            if (millis() > rebootMillis) ESP.restart();
        }
        if (SERIAL_DEBUG) Serial.println(F("ok."));
    }
}

// Connect to the MQTT broker if not already connected, reboot ESP after CONNECT_TIMEOUT_MS
void connectMQTT(bool force) {
    long rebootMillis = millis() + CONNECT_TIMEOUT_MS;
    if (SERIAL_DEBUG) {
        Serial.print(F("mqtt state = "));
        Serial.println(mqttClient.state());
        Serial.print(F("mqtt connected = "));
        Serial.println(mqttClient.connected());
    }
    if (force) {
        if (SERIAL_DEBUG) Serial.print(F("Forcing mqtt connect, connected = "));
        mqttClient.disconnect();
        delay(100);
        if (SERIAL_DEBUG) Serial.println(mqttClient.connected());
    }
    while (!mqttClient.connected()) {
        if (SERIAL_DEBUG) {
            Serial.println(F("Connecting to:"));
            Serial.println(cnf.mqtt_broker_url);
        }
        if (mqttClient.connect(cnf.node_id.c_str(), cnf.mqtt_username.c_str(),
                               cnf.mqtt_password.c_str())) {
            delay(100);
            break;
        }
        delay(4000);
        if (SERIAL_DEBUG) {
            Serial.print(F("state = "));
            Serial.println(mqttClient.state());
        }

        if (millis() > rebootMillis) ESP.restart();
    }
}

// Check if WiFi and MQTT are connected, if not reconnect
void checkMQTTConnection() {
    connectWiFi();
    connectMQTT(false);
}

// Send a message to the MQTT broker
bool publishPAX() {
    String message = "{\"pax\":";
    message += String(countedDevices);
    message += ",\"vbatt\":";
    message += String(getBatteryLevel());
    message += ",\"scans\":";
    message += String(scanCounter);
    message += ",\"latitude\":";
    message += cnf.latitude;
    message += ",\"longitude\":";
    message += cnf.longitude;
    message += ",\"name\":\"pax\",\"node_id\":\"";
    message += cnf.node_id;
    message += "\",\"macfilter\":";
    cnf.apply_mac_filter ? message += "1" : message += "0";
    message += "}";
    if (SERIAL_DEBUG)
        Serial.printf("publishing message with size %d\n", message.length());
    return mqttClient.publish(cnf.mqtt_topic.c_str(), message.c_str());
}

// Update LoRa payload
void updatePayload() {
    int batteryVoltage = 50.0 * getBatteryLevel();
    payload[0] = highByte(countedDevices);
    payload[1] = lowByte(countedDevices);
    if (batteryVoltage < 255) {
        payload[2] = (uint8_t)batteryVoltage;
    } else {
        payload[2] = 0;
    }
    payload[3] = (uint8_t)cnf.apply_mac_filter;
}

// Queue packet for sending, set next TX time
void do_send(osjob_t *j) {
    if (SERIAL_DEBUG) Serial.println(F("doSend()"));
    if ((LMIC.opmode & OP_TXRXPEND) == 0) {  // No TX/RX pending
        updatePayload();
        LMIC_setTxData2(1, payload, sizeof(payload), 0);  // queue TX packet
        nextTXMillis = millis() + cnf.txInterval_s * 1000;
        if (SERIAL_DEBUG) Serial.println(F("Packet queued"));
    }
}

// LoRa callback
void onEvent(ev_t ev) {
    if (ev == EV_JOINED) {  // OTAA joined network
        u4_t netid = 0;
        devaddr_t devaddr = 0;
        u1_t nwkKey[16];
        u1_t artKey[16];
        LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
        if (SERIAL_DEBUG) {
            Serial.println(F("EV_JOINED"));
            Serial.print(F("netid: "));
            Serial.println(netid, DEC);
            Serial.print(F("devaddr: "));
            Serial.println(devaddr, HEX);
        }

        LMIC_setLinkCheckMode(0);
        LMIC_setDrTxpow(12 - cnf.loraSf, 14);
        LMIC_setAdrMode(cnf.use_adr);
    }
    if (ev == EV_JOIN_TXCOMPLETE) {  // OTAA join failed
        if (SERIAL_DEBUG) Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
    }
    if (ev == EV_TXCOMPLETE) {  // Packet transmitted
        if (SERIAL_DEBUG) Serial.println(F("EV_TXCOMPLETE"));
        clearCRCs();
        periodicScanUntil(nextTXMillis - 10);
        os_setTimedCallback(&sendjob, os_getTime() + ms2osticksRound(10),
                            do_send);
    } else {
        if (SERIAL_DEBUG) Serial.printf("Event : %u\n", (unsigned)ev);
    }
}

void setupLoRaOtaa() {
    LMIC_setClockError(MAX_CLOCK_ERROR * 5 / 100);
    os_init();
    LMIC_reset();
}

void setupLoRaAbp() {
    delay(100);
    os_init();
    LMIC_reset();
    LMIC_setSession(0x13, cnf.devAddr, cnf.nwkskey, cnf.appskey);
#if defined(CFG_eu868)
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK, DR_FSK), BAND_MILLI);
#else
#error Region not supported
#endif
    LMIC_setLinkCheckMode(0);
    LMIC.dn2Dr = DR_SF9;                   // TTN RX2 window
    LMIC_setDrTxpow(12 - cnf.loraSf, 14);  // set SF according to config
    LMIC_setAdrMode(cnf.use_adr);          // set ADR according to config
}

// Lightsleep for LoRa connectivity, maintain mqtt connection for MQTT connectivity
void waitUntil(uint32_t sleepDurationMs) {
    if (!cnf.use_mqtt) {
        if (sleepDurationMs > 100) {
            esp_sleep_enable_timer_wakeup((sleepDurationMs - 100) * 1000);
            delay(100);
            esp_err_t sleepok = esp_light_sleep_start();
        } else {
            delay(sleepDurationMs);
        }
    } else {  // maintain mqtt connection
        ulong stopMillis = millis() + sleepDurationMs;
        while (millis() < stopMillis) {
            mqttClient.loop();
        }
    }
}

uint32_t calculateCRC32(const char *payload) {
    return CRC32::calculate(payload, sizeof(payload) - 1);
}

void clearCRCs() {
    for (int i = 0; i < countedDevices; ++i) {
        randomizedDeviceCrcs[i] = 0;
    }
    countedDevices = 0;
    scanCounter = 0;
}

// check if CRC is already known, if not add it to the list
boolean addCRCIfNew(uint32_t crc) {
    if (countedDevices >= MAC_BUFFER_SIZE) {
        return false;
    }
    for (int i = 0; i < countedDevices; ++i) {
        if (randomizedDeviceCrcs[i] == crc) {
            return false;
        }
    }
    randomizedDeviceCrcs[countedDevices] = crc;
    countedDevices++;
    return true;
}

void countDevice(const char *deviceaddress) {
    addCRCIfNew(calculateCRC32(deviceaddress));
}

// BLE callback
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (cnf.apply_mac_filter) {  // only count devices with addressType 0x01
            if (advertisedDevice.getAddressType() == 0x01) {
                countDevice(advertisedDevice.getAddress().toString().c_str());
            }
        } else {  // count all devices
            countDevice(advertisedDevice.getAddress().toString().c_str());
        }
    }
};

void setupBleScanner() {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();  // create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(false);  // active scan uses more power, but get results faster
    pBLEScan->setInterval(BLE_SCAN_INTERVAL);
    pBLEScan->setWindow(BLE_SCAN_WINDOW);  // less or equal setInterval value
}

// scan for BLE devices
void scanPAX() {
    scanStartTime = millis();
    if (SERIAL_DEBUG) Serial.println(F("scanning.."));
    BLEScanResults foundDevices = pBLEScan->start((uint32_t)(cnf.scan_duration_ms / 1000), false);
    pBLEScan->clearResults();  // delete results fromBLEScan buffer to release memory
    scanCounter++;
    scanning_duration = millis() - scanStartTime;  // calculate time used for scanning
}

float getBatteryLevel() {
    return (analogRead(BATTERY_PIN) / 4095.0) * 2 * 1.1 * 3.3;
}