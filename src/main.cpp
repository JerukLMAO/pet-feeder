#include <Arduino.h>
#include <ArduinoJson.h>
#include <CTBot.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

TBMessage msg;
CTBot myBot;
WiFiUDP ntpUDP;
Servo servo1;
Servo servo2;
AsyncWebServer server(80);

String ssid = "Fadgib";
String pass = "fadhligibran12";

String token = "6526441782:AAFJM0zrJW1GmXOcQazzifJE3h-oxG7k8Hg";
const int id = 1221284266;

unsigned long interval = 21600;
int prevEpoch = 0;

volatile int hasFedCat = 0;
volatile int hasFedFish = 0;
volatile boolean catFed = false;
volatile boolean fishFed = false;
volatile boolean buttonPressed = false;

boolean ledState = LOW;

NTPClient timeClient(ntpUDP, "ntp.bmkg.go.id", 25200, 30000);

void feedCat() {
  servo1.write(30);
  delayMicroseconds(500000);
  servo1.write(95);
  delayMicroseconds(300000);
  servo1.write(30);
  delayMicroseconds(500000);
  servo1.write(95);
  hasFedCat++;
  catFed = true;
}

void feedFish() {
  servo1.write(60);
  delayMicroseconds(500000);
  servo1.write(90);
  hasFedFish++;
  fishFed = true;
}

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

IRAM_ATTR void fish() {
  buttonPressed = true;
  feedFish();
}

IRAM_ATTR void cat() {
  buttonPressed = true;
  feedCat();
}

void setup() {
  servo1.attach(5, 500, 2500);
  servo1.write(95);
  servo2.attach(4, 500, 2500);
  pinMode(13, OUTPUT);

  pinMode(14, INPUT_PULLUP);
  attachInterrupt(14, cat, FALLING);
  pinMode(12, INPUT_PULLUP);
  attachInterrupt(12, fish, FALLING);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "Hi! I am ESP8266.");
  });

  ElegantOTA.begin(&server);
  server.begin();

  timeClient.begin();
  timeClient.update();

  myBot.setTelegramToken(token);

  if (myBot.testConnection()) {
    myBot.sendMessage(id, "Bot dinyalakan, terhubung ke " + ssid + " IP: " + ipToString(WiFi.localIP()));
  }
}

void loop() {

  if (ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }

  digitalWrite(13, ledState);


  if (catFed == true) {
    myBot.sendMessage(id, "Kucing telah diberi makan");
    catFed = false;
  }

  if (fishFed == true) {
    myBot.sendMessage(id, "Ikan telah diberi makan");
    catFed = false;
  }

  if (buttonPressed == true) {
    myBot.sendMessage(id, "Tombol ditekan 🕹");
    buttonPressed = false;
  }
 
  if (timeClient.getEpochTime() - prevEpoch >= interval) {
    prevEpoch = timeClient.getEpochTime();
    feedCat();
    feedFish();
    myBot.sendMessage(id, "waktunya makan ⏰");
  }


  if (myBot.getNewMessage(msg)) {
    if (msg.text == "Feed cat") {
      feedCat();
    }
    if (msg.text == "Feed fish") {
      feedFish();
    }
    if (msg.text == "Next feed") {
      int t = interval - (timeClient.getEpochTime() - prevEpoch);
      int s = t % 60;
      t = (t - s) / 60;
      int m = t % 60;
      t = (t - m) / 60;
      int h = t;
      myBot.sendMessage(id, "Dijadwalkan diberi makan dalam " + String(h) + ":" + String(m) + ":" + String(s));
    }
    if (msg.text == "Ingfo") {
      myBot.sendMessage(id, "Telah memberi makan kucing " + String(hasFedCat) + " kali, dan ikan " + String(hasFedCat) + " kali, NTP: " + timeClient.getFormattedTime());
    }
    if (msg.text == "Sync ntp") {
      myBot.sendMessage(id, "Sinkronisasi NTP...");
      timeClient.forceUpdate();
    }
  }
}
