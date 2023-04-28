#include "WiFi.h"
#include "HTTPClient.h"
#include "esp_wps.h"
#include "ArduinoJson.h"
#include <Arduino.h>
#include <Ticker.h>
#include <map>

#define LED_PIN 2


const float runPeriod = 1; //seconds

bool timeout = false;

Ticker mainTask;

const char* ssid     = "UCSD-DEVICE"; 
const char* password = "Fj7UPsFHb84e";

String API_BASE_URL = "http://cse191.ucsd.edu/api"; // root url of api
String API_HEALTH = API_BASE_URL + "/health";
String API_REGISTER_DEVICE = API_BASE_URL + "/register-device";


// create MAC storage, keys are mac addresses, values are rssi values
std::map<std::string, double> macAddresses;


String  postJsonHTTP(String url, String jLoad) {

  HTTPClient httpC;
  String resp="";   

  // debug info
  Serial.println("POST: "+url);
  Serial.println("JSON: "+jLoad);

  
  httpC.begin(url);                        //Specify destination for HTTP request
  httpC.addHeader("Content-Type", "application/json");

  int httpCode = httpC.POST(jLoad);              //Send the actual POST request

  if(httpCode>0){

    resp = httpC.getString();    //Get the response to the request
    
    Serial.print("POST Code: ");
    Serial.println(httpCode);   //Print return code
    Serial.println(resp);       //Print request answer

  }
  else {
    resp = "ERROR";
    Serial.print("Error on sending POST: ");
    Serial.println(httpCode);

  }

  httpC.end();  //Free resources

  return resp;
 }

StaticJsonDocument<2000>  getGeoLocation() {

  StaticJsonDocument<2000> jDoc;
  
  // get geo location info
  String locStr = "https://api.iplocation.net/?ip=99.10.120.238";
  
  // debug info
  Serial.println("Getting Location: "+locStr);
  String loc = postJsonHTTP(locStr, "");

  // debug
  Serial.println(loc);

  // parse json (need zipcode and timezone) -> jDoc
  DeserializationError error = deserializeJson(jDoc, loc);
  
  // Test if parsing succeeds.
  if (!error) {
    // do something with data - example
    String i = jDoc["isp"];
    Serial.println("connected via "+i);
  }
  else {
    Serial.println("parseObject() failed");
  }

  return jDoc;

}

String getMacStr() {
  char buffer[24];
  byte mac[6];                     // the MAC address
  
  WiFi.macAddress(mac);
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],mac[5]);
  return buffer;
}

/* Checks connection to our API by calling \health */
void checkApiConn() {
  HTTPClient httpC;
  String url = API_HEALTH;
  bool connected = true;
  
  Serial.println("Checking API connection: " + url);

  httpC.begin(url);             // Specify destination for HTTP request
  int httpCode = httpC.GET();   // Make the GET request

  // Check status of request
  if (httpCode == 200) {
    Serial.println("Connection Successful");
    // Success -> carry on
  } else {
    if (httpCode <= 0) {
      Serial.print("HTTP Error. HTTP Code: ");
      Serial.println(httpCode);
    } else {
      String resp = httpC.getString();
      Serial.print("API Connection Failure. HTTP Code: ");
      Serial.println(httpCode);
      Serial.println(resp);
    }
    // TODO: logic for connection failure?
  }

  httpC.end();  //Free resources
}

void registerDevice() {
  String groupNumber = "69";
  String macAddr = getMacStr();
  // Body of HTTP Post - empty list of devices to start out
  String dataStr = "{\"gn\":\"" + groupNumber + "\",\"espmac\":\"" + macAddr + "\"}";
  
  postJsonHTTP(API_REGISTER_DEVICE, dataStr);
  // TODO: what to do on /register-device failure?
}

void logDevice() {
  String groupNumber = "69";
  String macAddr = getMacStr();
  // Body of HTTP Post - empty list of devices to start out
  String dataStr = "{\"gn\":\"" + groupNumber + "\",\"espmac\":\"" + macAddr + "\",\"devices\":[";
  // manually format the json for each device
  for (int i = 0; i < macAddresses.size(); i++) {
    
  }
  
  // post the data
  postJsonHTTP(API_REGISTER_DEVICE, dataStr);

  // TODO: what to do on /log-device failure?
}

void setup()
{
  pinMode(LED_PIN, OUTPUT); // configures the LED pin to behave as an output

  Serial.begin(115200);
  while(!Serial){delay(100);} // Serial=true when Serial port is ready

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }


  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC: ");
  Serial.println(getMacStr());

  StaticJsonDocument<2000> locationDoc = getGeoLocation();

  // Check connection to our API
  checkApiConn();

  // Register device api call
  registerDevice();
 

  // setup our timebase
  mainTask.attach(runPeriod, setRunFlag);
  
}

void setRunFlag() {
  timeout = true;
}


void blink() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}


void loop()
{
  if (timeout) {
    
    // perform the scan, get whatever data structure it gives

    // convert that data structure to the map (skip existing values)


    // m.insert(std::make_pair("a", 1));
   
    // POST the data


    // CSE191 to do - if beacons are found log them using our API
    // Example from starter code:
      // String apiUrl = "http://cse191.ucsd.edu/api00/test";
      // String dataStr = "{\"name\":\"Ian\",\"msg\":\"IoT is fun\"}";

      // String resp = postJsonHTTP(apiUrl, dataStr);

    timeout = false;
  }
}

