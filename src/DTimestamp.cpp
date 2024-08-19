
/**
 * 
 *  SystemClock Library
 *  Copyright (C) 2024  Daniel L Toth
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published 
 *  by the Free Software Foundation, either version 3 of the License, or any 
 *  later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *  
 *  The author can be contacted at dan@leelanausoftware.com  
 *
 */

#include "DTimestamp.h"

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

void DTimestamp::update() {
  unsigned long currentMillis  = millis();
  unsigned long elapsedMillis  = currentMillis - _millis;
  _millis                      = currentMillis;  
 _ntpTime.addMillis(elapsedMillis);
}
 
} // End of namespace lsc