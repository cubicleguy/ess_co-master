/*
 *  The co-master is for evaluating Epson IMU devices with CANopen interface.
 *  Copyright(C) SEIKO EPSON CORPORATION 2021-2022. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
 
#ifndef SE_MASTER_HPP
#define SE_MASTER_HPP
#include <lely/coapp/master.hpp>
#include <lely/util/chrono.hpp>

#include <lely/co/nmt.h>
#include <lely/co/time.h>

#include "debug_print.h"

#define COMASTER_VERSION   "1.01"

class seMaster : public lely::canopen::AsyncMaster 
{
 public:
  using AsyncMaster::AsyncMaster;

  /**
   * SendTime(time_point&) can be used to send the message at a later time. 
   * If you use that, make sure to use the clock used by the master 
   * (via GetClock()) to create the time point.
   */
  template <class Clock, class Duration>
  bool SendTime(const ::std::chrono::time_point<Clock, Duration>& t) {
    ::std::lock_guard<BasicLockable> lock(*this);
    
    co_time_t * time = co_nmt_get_time((const co_nmt_t*)nmt());
    if (!time) {
      debug_print("Send TIME failed\n");
      return false;
    }
    auto start = ::lely::util::to_timespec(t);
    co_time_start_prod(time, &start, nullptr);
    debug_print("tv_sec:%ld, tv_nsec:%lu\n", start.tv_sec, start.tv_nsec);
    return true;
  }  
  /**
   * SendTime() sends a TIME message immediately
   */  
  bool SendTime(void) {
    return SendTime(GetClock().gettime());
  }  

  /**
   * StartTime(1000ms) periodically sends a TIME message
   */
  template <class Rep, class Period>  
  bool StartTime(const ::std::chrono::duration<Rep, Period>& d ) {
    ::std::lock_guard<BasicLockable> lock(*this);
    auto time = co_nmt_get_time((const co_nmt_t*)nmt());
    if (!time) {
      debug_print("Start TIME failed\n");
      return false;
    }
    auto interval = ::lely::util::to_timespec(d);    
    co_time_start_prod(time, nullptr, &interval);
    debug_print("interval tv_sec:%ld, tv_nsec:%lu\n", interval.tv_sec, interval.tv_nsec);
    return true;
  }
  /**
   * StopTime() stops StartTime() sending TIME message
   */
  void StopTime() {
    ::std::lock_guard<BasicLockable> lock(*this);
    auto time = co_nmt_get_time((const lely::CONMT*)nmt());
    if (time) co_time_stop_prod(time);
  }
  
};

#endif
