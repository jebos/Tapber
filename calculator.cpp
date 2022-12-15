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

#include "calculator.h"
#include "consumptionProfiles.h"

std::vector<CalculationResponse>
  Calculator::allPricesForProfile(const TibberResponse& response, 
                                   const std::vector<double>& profile) {
  std::vector<CalculationResponse> calculationResponse;                                    
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now);

  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  
  for (int i = 0; i < response.today.size(); ++i) {                     
    double cost = calculateCost( response.today,
                                 response.tomorrow,
                                 currentHour, 
                                 currentMinute, 
                                 i, 
                                 profile);
                                 
    if (cost > 0) {
      calculationResponse.push_back( {
        calculationResponse.size(),
        isoDateTimeStringToClock(response.today.at(i).sinceDate),
        cost 
      });
    }
    
    currentHour += 1; 
    // dont adjust the minute - timers in many devices only allow to adjust the hour!
  }

  currentHour = 0;
  
  for (int i = 0; i < response.tomorrow.size(); ++i) {
    double cost = calculateCost( {}, // do not consider today
                                 response.tomorrow,
                                 currentHour, 
                                 currentMinute, 
                                 i, 
                                 profile);
    if (cost > 0) {
      calculationResponse.push_back( {
        calculationResponse.size(),
        isoDateTimeStringToClock(response.tomorrow.at(i).sinceDate),
        cost
      });
    }
    
    currentHour += 1;
    
    if (currentHour > 23) {
      currentHour = 0;
    }    
  }

  return calculationResponse;
}

double Calculator::calculateCost(const std::vector<TibberPriceInfo>& today,
                     const std::vector<TibberPriceInfo>& tomorrow,
                     int currentHour, 
                     int currentMinute,
                     int startingIndex, 
                     const std::vector<double>& profile)
{
  double totalCostOfAction = 0;
  int currentPriceIndex = startingIndex;

  auto *priceSource = today.size() > 0 ? &today : &tomorrow;
  
  for (int i = 0; i < profile.size(); ++i) {
    if (priceSource->size() <= currentPriceIndex) { 
      break;
    }
    
    totalCostOfAction += profile.at(i) / 1000.0 * priceSource->at(currentPriceIndex).price;

    // each profile entry is for 10 minutes
    currentMinute += ConsumptionProfile::timePerEntry;

    // if we are over 60 we add one hour
    if (currentMinute > 60) {
      // Serial.println("Next price to consider");

      currentHour += 1;
      currentMinute -= 60;
      // there is a now price for each hour, so add one to the the index
      currentPriceIndex += 1;

      // if we move over the day, lets change the data source
      // and reset index and hour
      if (currentHour >23) {
        if (priceSource != &tomorrow) {
          // Serial.println("Next day to consider");
          currentHour = 0;
          priceSource = &tomorrow;
          currentPriceIndex = 0;
        } else {
          return -1;
        }
      }
    }
  }

  // Serial.println("totalCostOfAction" + String(totalCostOfAction));

  return totalCostOfAction;
}

String Calculator::isoDateTimeStringToClock ( String isoDatetime )
{
  // read iso time from startsAt and only read the time from it
  struct tm tm = {0};
  char buf[100];

  strptime(isoDatetime.c_str(), "%Y-%m-%dT%H:%M:%S.%fZT", &tm);
  strftime(buf, sizeof(buf), "%H:%M", &tm);
  return String(buf);
}
