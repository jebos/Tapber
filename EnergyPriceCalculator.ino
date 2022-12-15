/*
Energy Price Calculator
Copyright (C) 2022  Jeremias Bosch

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SSD1306Wire.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <sys/time.h>  
#include <coredecls.h>  
#include <list>

#include "credentials.h"
#include "consumptionProfiles.h"
#include "tibber.h"
#include "calculator.h"

// addess the ssd1306 oled on wemos d1
SSD1306Wire  display(0x3c, D2, D1);

// Screen Handling defines
#define SCREEN_DISPLAY_DURATION 10000

// Network SSID
const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWD;

// ntp time information
const char* ntpServer = "pool.ntp.org";
const char *TZstr = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00 ";
timeval cbtime;
bool cbtime_set = false;  

typedef void (*Screen)(void);

int currentScreen = 0;
int counter = 1;

String nowString;

int currentPriceInTodaysIndex = -1;
Tibber tibberAPI;
Calculator calculator;

void time_is_set (void)
{
  time_t t = time (nullptr);

  gettimeofday (&cbtime, NULL);
  cbtime_set = true;
  Serial.println
    ("------------------ settimeofday() was called ------------------");
  printf (" local asctime: %s\n",
   asctime (localtime (&t))); // print formated local time

  // set RTC using t if desired
}

void initializeSerial() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  delay(100);
}

void initializeDisplay() {
  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  startupProgress(5);
}

void connectWifi() {
  WiFi.hostname("Price Requester A");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  startupProgress(8);
}

void printNow() {
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now);

  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%S", timeinfo);
  nowString = String(timeStringBuff);
  Serial.println(nowString);

}
void initializeClock() {
  settimeofday_cb (time_is_set);
  time_t rtc_time_t = 946684800; 
  timeval tv = { rtc_time_t, 0 }; 
  settimeofday (&tv, NULL);
  configTime (TZstr, ntpServer);
}

void setup() {
  initializeSerial();
  initializeDisplay();
  initializeClock();
  connectWifi();  
  
  tibberAPI.initialize();
  
  startupProgress(20);
  
  tibberAPI.read();
  
  startupProgress(80);
  
  auto dryerClosetDryCost = calculator.allPricesForProfile(tibberAPI.lastResponse(), ConsumptionProfile::dryer_closetDry);
  sort(dryerClosetDryCost.begin(), dryerClosetDryCost.end(), &sortByLowestPrice);
  

  Serial.println("----" + (String) dryerClosetDryCost.front().priceOfAction + " " + (String) dryerClosetDryCost.back().priceOfAction );
 
  
  startupProgress(100);
}

void displayError(String message) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Error: ");
  display.drawString(0, 10, message);
}

void displayMessage(String message) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Info: ");
  display.drawString(0, 10, message);
}

void startupProgress(int progress) {
  display.clear();
  // draw the progress bar
  display.drawProgressBar(0, 32, 120, 10, progress);

  // draw the percentage as String
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 15, String(progress) + "%");
  display.display();
}

void drawInfo() {
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Verbunden: " + String(WiFi.status() == WL_CONNECTED ? "Ja" : "Nein"));
  display.drawString(0, 12, "IP: " + WiFi.localIP().toString());
  display.drawString(0, 32, "Stand: 09.12.22 - 10:35" );
}

void drawCurrentPrice() {

  double highestPrice = tibberAPI.lastResponse().tomorrowHighestPrice > tibberAPI.lastResponse().todayHighestPrice ?
                            tibberAPI.lastResponse().tomorrowHighestPrice 
                          : tibberAPI.lastResponse().todayHighestPrice;
                          
  String highestAt = tibberAPI.lastResponse().tomorrowHighestPrice > tibberAPI.lastResponse().todayHighestPrice ?
                            tibberAPI.lastResponse().tomorrowHighestPriceStartsAt
                          : tibberAPI.lastResponse().todayHighestPriceStartsAt;         
                          
  drawPrice("Aktueller Preis: ",
            tibberAPI.lastResponse().current.price, 
            "Höchster: " + String(highestPrice) + "Um:" + calculator.isoDateTimeStringToClock(highestAt));
}


void drawProfileCost(String title, const std::vector<double>& profile) {
  auto cost = calculator.allPricesForProfile(tibberAPI.lastResponse(), profile);
  sort(cost.begin(), cost.end(), &sortByIndex);
  auto now = cost.front();
  sort(cost.begin(), cost.end(), &sortByLowestPrice);

  drawActionCost( title,
            "Jetzt: " + String(now.priceOfAction) + "EUR",
            "+" + String(cost.front().originalIndex) + "  " + String(cost.front().priceOfAction) + "EUR",
            "+" + String(cost.back().originalIndex) + "  " + String(cost.back().priceOfAction)  + "EUR");
}

void drawWashing60Cost() {
  drawProfileCost("Waschmaschine 60",ConsumptionProfile::washingMachine_60deg);
}

void drawDryerClosetDryCost() {
  drawProfileCost("Trockner Schranktrocken", ConsumptionProfile::dryer_closetDry);
}

void drawPrice(String startsAt, double price, String meta) {
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, startsAt);
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 12, String(price));
  display.setFont(ArialMT_Plain_10);
  display.drawString(50, 25, "Euro | 1kWh");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 45, meta);
}

void drawActionCost(String title, String now, String low, String high) {
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, title);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 12, now);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 26, low);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 45, high);
}

// Screen screens[] = {drawInfo, drawPrice, drawCostOfWashing, drawCostOfDryer, recommendationA, recommendationB};
Screen screens[] = {drawCurrentPrice, drawWashing60Cost, drawDryerClosetDryCost}; //, drawCurrentPrice, drawTodayHighestPrice, drawTodayLowestPrice, drawTomorrowHighestPrice, drawTomorrowLowestPrice};

int screenCount = (sizeof(screens) / sizeof(Screen));
long timeSinceLastModeSwitch = 0;

void loop() {  
  if(!cbtime_set)    // don't do anything until NTP has set time
    return;

  // clear the display
  display.clear();
  screens[currentScreen]();

  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(10, 128, String(millis()));
  // write the buffer to the display
  display.display();

  if (millis() - timeSinceLastModeSwitch > SCREEN_DISPLAY_DURATION) {
    currentScreen = (currentScreen + 1)  % screenCount;
    timeSinceLastModeSwitch = millis();
    counter++;
  }
  
  delay(10);

  if (counter == 30) {
    display.displayOff();
    ESP.deepSleep(0);
  }
}
