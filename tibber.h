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

#ifndef TIBBER_H
#define TIBBER_H

// this class provides a simplified access to the tibber API

#include <WiFiClientSecure.h>
#include <list>

struct TibberPriceInfo {
  double price;
  String sinceDate;
  String level;
};

struct TibberResponse {

  TibberPriceInfo current;

  std::vector<TibberPriceInfo> today;
  std::vector<TibberPriceInfo> tomorrow;

  bool isValid {false};
  
  // some shortcuts
  double todayHighestPrice = 0;
  String todayHighestPriceStartsAt = "";
  
  double todayLowestPrice = 10;
  String todayLowestPriceStartsAt = "";
  
  double tomorrowHighestPrice = 0;
  String tomorrowHighestPriceStartsAt = "";
  
  double tomorrowLowestPrice = 10;
  String tomorrowLowestPriceStartsAt = "";
};

class Tibber {

public:
  Tibber();
  ~Tibber() = default;

  void initialize ();

  // calls the API
  // dont do this to often 
  // for constant access look into usage of Websockets
  // returns a tuple with the response and status (true=ok, false=nok)
  
  const TibberResponse& read();
  const TibberResponse& lastResponse() const;
  
private:

  TibberResponse m_lastResponse;
  
  
  bool connectToWebserver();
  bool queryAndParsePriceInformation();

  WiFiClientSecure m_client;
  bool m_initialized {false}; 
};

#endif
