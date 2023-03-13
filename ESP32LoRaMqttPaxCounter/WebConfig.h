// https://fhnw.mit-license.org/


#include <Arduino.h>
#include <AsyncTCP.h>			// https://github.com/me-no-dev/AsyncTCP
#include <DNSServer.h>			// https://github.com/espressif/arduino-esp32/tree/master/libraries/DNSServer
#include <ESPAsyncWebServer.h> 	// https://github.com/me-no-dev/ESPAsyncWebServer
#include <Preferences.h>		// https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences
#include <WiFi.h>				// https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi

const byte DNS_PORT = 53;

const char html_content_full[] PROGMEM = "<!DOCTYPE html><html><head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"> <title>&#9881;PAX Configuration</title> <style>html{font-family: Helvetica; display: inline-block; margin: 0px auto;}.grid-container{display: grid; grid-template-columns: 25%% 25%% 25%% 25%%; padding: 10px; margin-top: 10px; margin-bottom: 20px;}.gitm1{grid-column: 1 / span 2; text-align: left; font-size: auto; text-align: center;}.btn-gitm{grid-column: 3 / span 2; grid-row: 1; display: grid; grid-template-columns: 33%% 33%% 33%%; padding-top: 20px; padding-bottom: 10px;}.gitm3{grid-column: 1 / span 2; grid-row: 2; text-align: left; padding-right: 10px;}.gitm4{grid-column: 3 / span 2; grid-row: 2; text-align: left; padding-right: 10px; padding-left: 10px;}.grid-ct-conn{display: grid; grid-template-columns: 30%% 70%%; text-align: left; padding-left: 5px;}.gitmc1{grid-column: 1 / span 2;}select{width: 100%%; padding: 10px 10px; display: inline-block; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box;}.input{width: 100%%; padding: 10px 10px; display: inline-block; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box;}.updatebtn{width: 90%%; border: none; color: white; border-radius: 4px; box-sizing: content-box; background-color: #2E76CF;}.advbtn{width: 90%%; background-color: #808080; border: none; color: white; display: inline-block; border-radius: 4px; box-sizing: content-box;}.updatebtn:hover{background-color: #173b67;}.advbtn:hover{background-color: #4e4e4e;}label{width: 100%%; padding: 10px 10px; display: inline-block; text-align: right; box-sizing: border-box;}</style></head><body onload=\"toggleConnop();\"> <div class=\"grid-container\"> <div class=\"gitm1\"> <h1>PAX Counter Configuration</h1> </div><div class=\"gitm3\"> <div class=\"grid-ct-conn\"> <div class=\"gitmc1\"> <h2>Connectivity</h2> </div><div class=\"gitmc\"> <label for=\"connopt\"> Choose a connectivity option : </label> </div><div class=\"gitmc\"> <select id=\"connopt\" name=\"connopt\" form=\"updateconfig\" onchange=\"toggleConnop();\"> <option value=\"lora\" %LORA_SELECTED%>LoRa</option> <option value=\"mqtt\" %MQTT_SELECTED%>MQTT over WiFi</option> </select> </div><div class=\"gitmc1\"> <h4>LoRa Credentials</h4> </div><div class=\"gitmc\"> <form action=\"/post\" method=\"post\" id=\"updateconfig\"> <label for=\"activation\"> Activation method : </label> </div><div class=\"gitmc\"> <select id=\"activation\" name=\"activation\" onchange=\"toggleLoRaAct();\"> <option value=\"abp\" %ABP_SELECTED%>ABP</option> <option value=\"otaa\" %OTAA_SELECTED%>OTAA</option> </select><br><br></div><div class=\"gitmc\"> <label for=\"use_adr\">Use ADR : </label> </div><div class=\"gitmc\"> <input class=\"input-cb\" type=\"checkbox\" id=\"use_adr\" name=\"use_adr\" %USE_ADR%><br><br></div><div class=\"gitmc\"> <label for=\"use_adr\">Spreading Factor : </label> </div><div class=\"gitmc\"> <select class=\"form-select\" id=\"lorasf\" name=\"lorasf\"> <option value=\"7\" %SF7%>SF 7</option> <option value=\"8\" %SF8%>SF 8</option> <option value=\"9\" %SF9%>SF 9</option> <option value=\"10\" %SF10%>SF 10</option> <option value=\"11\" %SF11%>SF 11</option> <option value=\"12\" %SF12%>SF 12</option> </select> </div><div class=\"gitmc1\"> <h4>ABP</h4> </div><div class=\"gitmc\"> <label for=\"device_address\"> Device Address : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"device_address\" name=\"device_address\" placeholder=\"012345678\" required=\"required\" value=\"%DEVICE_ADDRESS%\"><br><br></div><div class=\"gitmc\"> <label for=\"nwkskey\"> NwkSKey : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"nwkskey\" name=\"nwkskey\" placeholder=\"0123456789ABCDEF0123456789ABCDEF\" required=\"required\" value=\"%NWKSKEY%\"><br><br></div><div class=\"gitmc\"> <label for=\"appskey\"> AppSKey : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"appskey\" name=\"appskey\" placeholder=\"0123456789ABCDEF0123456789ABCDEF\" required=\"required\" value=\"%APPSKEY%\"><br><br></div><div class=\"gitmc1\"> <h4>OTAA</h4> </div><div class=\"gitmc\"> <label for=\"appeui\"> AppEUI : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"appeui\" name=\"appeui\" placeholder=\"0123456789ABCDEF\" required=\"required\" value=\"%APPEUI%\"><br><br></div><div class=\"gitmc\"> <label for=\"deveui\"> DevEUI : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"deveui\" name=\"deveui\" placeholder=\"0123456789ABCDEF\" required=\"required\" value=\"%DEVEUI%\"><br><br></div><div class=\"gitmc\"> <label for=\"appkey\"> AppKey : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"appkey\" name=\"appkey\" placeholder=\"0123456789ABCDEF0123456789ABCDEF\" required=\"required\" value=\"%APPKEY%\"><br><br></div><div class=\"gitmc1\"> <h4>WiFi Credentials</h4> </div><div class=\"gitmc\"> <label for=\"ssid\"> SSID : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"ssid\" name=\"ssid\" required=\"required\" value=\"%SSID%\"><br><br></div><div class=\"gitmc\"> <label for=\"wifi_password\"> WiFi Password : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"wifi_password\" name=\"wifi_password\" required=\"required\" value=\"%WIFI_PASSWORD%\"><br><br></div><div class=\"gitmc1\"> <h4>MQTT Credentials</h4> </div><div class=\"gitmc\"> <label for=\"mqtt_broker\"> MQTT Broker URL : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"mqtt_broker\" name=\"mqtt_broker\" required=\"required\" value=\"%BROKER%\"><br><br></div><div class=\"gitmc\"> <label for=\"mqtt_port\"> MQTT Port : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"number\" id=\"mqtt_port\" name=\"mqtt_port\" required=\"required\" value=\"%PORT%\"><br><br></div><div class=\"gitmc\"> <label for=\"mqtt_use_tls\">Use TLS : </label> </div><div class=\"gitmc\"> <input class=\"input-cb\" type=\"checkbox\" id=\"mqtt_use_tls\" name=\"mqtt_use_tls\" %USE_TLS_CHECKED%><br><br></div><div class=\"gitmc\"> <label for=\"mqtt_username\"> MQTT Username : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"mqtt_username\" name=\"mqtt_username\" value=\"%MQTT_USERNAME%\"><br><br></div><div class=\"gitmc\"> <label for=\"mqtt_password\"> MQTT Password : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"mqtt_password\" name=\"mqtt_password\" value=\"%MQTT_PASSWORD%\"><br><br></div><div class=\"gitmc\"> <label for=\"node_id\"> Node ID : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"node_id\" name=\"node_id\" required=\"required\" value=\"%NODE_ID%\"><br><br></div><div class=\"gitmc\"> <label for=\"lat\"> Latitude : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"number\" id=\"lat\" name=\"lat\" step=\"any\" value=\"%LAT%\"><br><br></div><div class=\"gitmc\"> <label for=\"lon\"> Longitude : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"number\" id=\"lon\" name=\"lon\" step=\"any\" value=\"%LON%\"><br><br></div><div class=\"gitmc\"> <label for=\"mqtt_topic\"> MQTT Topic : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"text\" id=\"mqtt_topic\" name=\"mqtt_topic\" required=\"required\" value=\"%MQTT_TOPIC%\"><br><br></div></form> </div></div><div class=\"gitm4\"> <div class=\"grid-ct-conn\"> <div class=\"gitmc1\"> <h2>Measurement</h2> </div><div class=\"gitmc\"> <label for=\"txinterval\">PAX Transmit Interval [s] : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"number\" id=\"txinterval\" name=\"txinterval\" form=\"updateconfig\" required=\"required\" value=\"%TX_INTERVAL%\"><br><br></div><div class=\"gitmc\"> <label for=\"scaninterval\"> BLE Scan Interval [s] : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"number\" id=\"scaninterval\" name=\"scaninterval\" form=\"updateconfig\" required=\"required\" value=\"%SCAN_INTERVAL%\"><br><br></div><div class=\"gitmc\"> <label for=\"scandur\"> BLE Scan Duration [ms] : </label> </div><div class=\"gitmc\"> <input class=\"input\" type=\"number\" id=\"scandur\" name=\"scandur\" form=\"updateconfig\" required=\"required\" value=\"%SCAN_DUR%\"><br><br></div><div class=\"gitmc\"> <label for=\"filtermac\"> Filter for Smartphones : </label> </div><div class=\"gitmc\"> <input class=\"input-cb\" type=\"checkbox\" id=\"filtermac\" name=\"filtermac\" form=\"updateconfig\" %FILTERMAC_CHECKED%><br><br></div></div></div><div class=\"btn-gitm\"> <input type=\"button\" class=\"advbtn\" value=\"Advanced Settings\" onclick=\"if(confirm('Unsaved parameters will be lost. Continue?'))location.href='/advanced';\"/> <input type=\"button\" class=\"updatebtn\" value=\"Reboot\" onclick=\"if(confirm('Changes will be applied after the reboot.'))location.href='/apply';\"/> <button class=\"updatebtn\" form=\"updateconfig\">Save Configuration</button> </div></div><script>function toggleLoRaAct(){let connopt=document.getElementById(\"connopt\").selectedIndex; let actopt=document.getElementById(\"activation\").selectedIndex; if (connopt==0){if (actopt==0){document.getElementById('device_address').disabled=false; document.getElementById('appskey').disabled=false; document.getElementById('nwkskey').disabled=false; document.getElementById('appeui').disabled=true; document.getElementById('deveui').disabled=true; document.getElementById('appkey').disabled=true;}else{document.getElementById('device_address').disabled=true; document.getElementById('appskey').disabled=true; document.getElementById('nwkskey').disabled=true; document.getElementById('appeui').disabled=false; document.getElementById('deveui').disabled=false; document.getElementById('appkey').disabled=false;}}else{console.log(\"WiFi\");}}function toggleConnop(){let connopt=document.getElementById(\"connopt\").selectedIndex; if (connopt==0){document.getElementById('device_address').disabled=false; document.getElementById('appskey').disabled=false; document.getElementById('nwkskey').disabled=false; document.getElementById('use_adr').disabled=false; document.getElementById('lorasf').disabled=false; document.getElementById('appeui').disabled=false; document.getElementById('deveui').disabled=false; document.getElementById('appkey').disabled=false; document.getElementById('activation').disabled=false; document.getElementById('ssid').disabled=true; document.getElementById('wifi_password').disabled=true; document.getElementById('mqtt_broker').disabled=true; document.getElementById('mqtt_port').disabled=true; document.getElementById('mqtt_use_tls').disabled=true; document.getElementById('mqtt_username').disabled=true; document.getElementById('mqtt_password').disabled=true; document.getElementById('mqtt_topic').disabled=true; document.getElementById('node_id').disabled=true; document.getElementById('lat').disabled=true; document.getElementById('lon').disabled=true;}else if (connopt==1){document.getElementById('device_address').disabled=true; document.getElementById('appskey').disabled=true; document.getElementById('nwkskey').disabled=true; document.getElementById('use_adr').disabled=true; document.getElementById('lorasf').disabled=true; document.getElementById('appeui').disabled=true; document.getElementById('deveui').disabled=true; document.getElementById('appkey').disabled=true; document.getElementById('activation').disabled=true; document.getElementById('ssid').disabled=false; document.getElementById('wifi_password').disabled=false; document.getElementById('ssid').disabled=false; document.getElementById('wifi_password').disabled=false; document.getElementById('mqtt_broker').disabled=false; document.getElementById('mqtt_port').disabled=false; document.getElementById('mqtt_use_tls').disabled=false; document.getElementById('mqtt_username').disabled=false; document.getElementById('mqtt_password').disabled=false; document.getElementById('mqtt_topic').disabled=false; document.getElementById('node_id').disabled=false; document.getElementById('lat').disabled=false; document.getElementById('lon').disabled=false;}toggleLoRaAct();}</script></body></html>";

const char adv_config[] PROGMEM = "<!DOCTYPE html><html><head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"> <title>&#9881;PAX Configuration</title> <style>html{font-family: Helvetica; display: inline-block; margin: 0px auto;}.grid-container{display: grid; grid-template-columns: 25%% 25%% 25%% 25%%; padding: 10px; margin-top: 10px; margin-bottom: 20px;}.grtm{padding: 20px; font-size: 30px; text-align: center;}.btn-gitm{grid-column: 3 / span 2; grid-row: 1; display: grid; grid-template-columns: 33%% 33%% 33%%; padding-top: 20px; padding-bottom: 10px;}.grtm1{grid-column: 1 / span 2; text-align: left; font-size: auto; text-align: center;}.grtm2{grid-column: 3; grid-row: 1; text-align: left; padding: 20px; font-size: auto; text-align: center;}.grtm3{grid-column: 1 / span 2; grid-row: 2; text-align: left; padding-right: 10px;}.grid-ct-conn{display: grid; grid-template-columns: 30%% 70%%; text-align: left; padding-left: 5px;}.grtmc1{grid-column: 1 / span 2;}.input{width: 100%%; padding: 10px 10px; display: inline-block; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box;}.updatebtn{width: 90%%; border: none; color: white; border-radius: 4px; box-sizing: content-box; background-color: #2E76CF;}.advbtn{width: 90%%; border: none; color: white; border-radius: 4px; box-sizing: content-box; background-color: #808080;}.resetbtn{width: 90%%; border: none; color: white; border-radius: 4px; box-sizing: content-box; background-color: #c00000;}.updatebtn:hover{background-color: #173b67;}.advbtn:hover{background-color: #4e4e4e;}.resetbtn:hover{background-color: #ff0000;}label{width: 100%%; padding: 10px 10px; display: inline-block; text-align: right; box-sizing: border-box;}</style></head><body> <div class=\"grid-container\"> <div class=\"grtm1\"> <h1>Advanced Configuration</h1> </div><div class=\"grtm3\"> <div class=\"grid-ct-conn\"> <div class=\"grtmc1\"> <h2>Configuration Portal WiFi Credentials</h2> <p>Make sure you have written down your credentials!</p></div><div class=\"grtmc\"> <form action=\"/setadvanced\" method=\"post\" id=\"updateadv\"> <label for=\"cnf_ssid\"> Config AP SSID : </label> </div><div class=\"grtmc\"> <input class=\"input\" type=\"text\" id=\"cnf_ssid\" name=\"cnf_ssid\" placeholder=\"0123456789ABCDEF0123456789ABCDEF\" required=\"required\" value=\"%CNF_SSID%\"><br><br></div><div class=\"grtmc\"> <label for=\"cnf_password\"> Config AP Password : </label> </div><div class=\"grtmc\"> <input class=\"input\" type=\"text\" id=\"cnf_password\" name=\"cnf_password\" placeholder=\"0123456789ABCDEF0123456789ABCDEF\" required=\"required\" value=\"%CNF_PASSWORD%\"><br><br></div></form> </div></div><div class=\"btn-gitm\"> <input type=\"button\" class=\"resetbtn\" value=\"Reset Device\" onclick=\"if(confirm('Changes will reset.'))location.href='/reset';\"/> <input type=\"button\" class=\"advbtn\" value=\"Return\" onclick=\"if(confirm('Unsaved parameters will be lost. Continue?'))location.href='/';\"/> <button class=\"updatebtn\" form=\"updateadv\" onclick=\"return confirm('This will change the password for the configuration portal');\">Save Configuration and restart</button> </div></div></body></html>";

const char bye_msg[] PROGMEM = "<h1>Bye! &#128075;</h1><h2>Configuration Portal Credentials are: </h2><h3>SSID=%CNF_SSID%</h3><h3>Password=%CNF_PASSWORD%</h3>";

const char selected[] PROGMEM = "selected";
const char checked[] PROGMEM = "checked";
const char configured[] PROGMEM = "configured";
const char preferences_namespace[] PROGMEM = "paxcounter";
const char default_cnf_ssid[] PROGMEM = "PaxConfig";
const char default_cnf_pw[] PROGMEM = "password";
const char default_mqtt_broker_url[] PROGMEM = "broker.hivemq.com";
const char default_mqtt_topic[] PROGMEM = "data/pax";
const char default_lat_lon[] PROGMEM = "null";
const char default_node_id[] PROGMEM = "pax-01";
const uint32_t default_mqtt_port = 1883;
const uint8_t default_lora_sf = 8;
const uint32_t default_txInterval_s = 900;
const uint32_t default_scan_interval_s = 30;
const uint32_t default_scan_duration_ms = 3000;

struct nodeConfig {
    // Connectivity
    bool use_mqtt = false;
    // LoRa Parameters
    bool use_otaa = false; // true = OTA, false = ABP
    uint32_t devAddr;
    uint8_t appskey[16];
    String appskey_s;
    uint8_t nwkskey[16];
    String nwkskey_s;
    uint8_t deveui[8];
    String deveui_s;
    uint8_t appeui[8];
    String appeui_s;
    bool appeui_valid;
    uint8_t appkey[16];
    String appkey_s;

    uint8_t loraSf;
    bool use_adr = true;

    // WiFi Parameters
    String ssid;
    String wifi_password;

    // MQTT Parameters
    String mqtt_broker_url;
    int mqtt_port;
    bool use_tls = true;
    String mqtt_username;
    String mqtt_password;
    String node_id;
    String mqtt_topic;

    // Measurement
    uint32_t txInterval_s;
    uint32_t scan_interval_s;
    uint32_t scan_duration_ms;
    bool apply_mac_filter;

    // Device
    String cnf_ssid;
    String cnf_password;
    String latitude;
    String longitude;
};

nodeConfig getNodeConfig();

void storeConfiguration();
bool loadConfiguration();
void clearConfiguration();
void setAppSKeyFromString();
void setNwkSKeyFromString();
void setAppKeyFromString();
void setDevEUIFromString();
void setAppEUIFromString();
bool validateHex(String tovalidate);

String processor(const String &var);
void handlePostConfigData(AsyncWebServerRequest *request);
void handlePostAdvConfigData(AsyncWebServerRequest *request);

class WebConfig {
   public:
    WebConfig();
    ~WebConfig();
    void startConfigServer();
    void processDNSRequests();
    void printNodeConf();

   private:
    IPAddress local_ip;
    IPAddress gateway;
    IPAddress subnet;
};
