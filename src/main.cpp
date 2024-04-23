#include <Arduino.h>
#include <ArduinoOTA.h>  // Library for updating via OTA
#include <ArduinoJson.h> // For parsing JSON data (used by Telegram bot)
#include <CTBot.h>       // Library for interacting with Telegram bot
#include <NTPClient.h>   // Library for getting time from NTP servers
#include <WiFiUdp.h>     // Library for UDP communication (used by NTPClient)
#include <Servo.h>       // Library for controlling servos

TBMessage msg;  // Stores Telegram message data
CTBot myBot;    // Object for interacting with Telegram bot
WiFiUDP ntpUDP; // UDP object for NTP communication
Servo servo1;   // Servo object for controlling the cat feeder mechanism
Servo servo2;   // Servo object for controlling the fish feeder mechanism
WiFiClient wifiClient;


String ssid = "Fadgib";         // Your Wi-Fi network name (SSID)
String pass = "fadhligibran12"; // Your Wi-Fi network password

String ipfyAddress = String("api.ipify.org");
unsigned long abortTime;
String ip;

String token = "6526441782:AAFJM0zrJW1GmXOcQazzifJE3h-oxG7k8Hg"; // Your Telegram bot token
const int id = 1221284266;                                       // Telegram chat ID where messages will be sent

unsigned long interval = 21600; // Feeding interval in seconds (6 hours)
int prevEpoch = 0;              // Stores the previous epoch time for feeding schedule

volatile int hasFedCat = 0;             // Counts the number of times the cat has been fed
volatile int hasFedFish = 0;            // Counts the number of times the fish have been fed
volatile boolean catFed = false;        // Flag to indicate if the cat has been fed in the current loop iteration
volatile boolean fishFed = false;       // Flag to indicate if the fish have been fed in the current loop iteration
volatile boolean buttonPressed = false; // Flag to indicate if a button has been pressed

boolean ledState = LOW; // LED state (LOW or HIGH)

NTPClient timeClient(ntpUDP, "ntp.bmkg.go.id", 25200, 30000); // Sets up NTP client for time synchronization with Indonesian time server

void feedCat()
{
  servo1.write(30);          // Move servo to position for dispensing cat food
  delayMicroseconds(500000); // Wait for 0.5 seconds (500 milliseconds)
  servo1.write(95);          // Move servo back to closed position (95 degrees)
  delayMicroseconds(300000); // Wait for 0.3 seconds (300 milliseconds)

  // Repeat the movement for a second feeding cycle to ensure proper dispensing
  servo1.write(30);
  delayMicroseconds(500000);
  servo1.write(95);
  delayMicroseconds(300000);

  // Increment the counter for cat feeding
  hasFedCat++;

  // Set flag to indicate cat has been fed in this loop iteration
  catFed = true;
}

void feedFish()
{
  servo2.write(60);          // Move servo to position for dispensing fish food
  delayMicroseconds(500000); // Wait for 0.5 seconds (500 milliseconds)
  servo2.write(90);          // Move servo back to closed position (90 degrees)

  // Increment the counter for fish feeding
  hasFedFish++;

  // Set flag to indicate fish has been fed in this loop iteration
  fishFed = true;
}

String ipToString(IPAddress ip)
{
  // Creates an empty string to store the formatted IP address
  String s = "";

  // Loop iterates four times, once for each byte of the IP address
  for (int i = 0; i < 4; i++)
  {
    // Conditional expression for adding separators (".") and byte values
    s += i ? "." + String(ip[i]) : String(ip[i]);
  }

  // Returns the formatted string containing the human-readable IP address
  return s;
}

String getPublicIP()
{
  wifiClient.connect(ipfyAddress, 80);
  abortTime = millis() + 5000;
  wifiClient.println("GET / HTTP/1.1");
  wifiClient.println("Host: " + ipfyAddress);
  wifiClient.println();
  delay(5000);

  while (wifiClient.available())
  {
    if (abortTime > millis())
    {
      ip = "!!! Wifi Client Timedout!";
      wifiClient.stop();
      return "";
    }

    ip = wifiClient.readStringUntil('\n');
  }

  return ip;
}

// Interrupt Service Routine (ISR) for fish button press
IRAM_ATTR void fish()
{
  // Set global flag to indicate button press has occurred
  buttonPressed = true;
  // Trigger fish feeding process
  feedFish();
}

// Interrupt Service Routine (ISR) for cat button press
IRAM_ATTR void cat()
{
  // Set global flag to indicate button press has occurred
  buttonPressed = true;
  // Trigger cat feeding process
  feedCat();
}

void setup()
{
  // Attach servo1 to pin 5 with pulse width range of 500-2500 microseconds
  servo1.attach(5, 500, 2500);
  // Set initial position for servo1 (95 degrees)
  servo1.write(95);
  // Attach servo2 to pin 4 with pulse width range of 500-2500 microseconds
  servo2.attach(4, 500, 2500);

  // Set pin 13 as output for LED control
  pinMode(13, OUTPUT);

  // Set pin 14 as input with internal pull-up resistor (for cat button)
  pinMode(14, INPUT_PULLUP);
  // Attach interrupt to pin 14 to call the 'cat()' function on a falling edge (button press)
  attachInterrupt(14, cat, FALLING);
  // Set pin 12 as input with pull-up resistor (for fish button)
  pinMode(12, INPUT_PULLUP);
  // Attach interrupt to pin 12 to call the 'fish()' function on a falling edge
  attachInterrupt(12, fish, FALLING);

  // Connect to Wi-Fi network with specified SSID and password
  myBot.wifiConnect(ssid, pass);

  ArduinoOTA.begin();
  ArduinoOTA.onEnd([]()
                   { myBot.sendMessage(id, "Update OTA berhasil"); });

  // Initialize NTP client for time synchronization
  timeClient.begin();
  // Update time from NTP server
  timeClient.update();

  // Set the Telegram bot token for communication
  myBot.setTelegramToken(token);

  // Send a startup message to Telegram chat if connection is successful
  if (myBot.testConnection())
  {
    myBot.sendMessage(id, "Bot dinyalakan, terhubung ke " + ssid + ", IP: " + ipToString(WiFi.localIP()));
  }

  prevEpoch = timeClient.getEpochTime();

  delay(1000);
}

void loop()
{
  ArduinoOTA.handle();

  // Toggle LED state to provide a visual status indication
  if (ledState == LOW)
  {
    ledState = HIGH;
  }
  else
  {
    ledState = LOW;
  }
  digitalWrite(13, ledState);

  // Send Telegram messages for feeding events and reset flags
  if (catFed == true)
  {
    myBot.sendMessage(id, "Kucing telah diberi makan");
    catFed = false;
  }
  if (fishFed == true)
  {
    myBot.sendMessage(id, "Ikan telah diberi makan");
    fishFed = false;
  }
  if (buttonPressed == true)
  {
    myBot.sendMessage(id, "Tombol ditekan ");
    buttonPressed = false;
  }

  // Check for scheduled feeding time based on interval
  if (timeClient.getEpochTime() - prevEpoch >= interval)
  {
    prevEpoch = timeClient.getEpochTime();      // Update previous feeding time
    feedCat();                                  // Trigger cat feeding
    feedFish();                                 // Trigger fish feeding
    myBot.sendMessage(id, "waktunya makan ⏰"); // Send feeding notification
  }

  // Check for new Telegram messages and handle commands
  if (myBot.getNewMessage(msg))
  {
    // Trigger feeding based on commands
    if (msg.text == "Feed cat")
    {
      feedCat();
    }

    if (msg.text == "Feed fish")
    {
      feedFish();
    }

    if (msg.text == "Reset") 
    {
      prevEpoch = timeClient.getEpochTime();
      myBot.sendMessage(id, "Telah di reset");
    }

    if (msg.text == "Ip")
    {
    myBot.sendMessage(id, "http://" + getPublicIP() + ":8080/");
    }

    // Calculate and send time remaining for next scheduled feeding
    if (msg.text == "Next feed")
    {
      int t = interval - (timeClient.getEpochTime() - prevEpoch);
      int s = t % 60;
      t = (t - s) / 60;
      int m = t % 60;
      t = (t - m) / 60;
      int h = t;
      myBot.sendMessage(id, "Dijadwalkan diberi makan dalam " + String(h) + ":" + String(m) + ":" + String(s));
    }

    // Send status information (feeding counts and NTP time)
    if (msg.text == "Info")
    {
      myBot.sendMessage(id, "Telah memberi makan kucing " + String(hasFedCat) + " kali, dan ikan " + String(hasFedFish) + " kali. NTP: " + timeClient.getFormattedTime());
    }

    // Force NTP time update
    if (msg.text == "Update ntp")
    {
      myBot.sendMessage(id, "Mengupdate NTP...");
      timeClient.forceUpdate();
    }
  }
}

