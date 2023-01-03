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

#ifndef CONSUMPTIONPROFILES_H
#define CONSUMPTIONPROFILES_H

#include <list>

namespace ConsumptionProfile {
  
  // 10min consumptionProfiles
  // must be at least 1 and at max 60 and %2 == 0!
  static const int timePerEntry = 10;

  // entries in w/h
  static const std::vector<double> dryer_closetDry = {4, 42, 92, 93, 95, 95, 95, 96, 96, 97, 94, 90, 85, 80, 55};
  static const std::vector<double> washingMachine_60deg = {24, 16, 34, 14, 14, 18, 12, 12, 12, 209, 340, 340, 18};
  static const std::vector<double> washingMachine_40deg = {10, 180, 290, 11, 11, 14, 15, 15, 15, 15, 16, 15, 16, 15, 35, 1};

  // sometimes the timer in the appliance is setting the start time, sometimes its setting the done time, default to start time and allow adjustments if needed, go with hours not minutes for now
  // todo: should i change this to bool and count the entries in the energyProfile?
  // todo: change to minutes?
  static const int washingMachine_40deg_TimerAdjustment = -3;
  static const int washingMachine_60deg_TimerAdjustment = -2;
  static const int dryer_closetDry_TimerAdjustment = -3;
}

#endif
