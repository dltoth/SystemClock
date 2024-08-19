
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

#include "Timer.h"

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

Timer::Timer( const Timer& t ) {
  _millis   = t._millis;
  _setPoint = t._setPoint;
  _handler  = t._handler;
}

/**
void Timer::setPoint(int& h, int& m, int& s) {
  unsigned long seconds = setPointMillis()/1000;
  h = seconds/3600;
  seconds = seconds % 3600;
  m = seconds/60;
  s = seconds % 60; 
}
**/

void Timer::doDevice() {
  if(started()) {
    if((millis() > limit()) && (_handler!=NULL)) {
      reset();
      run();
    }
  }
  else if(paused()) {
    if(millis() > pauseLimit()) cancelPause();
  }
}

} // End of namespace lsc

