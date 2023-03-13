// https://fhnw.mit-license.org/

#include "WebConfig.h"

AsyncWebServer aserver(80);
DNSServer dnsServer;

nodeConfig config;
Preferences preferences;

bool config_changed = false;

// redirect to captive portal if we got a request for another domain
class CaptiveRequestHandler : public AsyncWebHandler {
   public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}
    bool canHandle(AsyncWebServerRequest *request) {
        return true;
    }
    void handleRequest(AsyncWebServerRequest *request) {
        request->redirect("/");
    }
};

nodeConfig getNodeConfig() {
    return config;
}

WebConfig::WebConfig() {
    local_ip = IPAddress(8, 8, 4, 4);
    gateway = IPAddress(8, 8, 4, 4);
    subnet = IPAddress(255, 255, 255, 0);
}
WebConfig::~WebConfig() {}

// set WiFi mode to AP, start server and DNS
void WebConfig::startConfigServer() {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_AP);
    WiFi.setHostname(config.ssid.c_str());

    WiFi.softAP(config.cnf_ssid.c_str(), config.cnf_password.c_str());
    delay(2000);

    WiFi.softAPConfig(local_ip, gateway, subnet);

    dnsServer.start(DNS_PORT, "*", gateway);
    aserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->send_P(200, "text/html", html_content_full, processor); });
    aserver.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        clearConfiguration();
        loadConfiguration();
        request->redirect("/apply");
    });
    aserver.on("/advanced", HTTP_GET, [](AsyncWebServerRequest *request) { request->send_P(200, "text/html", adv_config, processor); });
    aserver.on("/apply", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", bye_msg, processor);
        storeConfiguration();
        delay(1000);
        ESP.restart();
    });
    aserver.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) {
        handlePostConfigData(request);
        storeConfiguration();
        loadConfiguration();

        request->redirect("/");
    });
    aserver.on("/setadvanced", HTTP_POST, [](AsyncWebServerRequest *request) {
        handlePostAdvConfigData(request);
        request->redirect("/apply");
    });

    aserver.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    aserver.begin();
    Serial.println("server started");
}

// validate if a string contains only hex characters
bool validateHex(String tovalidate) {
    for (int i = 0; i < tovalidate.length(); ++i) {
        if (!isHexadecimalDigit(tovalidate.charAt(i))) {
            return false;
        }
    }
    return true;
}

// handle POST data from advanced config page
void handlePostAdvConfigData(AsyncWebServerRequest *request) {
    if (request->hasParam("cnf_ssid", true)) {
        config.cnf_ssid = request->getParam("cnf_ssid", true)->value();
    }
    if (request->hasParam("cnf_password", true)) {
        config.cnf_password = request->getParam("cnf_password", true)->value();
    }
}

// handle POST data from config page
void handlePostConfigData(AsyncWebServerRequest *request) {
    config_changed = true;
    if (request->hasParam("connopt", true)) {
        String co = request->getParam("connopt", true)->value();
        Serial.println(co);
        if (co == "mqtt") {
            config.use_mqtt = true;
        } else {
            config.use_mqtt = false;
        }
    }
    if (request->hasParam("activation", true)) {
        String act = request->getParam("activation", true)->value();
        Serial.println(act);
        if (act == "abp") {
            config.use_otaa = false;
        } else {
            config.use_otaa = true;
        }
    }
    if (request->hasParam("device_address", true)) {
        String device_address = request->getParam("device_address", true)->value();

        if (device_address.length() != 8) {
            config.devAddr = 0;
        } else {
            if (validateHex(device_address)) {
                config.devAddr = strtol(device_address.c_str(), NULL, 16);
            } else {
                config.devAddr = 0;
            }
        }
    }
    if (request->hasParam("appskey", true)) {
        String appskey = request->getParam("appskey", true)->value();

        if (appskey.length() != 32) {
            config.appskey[0] = 0;
            Serial.print("invalid_appskey: ");
        } else {
            if (validateHex(appskey)) {
                appskey.toUpperCase();
                config.appskey_s = appskey;
                setAppSKeyFromString();
            } else {
                config.appskey[0] = 0;
                config.appskey_s = String();

                Serial.print("invalid_appskey: ");
            }
        }
        Serial.println(appskey);
    }
    if (request->hasParam("nwkskey", true)) {
        String nwkskey = request->getParam("nwkskey", true)->value();
        if (nwkskey.length() != 32) {
            config.nwkskey[0] = 0;
            Serial.print("invalid_nwkskey: ");
        } else {
            if (validateHex(nwkskey)) {
                Serial.print("valid_appskey: ");
                nwkskey.toUpperCase();
                config.nwkskey_s = nwkskey;
                setNwkSKeyFromString();
            } else {
                config.nwkskey[0] = 0;
                config.nwkskey_s = String();

                Serial.print("invalid_nwkskey: ");
            }
        }
        Serial.println(nwkskey);
    }
    // otaa
    if (request->hasParam("appeui", true)) {
        String appeui = request->getParam("appeui", true)->value();
        if (appeui.length() != 16) {
            config.appeui_valid = false;
            Serial.print("invalid_appeui: ");
        } else {
            if (validateHex(appeui)) {
                Serial.print("valid_appeui: ");
                config.appeui_valid = true;
                appeui.toUpperCase();
                config.appeui_s = appeui;
                setAppEUIFromString();
            } else {
                config.appeui_valid = false;
                config.appeui_s = String();

                Serial.print("invalid_nwkskey: ");
            }
        }
        Serial.println(appeui);
    }
    if (request->hasParam("deveui", true)) {
        String deveui = request->getParam("deveui", true)->value();
        if (deveui.length() != 16) {
            config.deveui[0] = 0;
            Serial.print("invalid_deveui: ");
        } else {
            if (validateHex(deveui)) {
                Serial.print("valid_deveui: ");
                deveui.toUpperCase();
                config.deveui_s = deveui;
                setDevEUIFromString();
            } else {
                config.deveui[0] = 0;
                config.deveui_s = String();

                Serial.print("invalid_deveui: ");
            }
        }
        Serial.println(deveui);
    }
    if (request->hasParam("appkey", true)) {
        String appkey = request->getParam("appkey", true)->value();
        if (appkey.length() != 32) {
            config.appkey[0] = 0;
            Serial.print("invalid_appkey: ");
        } else {
            if (validateHex(appkey)) {
                Serial.print("valid_appkey: ");
                appkey.toUpperCase();
                config.appkey_s = appkey;
                setAppKeyFromString();
            } else {
                config.appkey[0] = 0;
                config.appkey_s = String();

                Serial.print("invalid_appkey: ");
            }
        }
        Serial.println(appkey);
    }

    if (!config.use_mqtt) {
        if (request->hasParam("use_adr", true)) {
            if (request->getParam("use_adr", true)->value() == "on") {
                config.use_adr = true;
            }
        } else {
            config.use_adr = false;
        }
    }

    if (request->hasParam("lorasf", true)) {
        config.loraSf = (uint8_t)request->getParam("lorasf", true)->value().toInt();
    }

    if (request->hasParam("ssid", true)) {
        config.ssid = request->getParam("ssid", true)->value();
    }
    if (request->hasParam("wifi_password", true)) {
        config.wifi_password = request->getParam("wifi_password", true)->value();
    }
    if (request->hasParam("mqtt_broker", true)) {
        config.mqtt_broker_url = request->getParam("mqtt_broker", true)->value();
        Serial.print("new Broker URL:");
        Serial.println(config.mqtt_broker_url);
    }
    if (request->hasParam("mqtt_port", true)) {
        config.mqtt_port = request->getParam("mqtt_port", true)->value().toInt();
    }
    if (config.use_mqtt) {
        if (request->hasParam("mqtt_use_tls", true)) {
            if (request->getParam("mqtt_use_tls", true)->value() == "on") {
                config.use_tls = true;
            }
        } else {
            config.use_tls = false;
        }
    }
    if (request->hasParam("mqtt_username", true)) {
        config.mqtt_username = request->getParam("mqtt_username", true)->value();
    }
    if (request->hasParam("mqtt_password", true)) {
        config.mqtt_password = request->getParam("mqtt_password", true)->value();
    }
    if (request->hasParam("node_id", true)) {
        config.node_id = request->getParam("node_id", true)->value();
    }
    if (request->hasParam("lat", true)) {
        config.latitude = request->getParam("lat", true)->value();
    }
    if (request->hasParam("lon", true)) {
        config.longitude = request->getParam("lon", true)->value();
    }
    if (request->hasParam("mqtt_topic", true)) {
        config.mqtt_topic = request->getParam("mqtt_topic", true)->value();
    }

    if (request->hasParam("txinterval", true)) {
        config.txInterval_s = request->getParam("txinterval", true)->value().toInt();
    }
    if (request->hasParam("scaninterval", true)) {
        config.scan_interval_s = request->getParam("scaninterval", true)->value().toInt();
    }
    if (request->hasParam("scandur", true)) {
        config.scan_duration_ms = request->getParam("scandur", true)->value().toInt();
    }
    if (request->hasParam("filtermac", true)) {
        if (request->getParam("filtermac", true)->value() == "on") {
            config.apply_mac_filter = true;
            Serial.println("Filter ON");
        }
    } else {
        config.apply_mac_filter = false;
        Serial.println("Filter OFF");
    }
}

// clear all config values
void clearConfiguration() {
    preferences.begin(preferences_namespace, false);
    preferences.clear();
    preferences.end();
}

// store config values in EEPROM
void storeConfiguration() {
    preferences.begin(preferences_namespace, false);

    preferences.putBool("use_mqtt", config.use_mqtt);
    preferences.putBool("use_otaa", config.use_otaa);

    preferences.putUInt("devAddr", config.devAddr);
    preferences.putString("appskey_s", config.appskey_s);
    preferences.putString("nwkskey_s", config.nwkskey_s);

    preferences.putString("deveui_s", config.deveui_s);
    preferences.putString("appeui_s", config.appeui_s);
    preferences.putString("appkey_s", config.appkey_s);
    preferences.putBool("appeui_valid", config.appeui_valid);

    preferences.putUInt("loraSf", config.loraSf);
    preferences.putBool("use_adr", config.use_adr);

    preferences.putString("ssid", config.ssid);
    preferences.putString("wifi_password", config.wifi_password);

    preferences.putString("mqtt_broker_url", config.mqtt_broker_url);
    preferences.putUInt("mqtt_port", config.mqtt_port);
    preferences.putBool("use_tls", config.use_tls);
    preferences.putString("mqtt_username", config.mqtt_username);
    preferences.putString("mqtt_password", config.mqtt_password);
    preferences.putString("mqtt_topic", config.mqtt_topic);
    preferences.putString("node_id", config.node_id);
    preferences.putString("latitude", config.latitude);
    preferences.putString("longitude", config.longitude);

    preferences.putUInt("txInterval_s", config.txInterval_s);
    preferences.putUInt("scan_interval", config.scan_interval_s);
    preferences.putUInt("scan_dur", config.scan_duration_ms);
    preferences.putBool("mac_filter", config.apply_mac_filter);

    preferences.putString("cnf_ssid", config.cnf_ssid);
    preferences.putString("cnf_password", config.cnf_password);

    preferences.end();
}

// load config values from EEPROM
bool loadConfiguration() {
    preferences.begin(preferences_namespace, true);

    config.use_mqtt = preferences.getBool("use_mqtt", false);
    config.use_otaa = preferences.getBool("use_otaa", false);
    config.devAddr = preferences.getUInt("devAddr", 0);
    config.appskey_s = preferences.getString("appskey_s", String());
    config.nwkskey_s = preferences.getString("nwkskey_s", String());
    config.deveui_s = preferences.getString("deveui_s", String());
    config.appeui_s = preferences.getString("appeui_s", String());
    config.appkey_s = preferences.getString("appkey_s", String());
    config.appeui_valid = preferences.getBool("appeui_valid", false);

    config.loraSf = preferences.getUInt("loraSf", default_lora_sf);
    config.use_adr = preferences.getBool("use_adr", true);

    config.ssid = preferences.getString("ssid");
    config.wifi_password = preferences.getString("wifi_password");

    config.mqtt_broker_url = preferences.getString("mqtt_broker_url", default_mqtt_broker_url);
    config.mqtt_port = preferences.getUInt("mqtt_port", default_mqtt_port);
    config.use_tls = preferences.getBool("use_tls", true);
    config.mqtt_username = preferences.getString("mqtt_username");
    config.mqtt_password = preferences.getString("mqtt_password");
    config.mqtt_topic = preferences.getString("mqtt_topic", default_mqtt_topic);
    config.node_id = preferences.getString("node_id", default_node_id);
    config.latitude = preferences.getString("latitude", default_lat_lon);
    config.longitude = preferences.getString("longitude", default_lat_lon);

    config.txInterval_s = preferences.getUInt("txInterval_s", default_txInterval_s);
    config.scan_interval_s = preferences.getUInt("scan_interval", default_scan_interval_s);
    config.scan_duration_ms = preferences.getUInt("scan_dur", default_scan_duration_ms);
    config.apply_mac_filter = preferences.getBool("mac_filter", true);

    config.cnf_ssid = preferences.getString("cnf_ssid", default_cnf_ssid);
    config.cnf_password = preferences.getString("cnf_password", default_cnf_pw);
    preferences.end();
    if (config.appskey_s.length() == 32)
        setAppSKeyFromString();
    if (config.nwkskey_s.length() == 32)
        setNwkSKeyFromString();
    if (config.appeui_s.length() == 16)
        setAppEUIFromString();
    if (config.deveui_s.length() == 16)
        setDevEUIFromString();
    if (config.appkey_s.length() == 32)
        setAppKeyFromString();
    return true;
}

// print config values to serial
void WebConfig::printNodeConf() {
    Serial.println("###################CONFIG#################");
    Serial.print("Connectivity: ");
    if (config.use_mqtt) {
        Serial.println("MQTT");
        Serial.printf("\tSSID = %s\n", config.ssid);
        Serial.printf("\tWiFiPW = %s\n", config.wifi_password);
        Serial.print("\tBroker = ");
        Serial.println(config.mqtt_broker_url);
        Serial.printf("\tPort = %d\n", config.mqtt_port);
        Serial.printf("\tTLS = %d\n", config.use_tls);
        Serial.printf("\tMQTTUsername = %s\n", config.mqtt_username);
        Serial.printf("\tMQTTPassword = %s\n", config.mqtt_password);
        Serial.printf("\tMQTTTopic = %s\n", config.mqtt_topic);
        Serial.print("\tNode_ID = ");
        Serial.println(config.node_id);
        Serial.print("\tLatitude = ");
        Serial.println(config.latitude);
        Serial.print("\tLongitude = ");
        Serial.println(config.longitude);
    } else {
        Serial.println("LoRa");
        Serial.printf("\tADR = %d\n", config.use_adr);
        Serial.printf("\tSF = %d\n", config.loraSf);
        if (config.use_otaa) {
            Serial.println("\tOTAA");
            Serial.print("\tAppEUI = ");
            for (int i = 0; i < 8; ++i) {
                Serial.printf("0x%02X ", config.appeui[i]);
            }
            Serial.print("\n\tDevEUI = ");

            for (int i = 0; i < 8; ++i) {
                Serial.printf("0x%02X ", config.deveui[i]);
            }
            Serial.print("\n\tAppKey = ");

            for (int i = 0; i < 16; ++i) {
                Serial.printf("0x%02X ", config.appkey[i]);
            }
            Serial.println();
        } else {
            Serial.println("\tABP");

            Serial.printf("\tDeviceAddress: % 04X\n", config.devAddr);
            Serial.print("\tAppSkey = ");
            for (int i = 0; i < 16; ++i) {
                Serial.printf("0x%02X ", config.appskey[i]);
            }
            Serial.print("\n\tNwsSkey = ");

            for (int i = 0; i < 16; ++i) {
                Serial.printf("0x%02X ", config.nwkskey[i]);
            }
            Serial.println();
        }
    }
    Serial.println("Measurement");
    Serial.printf("\tTXInterval = %d s\n", config.txInterval_s);
    Serial.printf("\tScanInterval = %d s\n", config.scan_interval_s);
    Serial.printf("\tScanDuration = %d ms\n", config.scan_duration_ms);
    Serial.printf("\tApplyFilter = %d\n", config.apply_mac_filter);

    Serial.println("Device");
    Serial.print("\tSSID = ");
    Serial.println(config.cnf_ssid);
    Serial.print("\tPW = ");
    Serial.println(config.cnf_password);

    Serial.println("####################END###################");
}

// process incoming requests
void WebConfig::processDNSRequests() {
    while (1) {
        dnsServer.processNextRequest();
        if (config_changed) {
            printNodeConf();
            config_changed = false;
        }
    }
}

void setAppSKeyFromString() {
    for (int i = 0; i < 16; ++i) {
        config.appskey[i] = strtol(config.appskey_s.substring(2 * i, 2 * i + 2).c_str(), NULL, 16);
    }
}
void setNwkSKeyFromString() {
    for (int i = 0; i < 16; ++i) {
        config.nwkskey[i] = strtol(config.nwkskey_s.substring(2 * i, 2 * i + 2).c_str(), NULL, 16);
    }
}

void setAppKeyFromString() {
    for (int i = 0; i < 16; ++i) {
        config.appkey[i] = strtol(config.appkey_s.substring(2 * i, 2 * i + 2).c_str(), NULL, 16);
    }
}

// MSB2LSB!
void setDevEUIFromString() {
    for (int i = 0; i < 8; ++i) {
        config.deveui[7 - i] = strtol(config.deveui_s.substring(2 * i, 2 * i + 2).c_str(), NULL, 16);
    }
}

// MSB2LSB!
void setAppEUIFromString() {
    for (int i = 0; i < 8; ++i) {
        config.appeui[7 - i] = strtol(config.appeui_s.substring(2 * i, 2 * i + 2).c_str(), NULL, 16);
    }
}

// replace placeholder with actual value
String processor(const String &var) {
    if (var == "LORA_SELECTED") {
        if (config.use_mqtt)
            return String();
        else
            return selected;
    }
    if (var == "MQTT_SELECTED") {
        if (config.use_mqtt)
            return selected;
        else
            return String();
    }

    if (var == "ABP_SELECTED") {
        if (!config.use_otaa)
            return selected;
        else
            return String();
    }
    if (var == "OTAA_SELECTED") {
        if (config.use_otaa)
            return selected;
        else
            return String();
    }

    if (var == "DEVICE_ADDRESS") {
        if (config.devAddr == 0x00)
            return String();
        else {
            String ucHexDevAddr = String(config.devAddr, HEX);
            ucHexDevAddr.toUpperCase();
            return ucHexDevAddr;
        }
    }
    if (var == "APPSKEY") {
        if (config.appskey[0] == 0x00)
            return String();
        else
            return config.appskey_s;
    }
    if (var == "NWKSKEY") {
        if (config.nwkskey[0] == 0x00)
            return String();
        else
            return config.nwkskey_s;
    }
    // otaa
    if (var == "APPEUI") {
        if (config.appeui_valid)
            return config.appeui_s;

        else
            return String();
    }
    if (var == "DEVEUI") {
        if (config.deveui[0] == 0x00)
            return String();
        else
            return config.deveui_s;
    }
    if (var == "APPKEY") {
        if (config.appkey[0] == 0x00)
            return String();
        else
            return config.appkey_s;
    }

    if (var == "USE_ADR") {
        if (config.use_adr)
            return checked;
        else
            return String();
    }
    if (var == "SF7") {
        if (config.loraSf == 7)
            return selected;
        else
            return String();
    }
    if (var == "SF8") {
        if (config.loraSf == 8)
            return selected;
        else
            return String();
    }
    if (var == "SF9") {
        if (config.loraSf == 9)
            return selected;
        else
            return String();
    }
    if (var == "SF10") {
        if (config.loraSf == 10)
            return selected;
        else
            return String();
    }
    if (var == "SF11") {
        if (config.loraSf == 11)
            return selected;
        else
            return String();
    }
    if (var == "SF12") {
        if (config.loraSf == 12)
            return selected;
        else
            return String();
    }
    if (var == "SSID") {
        if (config.ssid)
            return config.ssid;
        else
            return String();
    }
    if (var == "WIFI_PASSWORD") {
        if (config.wifi_password)
            return config.wifi_password;
        else
            return String();
    }
    if (var == "BROKER") {
        if (config.mqtt_broker_url)
            return config.mqtt_broker_url;
        else
            return String();
    }
    if (var == "PORT") {
        return String(config.mqtt_port);
    }
    if (var == "USE_TLS_CHECKED") {
        if (config.use_tls)
            return checked;
        else
            return String();
    }
    if (var == "MQTT_USERNAME") {
        if (config.mqtt_username)
            return config.mqtt_username;
        else
            return String();
    }
    if (var == "MQTT_PASSWORD") {
        if (config.mqtt_password)
            return config.mqtt_password;
        else
            return String();
    }
    if (var == "NODE_ID") {
        if (config.node_id)
            return config.node_id;
        else
            return String();
    }
    if (var == "LAT") {
        return config.latitude;
    }
    if (var == "LON") {
        return config.longitude;
    }
    if (var == "MQTT_TOPIC") {
        if (config.mqtt_topic)
            return config.mqtt_topic;
        else
            return String();
    }
    if (var == "TX_INTERVAL") {
        return String(config.txInterval_s);
    }
    if (var == "SCAN_INTERVAL") {
        return String(config.scan_interval_s);
    }
    if (var == "SCAN_DUR") {
        return String(config.scan_duration_ms);
    }
    if (var == "FILTERMAC_CHECKED") {
        if (config.apply_mac_filter)
            return checked;
        else
            return String();
    }
    if (var == "CNF_SSID") {
        if (config.cnf_ssid)
            return config.cnf_ssid;
        else
            return String();
    }
    if (var == "CNF_PASSWORD") {
        if (config.cnf_password)
            return config.cnf_password;
        else
            return String();
    }

    return String();
}
