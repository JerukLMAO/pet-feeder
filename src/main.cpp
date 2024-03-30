#include <Arduino.h>
#include <ArduinoJson.h>
#include <CTBot.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>

TBMessage msg;
CTBot myBot;
WiFiUDP ntpUDP;
Servo servo1;
Servo servo2;

String ssid = "Fadgib";
String pass = "fadhligibran12";
String token = "6526441782:AAFJM0zrJW1GmXOcQazzifJE3h-oxG7k8Hg";
const int id = 1221284266;
int hasFedCat = 0;
int hasFedFish = 0;
int interval = 21600;
int intervalms = 60000;
int prevEpoch = 0;
boolean ledState = LOW;
volatile boolean catFed = false;
volatile boolean fishFed = false;
volatile boolean buttonPressed = false;
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
  catFed = true;
}

ICACHE_RAM_ATTR void fish() {
  buttonPressed = true;
  feedFish();
}

ICACHE_RAM_ATTR void cat() {
  buttonPressed = true;
  feedCat();
}

void setup() {
  servo1.attach(5, 500, 2500);
  servo2.attach(4, 500, 2500);
  pinMode(13, OUTPUT);
  pinMode(14, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);

  attachInterrupt(12, fish, FALLING);
  attachInterrupt(14, cat, FALLING);

  servo1.write(95);

  myBot.wifiConnect(ssid, pass);
  myBot.setTelegramToken(token);

  timeClient.begin();
  timeClient.update();

  if (myBot.testConnection()) {
    myBot.sendMessage(id, "Bot dinyalakan 🤖");
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
    myBot.sendMessage(id, "Telah diberi makan");
    catFed = false;
  }
  if (buttonPressed == true) {
    myBot.sendMessage(id, "Tombol ditekan 🕹");
    buttonPressed = false;
  }

  if (timeClient.getEpochTime() - prevEpoch >= interval) {
    prevEpoch = timeClient.getEpochTime();
    feedCat();
    myBot.sendMessage(id, "waktunya makan ⏰");
  }

  if (myBot.getNewMessage(msg)) {
    if (msg.text == "Feed") {
      feedCat();
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
      myBot.sendMessage(id, "Udh makan " + String(hasFedCat) + " kali, sekarang jam " + timeClient.getFormattedTime());
    }
    if (msg.text == "Sync ntp") {
      myBot.sendMessage(id, "Sinkronisasi NTP...");
      timeClient.forceUpdate();
    }
    if (msg.text == "Woy") {
      myBot.sendMessage(id, "Yang bener aje 😂");
    }
  }
}