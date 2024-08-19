
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

#include "SystemClock.h"

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

SystemClock::SystemClock()  {
  _timeServer = NTPTime::getTimeServerAddress();
  _initDate.initialize(0,JAN1_2024,0);
  _sysTime.initialize(Instant(0,JAN1_2024,0));
  _syncTimer.set(0,_ntpSync,0);  
  _syncTimer.setHandler([this]{ 
                updateSysTime(); 
                _syncTimer.start();
            });
  _syncTimer.start();
}
/**
 *   Set timezone offset. A proper timezone offset has hours between -14 and +14, and minutes either 0, 15, 30, or 45. 
 */
void SystemClock::tzOffset( double hours ) {
  int h = ((hours<=-14)?(-14):((hours>14)?(14):((int)hours)));
  double fraction = hours - h;
  if( fraction < 0 ) {
     fraction = ((fraction<=-.75)?(-.75):((fraction<=-.5)?(-.5):(-.25)));
  }
  else if (fraction > 0 ) {
     fraction = ((fraction>=.75)?(.75):((fraction>=.5)?(.5):(.25)));
  }
  _tzOffset = (int32_t)(3600.0*(double)h + 3600.0*fraction);
}

void SystemClock::timerOFF(boolean flg) {
  if( !flg && timerOFF() ) {       // syncTimer is OFF and we are turning it ON
     _timerOFF=flg;
     resetSyncTimer();
  }
  else if( flg && timerON() ) {    // syncTimer is ON and we are turning it OFF 
     _timerOFF=flg;
     resetSyncTimer();
  }
}

void SystemClock::resetSyncTimer() {
  _syncTimer.set(0,ntpSync(),0);
  _syncTimer.reset();
  if( timerON() ) _syncTimer.start();
}

void SystemClock::ntpSync(unsigned int min) {
/**
 *   NTP synchronization minimum is 15 min and maximum is 24 hours
 */
  unsigned int refresh = ((min<15)?(15):((min>1440)?(1440):(min)));
  _ntpSync = refresh;
  _nextSync = _lastSync + ntpSync()*60;
  resetSyncTimer();
}

Instant SystemClock::sysTime() {
  if( (_lastSync == 0) || (_sysTime.ntpTime().secs() > _nextSync)  )  return updateSysTime();
  else return _sysTime.update().ntpTime();
}

Instant SystemClock::updateSysTime() {
  Instant ofst;
  _sysTime           = NTPTime::updateSysTime(ofst,_sysTime);
  if( _lastSync == 0 ) _start = _sysTime;
  _lastSync          = _sysTime.ntpTime().secs();
  _nextSync          = _lastSync + ntpSync()*60;
  resetSyncTimer();
  return _sysTime.ntpTime();
}

} // End of namespace lsc
