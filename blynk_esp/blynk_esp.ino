/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on NodeMCU.

  Note: This requires ESP8266 support package:
    https://github.com/esp8266/Arduino

  Please be sure to select the right NodeMCU module
  in the Tools -> Board menu!

  For advanced settings please follow ESP examples :
   - ESP8266_Standalone_Manual_IP.ino
   - ESP8266_Standalone_SmartConfig.ino
   - ESP8266_Standalone_SSL.ino

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "DHTesp.h"


// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "083c530277e24169a6582d25e063e928";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "DoanhChu-Class";
char pass[] = "hocdinhe";

DHTesp dht;
BlynkTimer timer;

void myTimerEvent()
{
  delay(dht.getMinimumSamplingPeriod());

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  String res = dht.getStatusString();
  if (res.indexOf("OK") != -1) {
    // You can send any value at any time.
    // Please don't send more that 10 values per second.
    Blynk.virtualWrite(V0, humidity);
    Blynk.virtualWrite(V1, temperature);
  }
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  dht.setup(10); // Connect DHT sensor to GPIO 17

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, myTimerEvent);
}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
}

