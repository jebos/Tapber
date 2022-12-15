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

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "tibber.h"

struct CalculationResponse {
    unsigned int originalIndex {0};
    String time; //date is not relevant! can only be today or tomorrow
    double priceOfAction {0.0};
};

static bool sortByLowestPrice(CalculationResponse a, CalculationResponse b)
{
  return a.priceOfAction < b.priceOfAction;
}

static bool sortByIndex(CalculationResponse a, CalculationResponse b)
{
  return a.originalIndex < b.originalIndex;
}

class Calculator {

public:
  Calculator(){}
  ~Calculator() = default;
  
  std::vector<CalculationResponse>
    allPricesForProfile(const TibberResponse& response, 
                        const std::vector<double>& profile);

  String isoDateTimeStringToClock ( String isoDatetime );
  
private:
  double calculateCost(const std::vector<TibberPriceInfo>& today,
                       const std::vector<TibberPriceInfo>& tomorrow,
                       int currentHour, 
                       int currentMinute,
                       int startingIndex, 
                       const std::vector<double>& profile);
};

#endif
