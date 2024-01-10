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
 
#include "seSensingUnitsObj.h"

seObj SensUnit_M_G552PC1x[] = {
  { 0x100000, U32, CONST, 0x00020194, "Device Type"},    
  { 0x100100,  U8,    RO,       0x00, "Error Register"},
  { 0x100200, U32,    RO, 0x00000000, "Manufacturer Status Register"},    
  { 0x100500, U32,    RW, 0x00000080, "SYNC COB-ID"}, 
  { 0x100600, U32,    RW, 0x00002710, "Communication Cycle Period"}, 
  { 0x100800,  VS, CONST, 0x32353547, "Manufacturer Device Name"},      /*G552*/
  { 0x100900,  VS, CONST, 0x30314350, "Manufacturer Hardware Version"}, /*PC70 or 30314350h ("PC10")*/
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
  { 0x180002,  U8,    RW,       0xfe, "TPDO1 Transmission Type" },
  { 0x180102,  U8,    RO,       0xfe, "TPDO2 Transmission Type" },
  { 0x180202,  U8,    RO,       0xfe, "TPDO3 Transmission Type" },
  { 0x180302,  U8,    RO,       0xfe, "TPDO4 Transmission Type" },
  { 0x1A0001, U32, CONST, 0x21000010, "TPDO1 Mapping 1 = Tc" },
  { 0x1A0002, U32, CONST, 0x71300110, "TPDO1 Mapping 2 = Gx" },
  { 0x1A0003, U32, CONST, 0x71300210, "TPDO1 Mapping 3 = Gy" },
  { 0x1A0004, U32, CONST, 0x71300310, "TPDO1 Mapping 4 = Gz" },
  { 0x1A0101, U32, CONST, 0x21000010, "TPDO2 Mapping 1 = Tc" },
  { 0x1A0102, U32, CONST, 0x71300410, "TPDO2 Mapping 2 = Ax" },
  { 0x1A0103, U32, CONST, 0x71300510, "TPDO2 Mapping 3 = Ay" },
  { 0x1A0104, U32, CONST, 0x71300610, "TPDO2 Mapping 4 = Az" },     
  { 0x1A0201, U32, CONST, 0x21000010, "TPDO3 Mapping 1 = Tc" },
  { 0x1A0202, U32, CONST, 0x71300710, "TPDO3 Mapping 2 = Temperature" },
  { 0x1A0203, U32, CONST, 0x20220410, "TPDO3 Mapping 3 = RTD" }, //< Reserved or ANG2: AI input PV 9 for 30314350h ("PC10")
  { 0x1A0204, U32, CONST, 0x20220110, "TPDO3 Mapping 4 = STS" },           
  { 0x1A0301, U32, CONST, 0x21000010, "TPDO4 Mapping 1 = Tc" }, 
  { 0x1A0302, U32, CONST, 0x21010220, "TPDO4 Mapping 2 = Ms" }, //< Subs 2 and 
  { 0x1A0303, U32, CONST, 0x21010110, "TPDO4 Mapping 3 = Dy" }, //< 3 switched vs G550    
  { 0x1F8000, U32,    RW, 0x00000008, "NMT Startup Mode" },
  { 0x200001,  U8,    RW,       0x01, "CAN Node-ID" },
  { 0x200002,  U8,    RW,       0x03, "CAN Bitrate" },
  { 0x200100,  U8,    RW,       0x0A, "Sensor Sample Rate" },
  { 0x200500,  U8,    RW,       0x10, "Apply parameters" },    
  { 0x202001,  U8,    RW,       0x00, "Attitude control" },
  { 0x202002,  U8,    RW,       0x00, "Attitude axis conversion" },
  { 0x202003,  U8,    RW,       0x00, "Attitude motion profile" },
  { 0x210000, U16,    RW,     0x0000, "Trigger Counter" },
  { 0x210101, U16,    RO,     0x0000, "Timestamp Day" },
  { 0x210102, U32,    RO, 0x00000000, "Timestamp Millisecond" },
  { 0x611001, U16, CONST,     0x28A1, "AI Sensor Type 1" },
  { 0x611002, U16, CONST,     0x28A1, "AI Sensor Type 2" },
  { 0x611003, U16, CONST,     0x28A1, "AI Sensor Type 3" },
  { 0x611004, U16, CONST,     0x2905, "AI Sensor Type 4" },
  { 0x611005, U16, CONST,     0x2905, "AI Sensor Type 5" },
  { 0x611006, U16, CONST,     0x2905, "AI Sensor Type 6" },
  { 0x611007, U16, CONST,     0x0064, "AI Sensor Type 7" },
  { 0x611008, U16, CONST,     0x28A1, "AI Sensor Type 8" },
  { 0x611009, U16, CONST,     0x28A1, "AI Sensor Type 9" },
  { 0x61100A, U16, CONST,     0x28A1, "AI Sensor Type 10" },
  { 0x613101, U32, CONST, 0x00410300, "AI Physical Unit PV 1" },
  { 0x613102, U32, CONST, 0x00410300, "AI Physical Unit PV 2" },
  { 0x613103, U32, CONST, 0x00410300, "AI Physical Unit PV 3" },
  { 0x613104, U32, CONST, 0xFDF10000, "AI Physical Unit PV 4" },
  { 0x613105, U32, CONST, 0xFDF10000, "AI Physical Unit PV 5" },
  { 0x613106, U32, CONST, 0xFDF10000, "AI Physical Unit PV 6" },
  { 0x613107, U32, CONST, 0x002D0000, "AI Physical Unit PV 7" },
  { 0x613108, U32, CONST, 0x00000000, "AI Physical Unit PV 8" },
  { 0x613109, U32, CONST, 0x00000000, "AI Physical Unit PV 9" },
  { 0x61310A, U32, CONST, 0x00000000, "AI Physical Unit PV 10" },
  { 0x61A001,  U8, CONST,       0x02, "AI Filter Type 1" },
  { 0x61A002,  U8, CONST,       0x02, "AI Filter Type 2" },
  { 0x61A003,  U8, CONST,       0x02, "AI Filter Type 3" },
  { 0x61A004,  U8, CONST,       0x02, "AI Filter Type 4" },
  { 0x61A005,  U8, CONST,       0x02, "AI Filter Type 5" },
  { 0x61A006,  U8, CONST,       0x02, "AI Filter Type 6" },
  { 0x61A007,  U8, CONST,       0x02, "AI Filter Type 7" },
  { 0x61A008,  U8, CONST,       0x02, "AI Filter Type 8" },
  { 0x61A009,  U8, CONST,       0x02, "AI Filter Type 9" },
  { 0x61A00A,  U8, CONST,       0x02, "AI Filter Type 10" },
  { 0x61A101,  U8,    RW,       0x09, "AI Filter Setting Constant 1" },
  { 0x61A102,  U8,    RO,       0x08, "AI Filter Setting Constant 2" },
  { 0x61A103,  U8,    RO,       0x08, "AI Filter Setting Constant 3" },
  { 0x61A104,  U8,    RO,       0x08, "AI Filter Setting Constant 4" },
  { 0x61A105,  U8,    RO,       0x08, "AI Filter Setting Constant 5" },
  { 0x61A106,  U8,    RO,       0x08, "AI Filter Setting Constant 6" },
  { 0x61A107,  U8,    RO,       0x08, "AI Filter Setting Constant 7" },
  { 0x61A108,  U8,    RO,       0x08, "AI Filter Setting Constant 8" },
  { 0x61A109,  U8,    RO,       0x08, "AI Filter Setting Constant 9" },
  { 0x61A10A,  U8,    RO,       0x08, "AI Filter Setting Constant 10" },
  { 0x713001, I16,    RO,     0x0000, "AI Input PV 1 (Gx)" }, //< The resolution is 0.01515 [dps/LSB]
  { 0x713002, I16,    RO,     0x0000, "AI Input PV 2 (Gy)" },
  { 0x713003, I16,    RO,     0x0000, "AI Input PV 3 (Gz)" },
  { 0x713004, I16,    RO,     0x0000, "AI Input PV 4 (Ax)" }, //< The resolution is fixed to 0.4 [mg/LSB]
  { 0x713005, I16,    RO,     0x0000, "AI Input PV 5 (Ay)" },
  { 0x713006, I16,    RO,     0x0000, "AI Input PV 6 (Az)" },
  { 0x713007, I16,    RO,     0x0000, "AI Input PV 7 (Temp)" },
  { 0x713008, I16,    RO,     0x0000, "AI Input PV 8 (ANG1)" },
  { 0x713009, I16,    RO,     0x0000, "AI Input PV 9 (ANG2)" },
  { 0x71300A, I16,    RO,     0x0000, "AI Input PV 10 (-)" }
};                        
uint32_t SensUnit_M_G552PC1x_sz = sizeof(SensUnit_M_G552PC1x)/sizeof(SensUnit_M_G552PC1x[0]);                        


