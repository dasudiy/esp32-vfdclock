#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

// WiFi network details
const char *ssid = "ssid";
const char *password = "password";

// NTP server
const char *ntpServer = "pool.ntp.org";
const int timeZone = 8; // Change this to your time zone in hours from UTC

// UDP
WiFiUDP udp;
unsigned int localPort = 2390; // Local port to listen for UDP packets
byte packetBuffer[48];         // Buffer to hold incoming and outgoing packets

// put function declarations here:
void displayDigit(int grid, int digit);
void blinkColon();
time_t getNtpTime();
void sendNtpPacket();

#define NUM_DIGITS 4
#define NUM_SEGMENTS 7
#define COLON_GRID 9
// GPIO pin assignments for the grids 
const int gridPins[NUM_DIGITS] = {15, 11, 6, 2};


// GPIO pin assignments for the segments
const int segmentPins[NUM_SEGMENTS] = {3, 5, 10, 7, 12, 4, 13};

// GPIO pin assignments for the colon
const int colonPins[2] = {14, 8};

// Segment patterns for each digit (0-9)
const bool digitPatterns[10][NUM_SEGMENTS] = {
    {0, 0, 0, 0, 0, 0, 1}, // 0
    {1, 0, 0, 1, 1, 1, 1}, // 1
    {0, 0, 1, 0, 0, 1, 0}, // 2
    {0, 0, 0, 0, 1, 1, 0}, // 3
    {1, 0, 0, 1, 1, 0, 0}, // 4
    {0, 1, 0, 0, 1, 0, 0}, // 5
    {0, 1, 0, 0, 0, 0, 0}, // 6
    {0, 0, 0, 1, 1, 1, 1}, // 7
    {0, 0, 0, 0, 0, 0, 0}, // 8
    {0, 0, 0, 0, 1, 0, 0}  // 9
};

void setup()
{
  // Initialize the grid pins as output and set them high
  for (int i = 0; i < NUM_DIGITS; i++)
  {
    pinMode(gridPins[i], OUTPUT);
    digitalWrite(gridPins[i], HIGH);
  }

  // Initialize the segment pins as output and set them high
  for (int i = 0; i < NUM_SEGMENTS; i++)
  {
    pinMode(segmentPins[i], OUTPUT);
    digitalWrite(segmentPins[i], HIGH);
  }

  // Initialize the colon pins as output and set them high
  for (int i = 0; i < 2; i++)
  {
    pinMode(colonPins[i], OUTPUT);
    digitalWrite(colonPins[i], HIGH);
  }
  
  digitalWrite(COLON_GRID, LOW);
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    blinkColon();
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize the UDP client
  udp.begin(localPort);
}

void loop()
{
  // Check if it's time to update the time
  if (timeStatus() != timeSet)
  {
    setTime(getNtpTime());
    Serial.println(now());
  }
  
  // Display the current time on the display
  for (int i = 0; i < NUM_DIGITS; i++)
  {
    int digit;
    if (i == 0)
      digit = hour() / 10;
    else if (i == 1)
      digit = hour() % 10;
    else if (i == 2)
      digit = minute() / 10;
    else
      digit = minute() % 10;
    displayDigit(i, digit);
  }
    
  // Blink the colon every second
  blinkColon();
}

time_t getNtpTime()
{
  // Send an NTP packet to the time server
  sendNtpPacket();

  // Wait for a response for up to 1 second
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1000)
  {
    int size = udp.parsePacket();
    if (size >= 48)
    {
      udp.read(packetBuffer, 48);
      unsigned long secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }

  // If no response after 1 second, return 0
  return 0;
}

void sendNtpPacket()
{
  // Initialize the packet buffer to 0
  memset(packetBuffer, 0, 48);

  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision

  // Send the packet
  udp.beginPacket(ntpServer, 123);
  udp.write(packetBuffer, 48);
  udp.endPacket();
}

void displayDigit(int grid, int digit)
{
  // Turn off all grids
  for (int i = 0; i < NUM_DIGITS; i++)
  {
    digitalWrite(gridPins[i], HIGH);
  }

  // Set the segment pins according to the digit pattern
  for (int i = 0; i < NUM_SEGMENTS; i++)
  {
    digitalWrite(segmentPins[i], digitPatterns[digit][i]);
  }

  // Turn on the grid for this digit
  digitalWrite(gridPins[grid], LOW);

  delayMicroseconds(1000); // Display this digit for a short time

  // Turn off the grid for this digit
  digitalWrite(gridPins[grid], HIGH);
}

void blinkColon()
{
  // Get the current time
  unsigned long currentMillis = millis();

  // Blink the colon every second
  if (currentMillis % 1000 < 500)
  {
    digitalWrite(colonPins[0], LOW);
    digitalWrite(colonPins[1], LOW);
  }
  else
  {
    digitalWrite(colonPins[0], HIGH);
    digitalWrite(colonPins[1], HIGH);
  }
}