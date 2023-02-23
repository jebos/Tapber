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

#include "tibber.h"
#include "credentials.h"

#include <ArduinoJson.h>

namespace CONSTANTS {
  // tibber API
  const char* API_SERVER = "api.tibber.com";  // Server URL
  const int   HTTPS_PORT = 443;
    
  // SHA1 Fingerprint of api.tibber.com certificate
  const char SSL_FINGERPRINT[] PROGMEM = "01 E9 FC DD 05 1D 0B C3 21 DB 38 0E 8C C1 10 4E 38 CF 40 DF";
}

Tibber::Tibber() {
}

void Tibber::initialize () {
  m_client.setFingerprint(CONSTANTS::SSL_FINGERPRINT);
  m_initialized = true;
}

const TibberResponse& Tibber::read() {

  m_lastResponse = TibberResponse();
  
  if (connectToWebserver()) {
    if (queryAndParsePriceInformation()) {}
  }
  
  return m_lastResponse;
}

const TibberResponse& Tibber::lastResponse() const{
 return m_lastResponse;
}

bool Tibber::connectToWebserver() {
  if (!m_initialized) {
    return false;
  }
  
  int r = 0; //retry counter
  while ((!m_client.connect(CONSTANTS::API_SERVER, CONSTANTS::HTTPS_PORT)) && (r < 30)) {
    delay(100);
    Serial.print(".");
    r++;
  }

  if (r == 30) {
    Serial.println("Connection failed");
    // displayError("Server Connection Failed");
    delay(1000);
    return false;
  }
  else {
    Serial.println("Connected to web");
  }

  return true;
}

bool Tibber::queryAndParsePriceInformation() {
  
  DynamicJsonDocument doc(6000);
  
  doc["query"] = "{ viewer { homes { currentSubscription { priceInfo { today { total startsAt } tomorrow { total startsAt } current { total startsAt level } } } } } }";

  m_client.println("POST /v1-beta/gql HTTP/1.1");
  m_client.println("Host: " + String(CONSTANTS::API_SERVER));
  m_client.println("Authorization: Bearer " + String(TIBBER_TOKEN));
  m_client.println("Content-Type: application/json");
  m_client.print("Content-Length: ");
  m_client.println(measureJson(doc));
  m_client.println();
  serializeJson(doc, m_client);
  m_client.println();

  delay(100);
  
  while (m_client.connected()) {
    Serial.println("connected read headers");
    String line = m_client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  String response = "";

  delay(300);
  
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (m_client.available()) {
    char c = m_client.read();
    response = response + c;
  }

  m_client.stop();

  // Deserialize the resonse into the JSON document
  // Reuse the existing document
  DeserializationError error = deserializeJson(doc, response);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }
  
  // read the current price informations from response json
  // let us assume the API is stable and we will know about changes a
  // TODO: add some checks
  // TODO: make this look nicer
  
  m_lastResponse.current.price = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["current"]["total"];
  m_lastResponse.current.sinceDate = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["current"]["startsAt"].as<String>();
  m_lastResponse.current.level = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["current"]["level"].as<String>();

  // m_lastResponse.current.sinceDate = isoDateTimeStringToClock(m_lastResponse.current.sinceDate);

  // those are JsonArrays
  // containting the hourly prices
  JsonArray today = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["today"];
  JsonArray tomorrow = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["tomorrow"];

  // do not consider historical data for today
  // there is no timemachine to go back and use a lower price (well, maybe there is, but I'm pretty sure the energy consumption must be paid in the current price range, though...

  bool considerTodaysInfo = false;
  int currentPriceInTodaysIndex = 0;

  // todo use limits
  m_lastResponse.todayHighestPrice = -1;
  m_lastResponse.todayLowestPrice = 10;
  
  for (int i = 0; i < today.size(); ++i) {
    if (String(today[i]["startsAt"]) == m_lastResponse.current.sinceDate) {
      considerTodaysInfo = true;
      currentPriceInTodaysIndex = i;
    }

    if (!considerTodaysInfo) {
      continue;
    }

    TibberPriceInfo info;
    
    info.price = today[i]["total"];
    info.sinceDate = static_cast<String>(today[i]["startsAt"]);
     
    m_lastResponse.today.push_back(info);
    
    if (today[i]["total"] > m_lastResponse.todayHighestPrice) {
      m_lastResponse.todayHighestPrice = today[i]["total"];
      m_lastResponse.todayHighestPriceStartsAt = static_cast<String>(today[i]["startsAt"]);
    }

    if (today[i]["total"] < m_lastResponse.todayLowestPrice) {
      m_lastResponse.todayLowestPrice = today[i]["total"];
      m_lastResponse.todayLowestPriceStartsAt = static_cast<String>(today[i]["startsAt"]);
    }
  }

  // no data yet for tomorrow
  if (tomorrow.size() == 0) {
    m_lastResponse.tomorrowHighestPrice = -1;
    m_lastResponse.tomorrowLowestPrice = 10;
  }
  
  for (int i = 0; i < tomorrow.size(); ++i) {
    TibberPriceInfo info;
    
    info.price = tomorrow[i]["total"];
    info.sinceDate = static_cast<String>(tomorrow[i]["startsAt"]);
     
    m_lastResponse.tomorrow.push_back(info);

    if (tomorrow[i]["total"] > m_lastResponse.tomorrowHighestPrice) {
      m_lastResponse.tomorrowHighestPrice = tomorrow[i]["total"];
      m_lastResponse.tomorrowHighestPriceStartsAt = static_cast<String>(tomorrow[i]["startsAt"]);
    }
    
    if (tomorrow[i]["total"] < m_lastResponse.tomorrowLowestPrice) {
      m_lastResponse.tomorrowLowestPrice = tomorrow[i]["total"];
      m_lastResponse.tomorrowLowestPriceStartsAt = static_cast<String>(tomorrow[i]["startsAt"]);
    }
  }

  return true;
}
