/*ICE #5
 * Here we incorporate data from our MPL115A2 sensor to obtain barometric pressure readings
 * and our DHT22 sensor to obtain temperature and humidity. These values are then published 
 * to an MQTT server using the PubSubCLient library that allows us to send and receive messages 
 * from said server. My partner for this assignment, Peter Schultz, then uses the CallBack function 
 * in his sketch to obtain these values in JSON format, parse, and display them on his OLED board. 
 * I will also be publishing these values to my OLED display board to cross check that we are both 
 * displaying the same values. 
*/

#define DEBUG 0 // Set to 1 for debug mode - won't connect to server. Set to 0 to post data

/************************** Configuration ***********************************/
#include <ESP8266WiFi.h> 
#include <ArduinoJson.h> 
#include <Wire.h>  // for I2C communications
#include <Adafruit_Sensor.h>  // the generic Adafruit sensor library used with both sensors
#include <DHT.h>   // temperature and humidity sensor library
#include <PubSubClient.h>   //Needed to send and receive messages from server 
#include <Adafruit_GFX.h> //OLED graphics library 
#include <Adafruit_SSD1306.h> //OLED text and images library 
#include <Adafruit_MPL115A2.h> // Barometric pressure sensor library
#include <SPI.h> //OLED

#define wifi_ssid "STC2"   //Wifi
#define wifi_password "bahtsang" //Wifi password 

// pin connected to DH22 data line
#define DATA_PIN 12
#define DHTTYPE DHT22
DHT dht(DATA_PIN, DHTTYPE);

// create MPL115A2 instance
Adafruit_MPL115A2 mpl115a2;

// create OLED Display instance on an ESP8266
// set OLED_RESET to pin -1 (or another), because we are using default I2C pins D4/D5.
#define OLED_RESET -1
#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

Adafruit_SSD1306 display(OLED_RESET);

WiFiClient espClient;             //espClient
PubSubClient mqtt(espClient);     //tie PubSub (mqtt) client to WiFi client

char mac[18]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!
char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array

unsigned long currentMillis, timerOne, timerTwo, timerThree; //we are using these to hold the values of our timers

// the refresh interval in for posting new data, in ms
#define SERVER_REFRESH_DELAY 10000

/////SETUP/////
void setup() {
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
 // mqtt.setCallback(callback); //register the callback function
  timerOne = timerTwo = timerThree = millis();

  // start the serial connection
  Serial.begin(115200);
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));


  // wait for serial monitor to open
  while (! Serial);

  Serial.println("Weather Station Started.");
  Serial.print("Debug mode is: ");

  if (DEBUG) {
    Serial.println("on.");
  } else {
    Serial.println("off.");
  }

  // set up the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Starting up.");
  display.display();

  // initialize dht22 sensor
  dht.begin();

  // initialize MPL115A2 sensor 
  mpl115a2.begin();

  Serial.println("MPL115 Sensor Test"); Serial.println("");
  mplSensorDetails();
}

/////SETUP_WIFI/////
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  WiFi.macAddress().toCharArray(mac, 18);  //.macAddress returns a byte array 6 bytes representing the MAC address
}                       

/////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("Rodrigo/+"); //we are subscribing to 'Rodrigo' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
 //-------------GET THE TEMPERATURE (DHT22)--------------//

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
  
  // wait 5 seconds (5000 milliseconds == 5 seconds)
  delay(5000);

  //-------------GET THE PRESSURE--------------//

  float pressureKPA = 0, temperatureC = 0;

  pressureKPA = mpl115a2.getPressure();
  Serial.print("Pressure (kPa): "); Serial.print(pressureKPA, 4); Serial.println(" kPa");
  
  mqtt.loop(); //this keeps the mqtt connection 'active'
  if (!mqtt.connected()) {
    reconnect();
  }
  

  if (millis() - timerOne > 10000) {
    sprintf(message, "{\"temperature\":\"%f%\", \"humidity\": \"%f\", \"pressure\": \"%f\" }", t, h, pressureKPA); // %d is used for an int; the value assigned to legoBatmanIronyLevel is place into d 
    mqtt.publish("Rodrigo/ICE5", message);
    timerOne = millis();
  }

  //-------------UPDATE THE DISPLAY--------------//
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Temp.   : ");
  display.print(t);
  display.println("'C");
  display.print("Humidity: ");
  display.print(h);
  display.println("%");
  display.print("Pressure: ");
  display.print(pressureKPA);
  display.println(" kPa");
  display.display();
  
  } //end loop

void mplSensorDetails(void)
{
  float pressureKPA = 0, temperatureC = 0;

  mpl115a2.getPT(&pressureKPA, &temperatureC);
  Serial.print("Pressure (kPa): "); Serial.print(pressureKPA, 4); Serial.print(" kPa  ");
  Serial.print("Temp (*C): "); Serial.print(temperatureC, 1); Serial.println(" *C both measured together");

  pressureKPA = mpl115a2.getPressure();
  Serial.print("Pressure (kPa): "); Serial.print(pressureKPA, 4); Serial.println(" kPa");

  temperatureC = mpl115a2.getTemperature();
  Serial.print("Temp (*C): "); Serial.print(temperatureC, 1); Serial.println(" *C");
  delay(5000);
}
