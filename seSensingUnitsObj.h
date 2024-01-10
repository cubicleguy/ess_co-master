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
 
#ifndef SE_SENSING_UNITS_OBJ_H
#define SE_SENSING_UNITS_OBJ_H

/* This ifdef allows the header to be used from both C and C++. */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define FLAG_G552PC_6DOF    0x00000001



typedef enum { U8=0x00003855, VS=0x00005356, I32=0x00323349, U32=0x00323355, I16=0x00363149, U16=0x00363155, AU=0x00005541 } odDataType;
typedef enum { CONST, RO, RW } odAccessType;
typedef struct {     
  union {
    uint32_t Addr;

    struct {
      uint32_t SUB         : 8;  /*!< [0...7] Sub index No        */
      uint32_t INDEX       : 16; /*!< [8..24] Index No            */
    } ADDR_b;
  };
} odAddr;

  
typedef struct {
  odAddr       Addr;
  odDataType   DataType;
  odAccessType AccessType;
  uint32_t     DefaultValue;
  char         ObjectName[64];
} seObj;

typedef enum {  
  MA =0x0000414D, 
  K32=0x0032334B, K64=0x0034364B, K128=0x3832314B, K512=0x3231354B, 
  U4 =0x00003455, U64=0x00343655, U128=0x38323155, U512=0x32313555, 
  NOT_VALID=0
             } filterType;

enum { UDF_LOAD=1, UDF_SAVE=2, UDF_ERASE=3, UDF_VERIFY=4 };

typedef struct {
  uint8_t AI_Filter_Setting_Constant_1;
  uint16_t tap;
  filterType FilterType; 
  uint8_t EligibleSampleRateIx;  
  uint16_t FreqCut;       
}filterSet;

typedef struct {
  odAddr       Addr;
  odDataType   DataType;
  odAccessType AccessType;
  uint32_t     Value;
  uint32_t     Flag;
} seOption;

// M_G550PC2x
extern seObj SensUnit_M_G550PC2x[];
extern uint32_t SensUnit_M_G550PC2x_sz;
extern const float fSampleRates_M_G550PC2x[];
extern const char * SampleRates_M_G550PC2x[];
extern filterSet FilterSet_M_G550PC2x[];
extern uint32_t FilterSet_M_G550PC2x_sz;

// M_G552PC1x
extern seObj SensUnit_M_G552PC1x[];
extern uint32_t SensUnit_M_G552PC1x_sz;

// M_G552PC7x
extern seObj SensUnit_M_G552PC7x[];
extern uint32_t SensUnit_M_G552PC7x_sz;
extern const float fSampleRates_M_G552PC7x[];
extern const char * SampleRates_M_G552PC7x[];
extern filterSet FilterSet_M_G552PC7x[];
extern uint32_t FilterSet_M_G552PC7x_sz;


// M_A552AC1x
extern seObj SensUnit_M_A552AC1x[];
extern uint32_t SensUnit_M_A552AC1x_sz;
extern const float fSampleRates_M_A552AC1x[];
extern const char * SampleRates_M_A552AC1x[];
extern filterSet FilterSet_M_A552AC1x[];
extern uint32_t FilterSet_M_A552AC1x_sz;
extern const char * unit_TiltAngleMode_M_A552AC1x[];
extern const char * unit_AcceleromMode_M_A552AC1x[];

#ifdef __cplusplus
}
#endif

#endif //<SE_SENSING_UNITS_OBJ_H

