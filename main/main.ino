#include "WiFi.h"
#include "HTTPClient.h"
#include "esp_wps.h"
// #include "ArduinoJson.h"
#include <Arduino.h>
#include <Ticker.h>
#include <map>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>


#define LED_PIN 2
#define RESET_PIN 4
 
 
const String ERROR_RESP = "ERROR";
const float runPeriod = 5; //seconds
const int scanTime = 1; //In seconds
int iteration = 0;
bool timeout = false;

BLEScan* pBLEScan;
Ticker mainTask;

const char* ssid     = "UCSD-DEVICE"; 
const char* password = "Fj7UPsFHb84e";
//const char* ssid     = "RESNET-GUEST-DEVICE"; 
//const char* password = "ResnetConnect";

const String API_BASE_URL = "http://cse191.ucsd.edu/api"; // root url of api
const String API_HEALTH = "http://cse191.ucsd.edu/api/health";
const String API_REGISTER_DEVICE = "http://cse191.ucsd.edu/api/register-device";
const String API_LOG_DEVICE = "http://cse191.ucsd.edu/api/log-devices";

// create MAC storage, keys are mac addresses, values are rssi values
std::map<String, String> bleDevices;

/*****************
 *   Scanning 
 *****************/
void parseBeacon(BLEAdvertisedDevice dev) {
  Serial.print(".");
  
  // get the values
  String mac = String(dev.getAddress().toString().c_str());
  String rssi = String(dev.getRSSI());

  // Add entry to map if doesn't already exist
  if(!bleDevices.count(mac)) {
    bleDevices[mac] = rssi;
  }
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  // This function is called for every device that BLEScan scans!
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    parseBeacon(advertisedDevice);
  }
};


/***************************
 *   Set Up Helper Functions 
 ***************************/
void rebootBoard() {
  Serial.println("");
  Serial.println("Resetting (Rebooting) Board...");
  delay(500);
  //digitalWrite(RESET_PIN, LOW);
  ESP.restart();
}

String postJsonHTTP(String url, String jLoad) {

  HTTPClient httpC;
  String resp="";   

  // debug info
  Serial.print("POST: ");
  Serial.println(url);
  Serial.print("JSON: ");
  Serial.println(jLoad);

  
  httpC.begin(url);                        //Specify destination for HTTP request
  httpC.addHeader("Content-Type", "application/json");

  int httpCode = httpC.POST(jLoad);              //Send the actual POST request

  if(httpCode == 200){

    resp = httpC.getString();    //Get the response to the request
    
    Serial.print(F("POST Code: "));
    Serial.println(httpCode);   //Print return code
    Serial.println(resp);       //Print request answer

  }
  else {
    resp = ERROR_RESP;
    Serial.print("Error on sending POST: ");
    Serial.println(httpCode);

  }

  httpC.end();  //Free resources

  return resp;
}

// StaticJsonDocument<2000>  getGeoLocation() {

//   StaticJsonDocument<2000> jDoc;
  
//   // get geo location info
//   String locStr = "https://api.iplocation.net/?ip=99.10.120.238";
  
//   // debug info
//   Serial.println("Getting Location: "+locStr);
//   String loc = postJsonHTTP(locStr, "");

//   // debug
//   Serial.println(loc);

//   // parse json (need zipcode and timezone) -> jDoc
//   DeserializationError error = deserializeJson(jDoc, loc);
  
//   // Test if parsing succeeds.
//   if (!error) {
//     // do something with data - example
//     String i = jDoc["isp"];
//     Serial.println("connected via "+i);
//   }
//   else {
//     Serial.println("parseObject() failed");
//   }

//   return jDoc;

// }

String getMacStr() {
  char buffer[24];
  byte mac[6];                     // the MAC address
  
  WiFi.macAddress(mac);
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],mac[5]);
  return buffer;
}

void connectToWifi() {
  int connectionTries = 0;
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      connectionTries++;
      // Reboot if taking too long to connect
      if (connectionTries > 15) {
        rebootBoard();
      }
  }

  Serial.println("");
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: "));
  Serial.println(String(WiFi.localIP()));
}

/* Checks connection to our API by calling \health */
void checkApiConn() {
  HTTPClient httpC;
  String url = API_HEALTH;
  bool connected = true;
  
  Serial.print(F("Checking API connection: "));
  Serial.println(url);

  httpC.begin(url);             // Specify destination for HTTP request
  int httpCode = httpC.GET();   // Make the GET request

  // Check status of request
  if (httpCode == 200) {
    Serial.println(F("Connection Successful"));
    // Success -> carry on
  } else {
    if (httpCode <= 0) {
      Serial.print(F("HTTP Error. HTTP Code: "));
      Serial.println(httpCode);
    } else {
      String resp = httpC.getString();
      Serial.print(F("API Connection Failure. HTTP Code: "));
      Serial.println(httpCode);
      Serial.println(resp);
    }
    // TODO: logic for connection failure?
    rebootBoard();

  }

  httpC.end();  //Free resources
}

void registerDevice() {
  Serial.println(F("Registering Device"));
  String groupNumber = "0";
  String macAddr = getMacStr();
  // Body of HTTP Post - empty list of devices to start out
  String dataStr = "{\"gn\":\"" + groupNumber + "\",\"espmac\":\"" + macAddr + "\"}";
  
  String resp = postJsonHTTP(API_REGISTER_DEVICE, dataStr);
    
  // on /register-device failure
  if (resp == ERROR_RESP) {
    rebootBoard();
  }
}

void setUpBLEScan() {
  Serial.println(F("Setting up BLEScan"));
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}


/*****************
 *   Set Up 
 *****************/
void setup()
{ 
  pinMode(LED_PIN, OUTPUT); // configures the LED pin to behave as an output

  // allows for resetting (rebooting) the board
  // digitalWrite(RESET_PIN, HIGH);
  // delay(200); 
  // pinMode(RESET_PIN, OUTPUT);

  Serial.begin(115200);
  while(!Serial){delay(100);} // Serial=true when Serial port is ready
  Serial.println(F("*************** Booting up... ******************"));

  Serial.print(F("MAC: "));
  Serial.println(getMacStr());

  // We start by connecting to a WiFi network
  connectToWifi();
  
  //StaticJsonDocument<2000> locationDoc = getGeoLocation();

  // Check connection to our API
  checkApiConn();

  // Register device api call
  registerDevice();

  setUpBLEScan();

  // setup our timebase
  mainTask.attach(runPeriod, setRunFlag);
  
}

/**************************
 *   Other Helper Functions 
 **************************/
void setRunFlag() {
  timeout = true;
}

void blink() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// void printMap() {
//   Serial.println(F("Printing Map"));
//   for(const auto& elem : bleDevices){
//     Serial.print(elem.first);
//     Serial.print(" ");
//     Serial.pringln(elem.second);
//   }
// }

void checkWifiConnection() {
  Serial.print(F("Checking WiFi Connection to "));
  Serial.println(ssid);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Reconnecting WiFi"));
    WiFi.disconnect();
    delay(500);
    connectToWifi();
  } else {
    Serial.println(F("Connected"));
  }
}

void scan() {
  Serial.println(F("*********** Beginning Scan ***************"));    
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  Serial.println("");
  Serial.println(F("*********** Scan Ended ***************"));
}

void logDevice() {
  Serial.println("Logging Devices");
  
  String groupNumber = "0";
  String macAddr = getMacStr();
  // Body of HTTP Post - empty list of devices to start out
  String dataStr = "{\"gn\":\"" + groupNumber + "\",\"espmac\":\"" + macAddr + "\",\"devices\":[";
  // manually format the json for each device in the map
  for (std::map<String,String>::iterator it=bleDevices.begin(); it!=bleDevices.end(); ++it) {
    String mac = it->first;
    String rssi = it->second;
    dataStr += "{\"mac\": \"" + mac + "\", \"rssi\": \"" + rssi + "\"},";
  }
  // remove extra comma
  if (bleDevices.size() > 0)
    dataStr.remove(dataStr.length() - 1);
  // close the list and the json object
  dataStr += "]}";
  
  // post the data
  postJsonHTTP(API_LOG_DEVICE, dataStr);

  // clear the map
  bleDevices.clear();

  // TODO: what to do on /log-device failure?
}

/*****************
 *   Main Loop 
 *****************/
void loop()
{
  // TEMPRORARY: for testing code
  if (iteration < 10) {

    blink(); // tells us our device is up and running

    scan();
    delay(200);

    // true when time to log-devices
    if (timeout) {
      checkWifiConnection();
      //logDevice();

      timeout = false;
    }

    iteration++;
  }
}

