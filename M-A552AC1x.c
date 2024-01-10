/*
 *  The co-master is for evaluating Epson IMU devices with CANopen interface.
 *  Copyright(C) SEIKO EPSON CORPORATION 2022. All rights reserved.
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
 
#include "seSensingUnitsObj.h"

const char * unit_TiltAngleMode_M_A552AC1x[] = {"  ", "Ix", "Iy", "Iz", "T", "??"};  
const char * unit_AcceleromMode_M_A552AC1x[] = {"  ", "Ax", "Ay", "Az", "T", "??"};  

const char * SampleRates_M_A552AC1x[] = { 
  "0"/*disable*/,    "1000",     "500",  
  "invalid"     , "invalid",     "200", 
  "invalid"     , "invalid", "invalid", 
  "invalid"     ,     "100", "invalid", 
  "invalid"     , "invalid", "invalid", 
  "invalid"     , "invalid", "invalid",     
  "invalid"     , "invalid",      "50", 
  "\0" };    
const float fSampleRates_M_A552AC1x[] = {  
   0/*disable*/,  1000.0f, 500.0f, 
  -1.0f        ,    -1.0f, 200.0f, 
  -1.0f        ,    -1.0f,  -1.0f, 
  -1.0f        ,   100.0f,  -1.0f, 
  -1.0f        ,    -1.0f,  -1.0f, 
  -1.0f        ,    -1.0f,  -1.0f, 
  -1.0f        ,    -1.0f,  50.0f };
filterSet FilterSet_M_A552AC1x[] = {
/* C1, tap,FilterType, EligibleSampleRateIx, fc */
  { 0,   0, NOT_VALID,  0,   0 },  
  
  { 1,  64,       K64,  2,  83 },
  { 2,  64,       K64,  1, 220 },
  { 3, 128,      K128,  5,  36 },
  { 4, 128,      K128,  2, 110 },
  { 5, 128,      K128,  1, 350 },
  { 6, 512,      K512, 20,   9 },
  { 7, 512,      K512, 10,  16 },
  { 8, 512,      K512,  5,  60 },
  { 9, 512,      K512,  2, 210 },
  {10, 512,      K512,  1, 460 },
/* EligibleSampleRateIx not used for UDF*/
  {11,   4,        U4,  1,   0 },
  {12,  64,       U64,  1,   0 },
  {13, 128,      U128,  2,   0 }, 
  {14, 512,      U512,  1,   0 }, 
  {15,   0, NOT_VALID,  0,   0 }
};
uint32_t FilterSet_M_A552AC1x_sz = sizeof(FilterSet_M_A552AC1x)/sizeof(FilterSet_M_A552AC1x[0]);

seObj SensUnit_M_A552AC1x[] = {
  { 0x100000, U32, CONST, 0x00020194, "Device Type"},    
  { 0x100100,  U8,    RO,       0x00, "Error Register"},
  { 0x100200, U32,    RO, 0x00000000, "Manufacturer Status Register"},    
  { 0x100500, U32,    RW, 0x00000080, "SYNC COB-ID"}, 
  { 0x100600, U32,    RW, 0x00000000, "Communication Cycle Period"}, 
  { 0x100800,  VS, CONST, 0x32353541, "Manufacturer Device Name"},      /*A552*/
  { 0x100900,  VS, CONST, 0x30314341, "Manufacturer Hardware Version"}, 
  { 0x100A00,  VS, CONST, 0x30302e31, "Manufacturer Software Version"}, /*1.00*/
  { 0x101001,  VS,    RW, 0x00000001, "Save All Parameters" },
  { 0x101101,  VS,    RW, 0x00000001, "Restore All Default Parameters" },
  { 0x101200, U32, CONST, 0x80000100, "TIME COB-ID" },
  { 0x101700, U16,    RW,     0x0000, "Producer Heartbeat Time" },
  { 0x101900,  U8,    RW,       0x00, "Synchronous Counter Overflow Value" },
  { 0x120001, U32,    RO, 0x00000600, "RSDO COB-ID" }, //< +NID
  { 0x120002, U32,    RO, 0x00000580, "TSDO COB-ID" }, //< +NID
  { 0x180001, U32,    RW, 0x40000180, "TPDO1 COB-ID" }, //< +NID
  { 0x180101, U32,    RW, 0x40000280, "TPDO2 COB-ID" }, //< +NID
  { 0x180201, U32,    RW, 0xC0000380, "TPDO3 COB-ID" }, //< +NID
  { 0x180301, U32,    RW, 0xC0000480, "TPDO4 COB-ID" }, //< +NID
  { 0x180002,  U8,    RW,       0x01, "TPDO1 Transmission Type" },
  { 0x180102,  U8,    RO,       0x01, "TPDO2 Transmission Type" },
  { 0x180202,  U8,    RO,       0x01, "TPDO3 Transmission Type" },
  { 0x180302,  U8,    RO,       0x01, "TPDO4 Transmission Type" },
  { 0x1A0001, U32, CONST, 0x91300120, "TPDO1 Mapping 1 = Ax" },
  { 0x1A0002, U32, CONST, 0x91300220, "TPDO1 Mapping 2 = Ay" },
  { 0x1A0101, U32, CONST, 0x91300320, "TPDO2 Mapping 1 = Az" },
  { 0x1A0102, U32, CONST, 0x21000010, "TPDO2 Mapping 2 = Sc" },     
  { 0x1A0201, U32, CONST, 0x21010110, "TPDO3 Mapping 1 = Dy" },
  { 0x1A0202, U32, CONST, 0x21010220, "TPDO3 Mapping 2 = Ms" },         
  { 0x1A0301, U32, CONST, 0x91300420, "TPDO4 Mapping 1 = Tmp" },  
  { 0x1F8000, U32,    RW, 0x00000008, "NMT Startup Mode" },
  { 0x200001,  U8,    RW,       0x01, "CAN Node-ID" },
  { 0x200002,  U8,    RW,       0x00, "CAN Bitrate" },
  { 0x200100, U32,    RW, 0x00000002, "Timer interval" },
  { 0x200500,  U8,    RW,       0x10, "Apply parameters" },    
  { 0x200700,  U8,    RW,       0x00, "User Defined Filter Parameter Set" },
  { 0x200801, U16,    RO,     0x0000, "Number of taps" },
  { 0x200802, U16,    RW,     0x0000, "Start/Current address" },
  { 0x200803, I32,    RW, 0x00000000, "Read/Write data" },
  { 0x210000, U16,    RW,     0x0000, "Sample counter" },
  { 0x210101, U16,    RO,     0x0000, "Timestamp Day" },
  { 0x210102, U32,    RO, 0x00000000, "Timestamp Millisecond" },
  { 0x611001, U16, CONST,     0x2905, "AI Sensor Type 1" },
  { 0x611002, U16, CONST,     0x2905, "AI Sensor Type 2" },
  { 0x611003, U16, CONST,     0x2905, "AI Sensor Type 3" },
  { 0x611004, U16, CONST,     0x2905, "AI Sensor Type 4" },
  { 0x613101, U32, CONST, 0x00F10000, "AI Physical Unit PV 1" },
  { 0x613102, U32, CONST, 0x00F10000, "AI Physical Unit PV 2" },
  { 0x613103, U32, CONST, 0x00F10000, "AI Physical Unit PV 3" },
  { 0x613104, U32, CONST, 0x002D0000, "AI Physical Unit PV 4" },
  { 0x61A001,  U8, CONST,       0x02, "AI Filter Type 1" },
  { 0x61A002,  U8, CONST,       0x02, "AI Filter Type 2" },
  { 0x61A003,  U8, CONST,       0x02, "AI Filter Type 3" },
  { 0x61A004,  U8, CONST,       0x00, "AI Filter Type 4" },
  { 0x61A101,  U8,    RW,       0x09, "AI filter tap constant 1" },
  { 0x61A102,  U8,    RO,       0x09, "AI filter tap constant 2" },
  { 0x61A103,  U8,    RO,       0x09, "AI filter tap constant 3" },
  { 0x61A104,  U8,    RO,       0x09, "AI filter tap constant 4" },
  { 0x913001, I32,    RO, 0x00000000, "AI Input PV 1" }, 
  { 0x913002, I32,    RO, 0x00000000, "AI Input PV 2" },
  { 0x913003, I32,    RO, 0x00000000, "AI Input PV 3" },
  { 0x913004, I32,    RO, 0x00000000, "AI Input PV 4 (Tmp)" },
};
uint32_t SensUnit_M_A552AC1x_sz = sizeof(SensUnit_M_A552AC1x)/sizeof(SensUnit_M_A552AC1x[0]);                        


