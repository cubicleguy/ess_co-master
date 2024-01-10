/*
 *  The co-master is for evaluating Epson IMU devices with CANopen interface.
 *  Copyright(C) SEIKO EPSON CORPORATION 2021. All rights reserved.
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
 
#ifndef __DEBUG_PRINT__
#define __DEBUG_PRINT__


#if defined(NDEBUG)
#define debug_print(...)
#else
#define debug_print(...) \
  { fprintf(stderr, ">>>%s:%d:%s():", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__); }                                      
#endif


#endif //< __DEBUG_PRINT__
