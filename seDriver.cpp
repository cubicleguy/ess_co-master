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
 
#include <stdio.h>
#include <wait.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "seDriver.hpp"
#include "seSensingUnitsObj.h"
#include "debug_print.h"

#define printme(...) if(m_ShowMe){fprintf(stderr, __VA_ARGS__);}
extern char can_ch[32];

using namespace std::chrono_literals;
using namespace lely;
using namespace lely::ev;
using namespace lely::io;
using namespace lely::canopen;

uint32_t  seDriver::HeartBeatSlave{NO};
uint32_t  seDriver::HeartBeatMaster{NO};
uint32_t  seDriver::CANbitRateIx{NO}; 
uint32_t  seDriver::MaxSample{0}; 
uint32_t  seDriver::NMT{NO};
uint32_t  seDriver::NewNodeID{NO};
uint32_t  seDriver::TimeStamp{NO};
uint32_t  seDriver::DisplayNode{NO};
uint32_t  seDriver::Filter{0};
uint32_t  seDriver::ReadWrite{NO};
uint32_t  seDriver::ReadWriteDataType{AU};
uint32_t  seDriver::ReadWriteIndex{NO};
uint32_t  seDriver::ReadWriteSub{NO};
uint32_t  seDriver::ReadWriteVal{NO};
uint32_t  seDriver::TPDOSettings{NO};
uint32_t  seDriver::TPDOMode{NO};
uint32_t  seDriver::TPDOverflowCnt{NO};
uint32_t  seDriver::Dump{0};
bool      seDriver::RestoreAllSettings{false};
bool      seDriver::LogFile{false};
bool      seDriver::SaveParams{false};
bool      seDriver::RawOutput{false};
bool      seDriver::Fahrenheit{false};
char      seDriver::AppStartTime[32];
uint32_t  seDriver::UnitCnt{0};
uint32_t  seDriver::UnitBootCnt{0};
uint32_t  seDriver::Sync{NO};
uint32_t  seDriver::SyncPeriod{NO};    //< SYNC Cycle Period in msec
uint32_t  seDriver::SyncOverflow{NO};  //< SYNC Counter Overflow 
uint32_t  seDriver::UDFParam{NO};
uint32_t  seDriver::FIRCoeff[512];
uint32_t  seDriver::ApplyChanges{NO};
bool      seDriver::UDFFile{false};
char    * seDriver::SampleRatePzs{NULL};

  
seDriver::seDriver(ev_exec_t* exec, seMaster& master, uint8_t id) 

: FiberDriver(exec, (canopen::AsyncMaster&)master, id) 
, m_fpLogFile(NULL)
, m_fpMsgFile(NULL)
, m_Error(false)
, m_ShowMe(false)
, m_InitialBoot(true)
, m_SampleProcessing(false)
, m_SampleCounter(0)
, m_BootState(NmtState::START)
, m_DeviceName(0)
, m_HardwareVersion(0)
, m_SoftwareVersion(0)
, m_exVal(0)
, m_Days(0)
, m_DaysInSec(0)
, m_comaix(0)
, m_GyroScale(0)
, m_AccelScale(0)
, m_AttiScale1(0)
, m_AttiScale2(0)
, m_TiltAngleScale(0)
, m_DumpSize(0)
, m_DumpObj(NULL)
, m_OptionSize(0)
, m_FilterSetSize(0)
, m_FilterSet(NULL)
, m_SampleRateStr(NULL)
, m_SampleRateFloat(NULL)
, m_SampleTime(0.0f)
, m_SampleIncremTime(0.0f)
, m_TransmissionMode(0)
, m_UnitVar(0)
, m_unitModeStr(0)
, m_comacnt(0)
, m_TPDOSettings(TPDOSettings)
, m_NMT(NMT)
, m_ChangeModeTime(  5000000) //< 5 sec
, m_RestoreParamTime(1000000) //< 1 sec
, m_SaveParamTime(    500000) //< 0.5 sec
, m_ApplyParamTime(  1000000) //< 1 sec
, m_OtherTime(          1000) //< 1 msec
{   
  memset(m_strtmp, 0, 1000);
  memset(m_strlog, 0, 1000);
  memset(m_strtmp_extra, 0, 64);
  memset(m_strlog_extra, 0, 64);  
  memset(m_comastr, 0,  16);
  memset(m_DumpFIRCoeff, 0, 512 * sizeof(uint32_t)); 
  
  m_ShowMe = id == (uint8_t)DisplayNode;
  if ( NewNodeID != NO || RestoreAllSettings || 
    CANbitRateIx != NO || DisplayNode == SHOW_ALL) 
  {
      m_ShowMe = true;
  }
  
  UnitCnt++; //< CAN unit counter
  
} 

seDriver::~seDriver() 
{ 
  debug_print("Node %d, deleting...\n", id());
  if ( m_fpLogFile ) {
    debug_print("Node %d: closing log file...\n", id());
    if (m_SampleCounter) {
      fprintf(m_fpLogFile, "%s\n\n", m_comastr+m_comaix); 
    } else {
      fprintf(m_fpLogFile, "%s", "\n\n");  
    }

    fclose(m_fpLogFile);
    m_fpLogFile = NULL;
  }
  if ( m_fpMsgFile ) {
    fclose(m_fpMsgFile);
    m_fpMsgFile = NULL;
  }
}
  
void seDriver::setRawOutput( bool val )
{ 
  RawOutput = val;      
}
    
uint32_t seDriver::tryReadWriteDataType( char * s )
{
  str2upcase( s );
  debug_print("%s\n", s);  
  ReadWriteDataType  = *((uint32_t*)s);
  if (strcmp(s, "U8")==0 || strcmp(s, "VS")==0) {
    ReadWriteDataType &= 0x00ffffff;
  }
  debug_print("ReadWriteDataType: %.4s (%.08xh)\n", (char*)&ReadWriteDataType, ReadWriteDataType); 
  /* U8, U16, U32, I16, VS */
  switch (ReadWriteDataType)
  {
    case 0x00003855: //<  U8
    case 0x00005356: //<  VS
    case 0x00323355: //< U32
    case 0x00323349: //< I32
    case 0x00363149: //< I16
    case 0x00363155: //< U16      
    break;
    
    default:
      ReadWriteDataType = NO;
  }

  return ReadWriteDataType;
}

int seDriver::tryNMTcmd( char * s )
{ 
  str2upcase( s );
  debug_print("%s\n", s);
  
  if (      ! strcmp(s,       "START"     )) NMT =   1;
  else if ( ! strcmp(s,        "STOP"     )) NMT =   2;
  else if ( ! strcmp(s, "ENTER_PREOP"     )) NMT = 128;
  else if ( ! strcmp(s,  "RESET_NODE"     )) NMT = 129;
  else if ( ! strcmp(s,  "RESET_COMM"     )) NMT = 130;
  else if ( ! strcmp(s,           "1"     )) NMT =   1;
  else if ( ! strcmp(s,           "2"     )) NMT =   2;
  else if ( ! strcmp(s,         "128"     )) NMT = 128;
  else if ( ! strcmp(s,         "129"     )) NMT = 129;
  else if ( ! strcmp(s,         "130"     )) NMT = 130;
  else     /* failed */                      return -1;  

  return NMT;    
}
    
int seDriver::trySampleRate( char * sr )
{ 

  if (      ! strcmp(sr, "1000"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,  "500"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,  "250"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,  "125"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,   "62.5"   )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,   "31.25"  )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,   "15.625" )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,  "400"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,  "200"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,  "100"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,   "80"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,   "50"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,   "40"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,   "25"     )) SampleRatePzs = sr;
  else if ( ! strcmp(sr,   "20"     )) SampleRatePzs = sr; 
  else     /* failed */                return  -1;  
  
  return 0;
}
 
uint32_t seDriver::setSampleRate(char * srate)
{      
    for (uint32_t i=0; m_SampleRateStr[i] != NULL; i++) {
        if (strcmp(m_SampleRateStr[i], srate) == 0) {
            
            if ( m_DeviceName == *(uint32_t*)"A552" ) {
              Wait(AsyncWrite<uint32_t>(0x2001, 0, (uint32_t)i));
            } else {
              Wait(AsyncWrite<uint8_t>(0x2001, 0, (unsigned char)i));
            }
            USleep(m_OtherTime);
            debug_print("SampleRateIx=%u\n", i);
            return i;      
        }
    }
    
    debug_print("SampleRateIx=%u\n", NO);
    
    return NO;
}
 
int seDriver::tryCANbitRate( char * br )
{ 
  str2upcase( br );
    
  if (      ! strcmp(br, "1M"     )) CANbitRateIx =  0;
  else if ( ! strcmp(br,  "800K"  )) CANbitRateIx =  1;
  else if ( ! strcmp(br,  "500K"  )) CANbitRateIx =  2;
  else if ( ! strcmp(br,  "250K"  )) CANbitRateIx =  3;
  else if ( ! strcmp(br,  "125K"  )) CANbitRateIx =  4;
  else if ( ! strcmp(br,   "50K"  )) CANbitRateIx =  5;
  else if ( ! strcmp(br,   "20K"  )) CANbitRateIx =  6;
  else if ( ! strcmp(br,   "10K"  )) CANbitRateIx =  7;
  else if ( ! strcmp(br, "1000000")) CANbitRateIx =  0;
  else if ( ! strcmp(br,  "800000")) CANbitRateIx =  1;
  else if ( ! strcmp(br,  "500000")) CANbitRateIx =  2;
  else if ( ! strcmp(br,  "250000")) CANbitRateIx =  3;
  else if ( ! strcmp(br,  "125000")) CANbitRateIx =  4;
  else if ( ! strcmp(br,   "50000")) CANbitRateIx =  5;
  else if ( ! strcmp(br,   "20000")) CANbitRateIx =  6;
  else if ( ! strcmp(br,   "10000")) CANbitRateIx =  7;      
  else     /* faled */               return  -1; 

  return CANbitRateIx;
}

uint32_t seDriver::getCANbitRate( uint32_t ix )
{
  uint32_t retval = (uint32_t)-1;
  uint32_t can_bitrates[] = { 1000000, 800000, 500000, 250000, 125000, 
                                                 50000, 20000, 10000 };
  
  if (ix < sizeof(can_bitrates)/sizeof(can_bitrates[0])) {
    retval = can_bitrates[ix];
  }
  
  return retval;   
}

void seDriver::str2upcase( char * s ) 
{
  for (uint32_t i = 0; s[i]!='\0'; i++) {
    if(s[i] >= 'a' && s[i] <= 'z') {
       s[i] = s[i] -32;
    }
  }
} 
    
void seDriver::getDataType( void ) 
{
  if ( m_DumpObj==NULL || m_DumpSize==0 ) return;
        
  for ( uint32_t i=0; i < m_DumpSize; i++ ) {
    if ( ReadWriteIndex == m_DumpObj[i].Addr.ADDR_b.INDEX &&
          ReadWriteSub == m_DumpObj[i].Addr.ADDR_b.SUB ) {
      ReadWriteDataType = m_DumpObj[i].DataType; 
      debug_print("ReadWriteDataType: %.4s (%.08xh)\n", (char*)&ReadWriteDataType, (uint32_t)ReadWriteDataType); 
      break; 
    }
  }
}
 
void seDriver::Read( void ) 
{
  try {
    uint32_t val = 0;
     
    if ( ReadWriteDataType == AU ) {
      getDataType();
    }
            
    switch (ReadWriteDataType)
    {
      case U32:
        val = (uint32_t)Wait(AsyncRead<uint32_t>(ReadWriteIndex, ReadWriteSub));
        printf("Node: %d, Index: %04xh, Sub: %02xh, U32=%.08xh\n", 
        id(), ReadWriteIndex, ReadWriteSub, (uint32_t)val);
        break;
        
      case U16:
        val = (uint16_t)Wait(AsyncRead<uint16_t>(ReadWriteIndex, ReadWriteSub));
        printf("Node: %d, Index: %04xh, Sub: %02xh, U16=    %.04xh\n", 
        id(), ReadWriteIndex, ReadWriteSub, (uint16_t)val);
        break;    
        
      case U8:
        val = (uint8_t)Wait(AsyncRead<uint8_t>(ReadWriteIndex, ReadWriteSub));
        printf("Node: %d, Index: %04xh, Sub: %02xh,  U8=      %.02xh\n", 
        id(), ReadWriteIndex, ReadWriteSub, (uint8_t)val);
        break; 
        
      case I16:
        val = /*(int16_t)*/Wait(AsyncRead<int16_t>(ReadWriteIndex, ReadWriteSub));
        printf("Node: %d, Index: %04xh, Sub: %02xh, I16=    %.04xh\n", 
        id(), ReadWriteIndex, ReadWriteSub, (uint16_t)val);
        break; 
        
      case I32:
        val = Wait(AsyncRead<int32_t>(ReadWriteIndex, ReadWriteSub));
        printf("Node: %d, Index: %04xh, Sub: %02xh, I16=%.08xh\n", 
        id(), ReadWriteIndex, ReadWriteSub, (uint32_t)val);
        break;         
           
      case VS:
        val = (uint32_t)Wait(AsyncRead<uint32_t>(ReadWriteIndex, ReadWriteSub));
        if (4 == strnlen((char*)&val, 4) ) {       
          printf("Node: %d, Index: %04xh, Sub: %02xh,  VS=    %.4s\n", 
          id(), ReadWriteIndex, ReadWriteSub, (char*)&val);
        } else {
          printf("Node: %d, Index: %04xh, Sub: %02xh, U32=%.08xh\n", 
          id(), ReadWriteIndex, ReadWriteSub, val);
        }
        break; 
        
      case AU:
        fprintf(stderr, "Node: %d, Error: Unexpected object address (Index: %04xh, Sub: %02xh)\n", id(), ReadWriteIndex, ReadWriteSub);
        break;
        
      default:
        fprintf(stderr, "Node: %d, Error: Unexpected DataType %.08xh\n", id(), (uint32_t)ReadWriteDataType);
    }
  } 
  
  catch (::std::error_code ec) {
    if (ec) throw ::std::system_error(ec, nullptr);
  }

}
        
void seDriver::Write( void ) 
{ 
  
  if ( ReadWriteDataType == AU ) {
    getDataType();
  }
    
  try {
    switch (ReadWriteDataType)
    {
      case VS:
      case U32:
        Wait(AsyncWrite<uint32_t>(ReadWriteIndex, ReadWriteSub, (uint32_t)ReadWriteVal));
        USleep(m_OtherTime); 
        break;
        
      case U16:
        Wait(AsyncWrite<uint16_t>(ReadWriteIndex, ReadWriteSub, (uint16_t)ReadWriteVal));
        USleep(m_OtherTime); 
        break;    
        
      case U8:
        Wait(AsyncWrite<uint8_t>(ReadWriteIndex, ReadWriteSub, (uint8_t)ReadWriteVal));
        USleep(m_OtherTime); 
        break; 
        
      case I16:
        Wait(AsyncWrite<int16_t>(ReadWriteIndex, ReadWriteSub, (int16_t)ReadWriteVal));
        USleep(m_OtherTime); 
        break;  
        
      case I32:
        Wait(AsyncWrite<int32_t>(ReadWriteIndex, ReadWriteSub, (int32_t)ReadWriteVal));
        USleep(m_OtherTime); 
        break; 

      case AU:
        fprintf(stderr, "Node: %d, Error: Unexpected object address (Index: %04xh, Sub: %02xh)\n", id(), ReadWriteIndex, ReadWriteSub);
        break;
                
      default:
        fprintf(stderr, "Node: %d, Error: Unexpected DataType %.08xh\n", id(), (uint32_t)ReadWriteDataType);
    }  
  }  
  
  catch (::std::error_code ec) {
    if (ec) throw ::std::system_error(ec, nullptr);
  }
}


void seDriver::DisableSyncProducer(void) 
{ 
  uint32_t sync = 0x00000080;
  debug_print("Node %d: Disabling SYNC producer\n", id());
  Wait(AsyncWrite<uint32_t>(0x1005, 0, (uint32_t)sync));
  USleep(m_OtherTime);
#ifndef NDEBUG    
  sync = Wait(AsyncRead<uint32_t>(0x1005, 0));
  debug_print("Node %d: SYNC message producer: %s (=0x%08x)\n", id(), (sync & 0x40000000) ? "enabled":"disabled", sync);   
#endif   
}

void seDriver::SetSyncProducer(void) 
{
  // Synchronous Counter Overflow Value
  // The host device can change the value of this OD only when the 
  // communication cycle period OD [1006h, 00h] is 0000 0000h.
  // *
  // When the host device sets 02h-F0h to this OD, the SYNC message 
  // transmitted by the unit (when operating as SYNC producer) has an 
  // optional counter. The SYNC producer increments the counter value 
  // by 1 every time it sends a SYNC message. When the counter value 
  // matches the maximum value defined by this OD, the counter resets 
  // to 1 at the next SYNC. The SYNC counter starts with a value of 1 
  // in the first SYNC message, which is transmitted when 1 is 
  // written to bit 30 of the SYNC Producer Enable OD [1005h, 00h].
  // The SYNC message has no optional counter when 00h is written to this OD.
  // *
  // 00h = SYNC message has no counter
  // 02h-F0h = overflow value
  if ( Sync == id() ) {
    if ( SyncOverflow != NO ) {
      Wait(AsyncWrite<uint32_t>(0x1006, 0, (uint32_t)0));
      USleep(m_OtherTime);        
      Wait(AsyncWrite<uint8_t>(0x1019, 0, (uint8_t)SyncOverflow));  
      USleep(m_OtherTime);      
    }               
         
    if ( SyncPeriod != NO ) {
      Wait(AsyncWrite<uint32_t>(0x1006, 0, (uint32_t)SyncPeriod*1000/*convert ms to us*/));
      USleep(m_OtherTime);       
    }  
   
#ifndef NDEBUG               
    SyncOverflow = (uint8_t)Wait(AsyncRead<uint8_t>(0x1019, 0));   
    debug_print("Node %d: Synchronous Counter Overflow Value is %u\n", id(), SyncOverflow);               
    SyncPeriod = Wait(AsyncRead<uint32_t>(0x1006, 0));
    SyncPeriod /= 1000; //< convert to ms
    debug_print("Node %d: SYNC cycle period is %u ms\n", id(), SyncPeriod);  
#endif
  }  
  
}

void seDriver::EnableSyncProducer(void) {
   
  uint32_t sync = 0x40000080;
  debug_print("Node %d: Enabling SYNC producer\n", id());
  Wait(AsyncWrite<uint32_t>(0x1005, 0, (uint32_t)sync));
  USleep(m_OtherTime);
  
#ifndef NDEBUG
  sync = Wait(AsyncRead<uint32_t>(0x1005, 0));
  debug_print("Node %d: SYNC message producer: %s (=0x%08x)\n", id(), (sync & 0x40000000) ? "enabled":"disabled", sync);   
#endif  
  printf("Node %d: SYNC producer: %u msec\n", id(), SyncPeriod); 
}

// This function gets called when the boot-up process of the slave completes.
// The 'st' parameter contains the last known NMT state of the slave
// (typically pre-operational), 'es' the error code (0 on success), and 'what'
// a description of the error, if any.
void seDriver::OnBoot(canopen::NmtState st, char es, const std::string& what) noexcept //override 
{     

  if (!es || es == 'L') {
    debug_print("Node %d: booted sucessfully [NMT state %d] [error code %d]\n", id(), (int)st, es);
  } else {
    fprintf(stderr, "Node: %d, [NMT state %d] [error code %c] failed to boot %s\n", id(), (int)st, es, what.c_str()); 
    fprintf(stderr, "Sending NMT RESET_COMM command to node %d\n", id());
    if ( openErrMsgFile() ) {
      fprintf(m_fpMsgFile, "Node: %d, [NMT state %d] [error code %c] failed to boot %s\n" , id(), (int)st, es, what.c_str()); 
      fprintf(m_fpMsgFile, "Sending NMT RESET_COMM command to node %d\n", id());
    }   
    
    master.Command(canopen::NmtCommand::RESET_COMM, id());
    return;
  }      
  
  if ( m_InitialBoot ) {    
    m_InitialBoot = false;
    
    if ( LogFile && m_fpLogFile==NULL) {
      char file_name[128];
      sprintf(file_name, "%.4s ID%d %s.csv", (char*) &m_DeviceName, id(), AppStartTime);
      
      if ((m_fpLogFile = fopen(file_name, "wb")) == NULL) {
        fprintf(stderr, "Error: opening log file");
      }               
    } 
    
    getModelFeatures();   
        
    /* Those objects access need PREOP mode for changes */
    if ( SampleRatePzs != NULL || NewNodeID != NO ||
         CANbitRateIx != NO || RestoreAllSettings || SaveParams ||
         TimeStamp != NO || Filter != 0 || ReadWrite == WRITE || TPDOMode != NO || 
         (Dump & 2)/*UDF*/ || UDFParam != NO || ApplyChanges != NO) {             
      
      /* 
       * Check if unit set to startup to OPER or PREOP mode.
       * It automatically go to OPER mode if bit2 = 0
       */
      if ( m_BootState == canopen::NmtState::START ) {      
        debug_print("Node %d: ENTER_PREOP is sent\n", id());
        master.Command(canopen::NmtCommand::ENTER_PREOP, id());
        USleep(m_ChangeModeTime); 
        m_BootState = canopen::NmtState::PREOP;
      }
          
      if ( NewNodeID != NO || CANbitRateIx != NO || RestoreAllSettings ) {
        
        if ( RestoreAllSettings ) { //< Restore Factory Default
          debug_print("Restore All Settings\n");      
          Wait(AsyncWrite<::std::string>(0x1011, 1, "load"));
          if ( (canopen::NmtCommand)m_NMT != canopen::NmtCommand::RESET_NODE || SaveParams == false ) {
            if ( (canopen::NmtCommand)m_NMT != canopen::NmtCommand::RESET_NODE && SaveParams == false ) {
              printme("Node %d: Loading OD with factory default values. The --save and --reset commands are required to make the changes permanent and valid.\n", id());    
            } else if ( ! SaveParams ) {
              printme("Node %d: Loading OD with factory default values. The --save command is required to make the changes permanent.\n", id()); 
            } else {
              printme("Node %d: Loading OD with factory default values. The --reset command is required to make the changes valid.\n", id()); 
            }   
          } else {
            printme("Node %d: Loading OD with factory default values\n", id());
          }        
          USleep(m_RestoreParamTime);                
        }        
        /* Change Node ID */
        if ( NewNodeID != NO ) { //< Change Node ID   
          debug_print("New Node ID=%u\n", NewNodeID);
          Wait(AsyncWrite<uint8_t>(0x2000, 1, (uint8_t)NewNodeID));        
          /* Need wait for 3 sec, save and then power cycle Node */
          if ( (canopen::NmtCommand)m_NMT != canopen::NmtCommand::RESET_NODE || SaveParams == false ) {
            if ( (canopen::NmtCommand)m_NMT != canopen::NmtCommand::RESET_NODE && SaveParams == false ) {
              printme("Node %d: ID changing to %d. The --save and --reset commands are required to make the changes permanent and valid.\n", id(), (uint8_t)NewNodeID);  
            } else if ( ! SaveParams ) {
              printme("Node %d: ID changing to %d. The --save command is required to make the changes permanent.\n", id(), (uint8_t)NewNodeID);    
            } else {
              printme("Node %d: ID changing to %d. The --reset command is required to make the changes valid.\n", id(), (uint8_t)NewNodeID);
            }
          } else {
            printme("Node %d: ID changing to %d\n", id(), (uint8_t)NewNodeID);  
          }      
          USleep(m_OtherTime); 
        }
        if ( CANbitRateIx != NO ) { //< Change CAN Bitrate 
          debug_print("Change CAN Bitrate =%u\n", CANbitRateIx);
          Wait(AsyncWrite<uint8_t>(0x2000, 2, (uint8_t)CANbitRateIx));
          /* Need to wait for 3 sec, save and then power cycle Node */
          if ( (canopen::NmtCommand)m_NMT != canopen::NmtCommand::RESET_NODE || SaveParams == false ) {
            if ( (canopen::NmtCommand)m_NMT != canopen::NmtCommand::RESET_NODE && SaveParams == false ) {
              printme("Node %d: Changing CAN Bitrate. The --save and --reset commands are required to make the changes permanent and valid.\n", id()); 
            } else if ( ! SaveParams ) {   
              printme("Node %d: Changing CAN Bitrate. The --save command is required to make the changes permanent.\n", id()); 
            } else {
              printme("Node %d: Changing CAN Bitrate. The --reset command is required to make the changes valid.\n", id()); 
            }
          } else {
            printme("Node %d: Changing CAN Bitrate. The bitrate of the can network must be changed as well.\n", id()); 
          }           
          USleep(m_OtherTime); 
        } 
 
      } else {   
                
        DisableTPDO();
            
        /* Change Transmission Type */
        if ( TPDOMode != NO ) {    
          /* Set Transmission Mode */
          debug_print("Setting Transmission Mode: %xh\n", TPDOMode);
          Wait(AsyncWrite<uint8_t>(0x1800, 2, (uint8_t)TPDOMode)); 
          USleep(m_OtherTime);             
        }                     
        
        /* Sample Rate */
        if (SampleRatePzs != NULL) {
          setSampleRate(SampleRatePzs);
        } 
        
        /* Filter Settings */
        if ( Filter != 0 ) {
          setFilter();
        }              
      
        /* Write Object */
        if (ReadWrite == WRITE) {
          Write();
        } 
           
        if ( ApplyChanges != NO ) {
          applyParam();
        }
        
        /* Time Message */
        if ( TimeStamp == 0 ) {         //< Single Shot TIME message
          debug_print("Sending TIME to node %d\n", id());
          ((seMaster *)&master)->SendTime();
          USleep(m_OtherTime);
        } else if ( TimeStamp != NO ) { //< Send TIME periodically
          debug_print("Start sending TIME to node %d\n", id());
          const ::std::chrono::milliseconds ms(TimeStamp);
          ((seMaster *)&master)->StartTime(ms);
          USleep(m_OtherTime);        
        }
              
        EnableTPDO();
      }
                       
      
      if ( SaveParams ) {        
        debug_print("Save Settings\n");
        const ::std::chrono::milliseconds timeout(300); //< otherwise 'G552' gets timed out (05040000)
        Wait(AsyncWrite<::std::string>(0x1010, 1, "save", timeout));
        if ( NewNodeID == NO && CANbitRateIx == NO && RestoreAllSettings==false && 
                    (canopen::NmtCommand)m_NMT != canopen::NmtCommand::RESET_NODE ) {
          printme("Node %d: Saving OD settings to non-volatile memory. The --reset command might be required to make the changes valid\n", id()); 
        } else {
          printme("Node %d: Saving OD settings to non-volatile memory\n", id()); 
        }
        USleep(m_SaveParamTime);
      }
      if ( Dump & 2) DumpUDF();
      /* end PREOP settings */
    } else if ( NMT==NO && MaxSample==0 && Sync==NO && ReadWrite==NO && Dump==0 && m_TPDOSettings == NO) {
      printf("Node %d: Manufacturer Device Name: 0x%.08x, '%.4s'\n", id(), (uint32_t)m_DeviceName, (char*) &m_DeviceName);
      printf("Node %d: Manufacturer Hardware Version: 0x%.08x, '%.4s'\n", id(), (uint32_t)m_HardwareVersion, (char*) &m_HardwareVersion);
      printf("Node %d: Manufacturer Software Version: 0x%.08x, '%.4s'\n", id(), (uint32_t)m_SoftwareVersion, (char*) &m_SoftwareVersion);  
      
      if ( m_BootState == canopen::NmtState::START ) {
        debug_print("Sending NMT STOP command to node %d\n", id());
        master.Command(canopen::NmtCommand::STOP, id());
      }
    } 
    
    /* Read Object */
    if (ReadWrite == READ){
      Read();      
    }
        
    if ( m_NMT != NO ) {       
      if ( (canopen::NmtCommand)m_NMT == canopen::NmtCommand::RESET_NODE ) {        
        printme("Node %d: Resetting...\n", id());         
      }
      master.Command((canopen::NmtCommand)m_NMT, id());
    }  
       
    /* first boot */
  } else { 
    /* reboot */
    printme("Node %d: The Reset Occurred.\n", id()); 
  }
  
  if ( NewNodeID != NO || CANbitRateIx != NO || RestoreAllSettings ) {
    quit();
  } else if ( (canopen::NmtCommand)m_NMT != canopen::NmtCommand::RESET_NODE ) { 
    /* Dump Objects' Values */
    if ( (Dump&1)/*REG*/ && MaxSample==0 ) ObjectDump();  
    if (MaxSample) {
      if (m_DeviceName==*(uint32_t*)"G552" || m_DeviceName==*(uint32_t*)"A552") {
        /* Get Unit variant Mode */
        m_UnitVar = Wait(AsyncRead<uint8_t>(0x2005, 0));
        if (m_UnitVar == 0x10) {
          m_unitModeStr = unit_AcceleromMode_M_A552AC1x;
        } else if (m_UnitVar == 0x20) {
          m_unitModeStr = unit_TiltAngleMode_M_A552AC1x;
        }
      }
      /* Operation mode checker */
      if ( m_NMT == NO && m_BootState != canopen::NmtState::START ) { 
        master.Command(canopen::NmtCommand::START, id());
        debug_print("Sending NMT START cammand to node %d\n", id());
        USleep(m_ChangeModeTime);
      } 
      
      getSampleTime();
      if (m_fpLogFile != NULL) startLogRecord();
      else if ((Dump&1)/*REG*/) ObjectDump(); 
      m_SampleProcessing = true;
      if ( Sync == id() ) { 
        DisableSyncProducer();
        SetSyncProducer();
        /* Start SYNC producer from slave */
        EnableSyncProducer();
      } else if ( Sync == MASTER_SYNC && !master[0x1006][0] && (++UnitBootCnt>=UnitCnt)) {
        printf("Master SYNC producer: %u msec\n", SyncPeriod); 
        /* Synchronous counter overflow */
        if ( SyncOverflow != NO ) master[0x1019][0] = (uint8_t)SyncOverflow;          
        /* Start SYNC producer from master */
        master[0x1006][0] = SyncPeriod*1000;                
      }
    } else if ( Sync == id() ) {   
      DisableSyncProducer();
      SetSyncProducer();
      /* Start SYNC producer from slave */
      EnableSyncProducer(); 
    } else if ( Sync == MASTER_SYNC && !master[0x1006][0]) { 
      printf("Master SYNC producer: %u msec\n", SyncPeriod);
      /* Synchronous counter overflow */
      if ( SyncOverflow != NO ) master[0x1019][0] = (uint8_t)SyncOverflow;       
      /* Start SYNC producer from master */
      master[0x1006][0] = SyncPeriod*1000; 
    } else {
      debug_print("Sending NMT STOP command to node %d\n", id());
      master.Command((canopen::NmtCommand)2, id());
      quit();
    }
  } else {
      m_NMT = NO;
  }
  
}

void seDriver::DumpUDF(void)
{
  if (m_DeviceName==*(uint32_t*)"A552") {
    uint16_t tap = (uint16_t)Wait(AsyncRead<uint16_t>(0x2008, 1));

    printme("Node: %d: UDF number of tap: %u\n", id(), tap);
    if ( tap ) {          
      /* Access the FIR coefficients in the RAM work area. */
      /* Set the index value in OD [2008h, 02]. */
      Wait(AsyncWrite<uint16_t>(0x2008, 2, 0));

      /* (For tap4: 0 to 3, for tap512: set in the range of 0 to 511) */
      for (uint16_t i=0; i<tap; i++) {
          m_DumpFIRCoeff[i] = (uint32_t)Wait(AsyncRead<uint32_t>(0x2008, 3));
          if (i%4 == 0) printf("\n");
          printme("%08xh ", m_DumpFIRCoeff[i]);
      }
      printme("\n\n");
      
    } 
  } else {
    fprintf(stderr, "Node: %d: UDF does not exist in %.4s\n", id(), (char*)&m_DeviceName);
  }
}

void seDriver::setUDFParam(void) 
{ 
  if ( UDFParam != NO && m_DeviceName == *(uint32_t*)"A552") {    
    
    switch (UDFParam)
    {
      case UDF_LOAD:
        /* Load from to the RAM work area data from the UDF valid area. */
        Wait(AsyncWrite<uint8_t>(0x2007, 0, 0x11));
        printf("Node: %d, UDF LOAD takes up to 15 sec...\n", id());
        break; 
              
      case UDF_SAVE:
        /* Save the RAM work area data to the UDF valid area. */
        Wait(AsyncWrite<uint8_t>(0x2007, 0, 0x21)); 
        printf("Node: %d, UDF SAVE takes up to 27 sec...\n", id());
        break;
        
      case UDF_ERASE:
        /* Erase the UDF valid area. */
        Wait(AsyncWrite<uint8_t>(0x2007, 0, 0x31));
        printf("Node: %d, UDF ERASE takes up to 15 sec...\n", id());
        break;  
        
      case UDF_VERIFY:
        /* Verify. */
        Wait(AsyncWrite<uint8_t>(0x2007, 0, 0x41));
        printf("Node: %d, UDF VERIFY takes up to 30 sec...\n", id());
        break;
        
      default:
        return;
    }
    
    uint8_t retval;
    for ( int i=0; i<35; i++ ) {
      USleep(1000000);
      retval = (uint8_t)Wait(AsyncRead<uint8_t>(0x2007, 0));
      if ((retval&0x0f)==0 || (retval&0x0f)==8) {
        break;
      } else {
        fprintf(stderr, "%i ", i+1);
      }
    }
    
    if ((retval&0x0f) == 0) {
      printf("\nNode: %d, UDF command successfully completed\n", id());
    } else {
      fprintf(stderr, "\nNode: %d, UDF command failed:%xh\n", id(), retval);
    }    
  }
}

void seDriver::applyParam(void) 
{ 
  if ( m_DumpObj==NULL || m_DumpSize==0 ) return;
        
  for ( uint32_t i=0; i < m_DumpSize; i++ ) {
    if ( m_DumpObj[i].Addr.ADDR_b.INDEX == 0x2005) { 
      if ( ApplyChanges == 0 ) {
        m_UnitVar = (uint8_t)Wait(AsyncRead<uint8_t>(0x2005, 0));
        USleep(m_OtherTime);
      
        /* Apply parameters */
        Wait(AsyncWrite<uint8_t>(0x2005, 0, m_UnitVar | 1));
      } else {
        /* Apply parameters */
        Wait(AsyncWrite<uint8_t>(0x2005, 0, ApplyChanges));        
      }
      
      debug_print("Node: %d, Apply Parameters takes 1 sec...\n", id());
      USleep(m_ApplyParamTime);   
    } 
  }
}

void seDriver::setFilter(void) 
{   
  uint8_t sps_ix = Wait(AsyncRead<uint8_t>(0x2001, 0));
  uint8_t c1 = (uint8_t)-1;


  if ( Filter == MA ) {
    for ( uint32_t i=0; i< m_FilterSetSize; i++ ) {
      if (m_FilterSet[i].FilterType == Filter) { 
        if ( m_FilterSet[i].EligibleSampleRateIx==sps_ix ) {   
          c1 = m_FilterSet[i].AI_Filter_Setting_Constant_1;
          break;
        }         
      } 
    }
  } else if ( (uint8_t)Filter == 'U' ) { /*UDF*/
    /*
     * Group Delay(msec) = (TapNumber-1)/(fsampling(kHz)*2, fsampling=4kHz
     * Group Delay(msec) = (TapNumber-1)/8
     */
    if ( m_DeviceName == *(uint32_t*)"A552") {
      for ( uint32_t i=0; i< m_FilterSetSize; i++ ) {
        if (m_FilterSet[i].FilterType == Filter) {  
#ifndef NDEBUG
          uint8_t timeInterval = m_FilterSet[i].EligibleSampleRateIx;
#endif            
          /* Configure the Tap Number of the UDF FIR coefficients. */
          /* Set the tap number in OD [2008h, 01] as an unsigned 16-bit integer. (Set from 4/64/128/512）*/
          debug_print("sps_ix:%u, timeInterval:%u\n", sps_ix, timeInterval)
          debug_print("Set AI filter tap: %u\n", m_FilterSet[i].tap)
          Wait(AsyncWrite<uint16_t>(0x2008, 1, (uint16_t)m_FilterSet[i].tap));
          USleep(m_OtherTime);

          /* Access the FIR coefficients in the RAM work area. */
          /* Set the [start] index value in OD [2008h, 02]. */
          Wait(AsyncWrite<uint16_t>(0x2008, 2, 0));
          USleep(m_OtherTime);

          if ( UDFFile ) {
            /* (For tap4: 0 to 3, for tap512: set in the range of 0 to 511) */
            for (uint16_t ii=0; ii<m_FilterSet[i].tap; ii++) {
              Wait(AsyncWrite<uint32_t>(0x2008, 3, (uint32_t)FIRCoeff[ii]));
              USleep(m_OtherTime);       
            }
          }
          
          if (UDFParam != NO) setUDFParam();
          if (UDFParam == UDF_ERASE) return;

          c1 = m_FilterSet[i].AI_Filter_Setting_Constant_1;    
          break;
        } 
      } 
    } else {
      fprintf(stderr, "Error: UDF not exist in %.4s\n", (char*)&m_DeviceName); 
      quit();
    }
  } else if ( (uint8_t)Filter == 'K' ) { /* KAISER */
    float sps_float = m_SampleRateFloat[sps_ix];
    for ( uint32_t i=0; i< m_FilterSetSize; i++ ) {
      if (m_FilterSet[i].FilterType == Filter) {
        uint8_t ixf = m_FilterSet[i].EligibleSampleRateIx;
        if ( sps_float >= m_SampleRateFloat[ixf] ) {          
          c1 = m_FilterSet[i].AI_Filter_Setting_Constant_1;
          break;
        }         
      } 
    }            
  } else if ( Filter < m_FilterSetSize ) {
      
    if (m_FilterSet[Filter].FilterType != NOT_VALID) {
      c1 = (uint8_t)Filter;
    }
      
  } else {
    fprintf(stderr, "Error: Unknown filter setting: %xh\n", Filter); 
    quit();    
  }
  
  if ( c1 == (uint8_t)-1 ) { 
    fprintf(stderr, "Error: Failed to set filter for %s sps\n", m_SampleRateStr[sps_ix]); 
    quit();    
  } else {
    /* Set AI filter tap constant OD[61A1h, 01]. */
    debug_print("Set AI filter tap constant: %u\n", c1)
    Wait(AsyncWrite<uint8_t>(0x61a1, 1, (unsigned char)c1)); 
    USleep(m_SaveParamTime);  

  }

}
 
// This function gets called during the boot-up process for the slave. The
// 'res' parameter is the function that MUST be invoked when the configuration
// is complete. Because this function runs as a task inside a coroutine, it
// can suspend itself and wait for an asynchronous function, such as an SDO
// request, to complete.
void seDriver::OnConfig(std::function<void(std::error_code ec)> res) noexcept //<< override 
{  
  try { 
    // Perform a few SDO write requests to configure the slave. The
    // AsyncWrite() function returns a future which becomes ready once the
    // request completes, and the Wait() function suspends the coroutine for
    // this task until the future is ready.

    if ( m_TPDOSettings != NO ) EnableTPDO();
    // This OD sets the NMT state after bootup of the sensor node. If bit 2 of this OD is 0,
    // the sensor node will go to operational state after bootup. There is 3 seconds
    // maximum interval from pre-operational mode to operational mode. 
    if ( ((uint32_t)1<<2) & (uint32_t)Wait(AsyncRead<uint32_t>(0x1F80, 0)) ) {
      m_BootState = NmtState::PREOP;
      debug_print("Boot NMT State: PREOP\n");
    } else {
      debug_print("Boot NMT State: START\n");
    }

    std::string tmpstr;  
    tmpstr = Wait(AsyncRead<::std::string>(0x1008, 0)); memcpy(&m_DeviceName,      tmpstr.c_str(), 4); 
    tmpstr = Wait(AsyncRead<::std::string>(0x1009, 0)); memcpy(&m_HardwareVersion, tmpstr.c_str(), 4);
    tmpstr = Wait(AsyncRead<::std::string>(0x100a, 0)); memcpy(&m_SoftwareVersion, tmpstr.c_str(), 4);
    debug_print("Node %d: Manufacturer Device Name: 0x%.08x, '%.4s'\n", id(), (uint32_t)m_DeviceName, (char*) &m_DeviceName);
    debug_print("Node %d: Manufacturer Hardware Version: 0x%.08x, '%.4s'\n", id(), (uint32_t)m_HardwareVersion, (char*) &m_HardwareVersion);
    debug_print("Node %d: Manufacturer Software Version: 0x%.08x, '%.4s'\n", id(), (uint32_t)m_SoftwareVersion, (char*) &m_SoftwareVersion);
    
    if ( m_DeviceName == *(uint32_t*)"A552" && (TPDOSettings&3) != 3) {
      fprintf(stderr, "Error: TPDO 1 and 2 cannot be disabled for A552 (Node %d)\n", id());
      quit();
    }
            
    // Configure the slave to produce a heartbeat every 'HeartBeatSlave' ms.
    if ( HeartBeatSlave != NO ) {
      Wait(AsyncWrite<uint16_t>(0x1017, 0, (uint16_t)HeartBeatSlave));
      USleep(m_OtherTime);
      debug_print("HeartBeatSlave: %d\n", (uint16_t)HeartBeatSlave);
    }

    // Configure the heartbeat consumer on the master. 
    if ( HeartBeatMaster != NO ) {
      const ::std::chrono::milliseconds ms((uint16_t)HeartBeatMaster);
      ConfigHeartbeat(ms);  
      debug_print("HeartBeatMaster: %d\n", (uint16_t)HeartBeatMaster);
    }

    // Report success (empty error code).
    res({});
  } catch (canopen::SdoError& e) {
    // If one of the SDO requests resulted in an error, abort the
    // configuration and report the error code.
    res(e.code());
  }
}

// This function is similar to OnConfg(), but it gets called by the
// AsyncDeconfig() method of the master.
void seDriver::OnDeconfig(std::function<void(std::error_code ec)> res) noexcept //<< override 
{
  try {     
    debug_print("Node %d, OnDeconfig...\n", id());
    res({});
  } catch (canopen::SdoError& e) {
    res(e.code());
  }
}
  
void seDriver::OnTime(const ::std::chrono::system_clock::time_point& abs_time) noexcept //< override  
{
  timespec time = ::lely::util::to_timespec(abs_time);
  printme("Node %d: OnTime: tv_sec:%ld, tv_nsec:%lu\n", id(), time.tv_sec, time.tv_nsec);   
  
  if ( openErrMsgFile() ) {
    fprintf(m_fpMsgFile, "Node %d: OnTime: tv_sec:%ld, tv_nsec:%lu\n", id(), time.tv_sec, time.tv_nsec);  
  }
}
// This function gets called every time a value is written to the local object
// dictionary of the master by an RPDO (or SDO, but that is unlikely for a
// master), *and* the object has a known mapping to an object on the slave for
// which this class is the driver. The 'idx' and 'subidx' parameters are the
// object index and sub-index of the object on the slave, not the local object
// dictionary of the master.
void seDriver::OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept //<override 
{ 
  if ( m_SampleProcessing ) {
    bool isData = prcOutput(idx, subidx);  
    
    if ( (m_SampleCounter > MaxSample) && (MaxSample != (uint32_t)-1) ) { 
      m_SampleProcessing = false;  
      if ( Sync != id() ) {
        debug_print("Sending NMT STOP command to node %d\n", id());
        master.Command((canopen::NmtCommand)2, id());
      }
      quit();        
    } else  if ( isData ) { 
      printme("%s", m_strtmp);
      
      if ( m_fpLogFile ) {
        fwrite( m_strlog, 1, strlen(m_strlog), m_fpLogFile); 
      }
    } 
  }   
}

void seDriver::OnHeartbeat(bool occurred) noexcept //<override
{  
  if ( openErrMsgFile() ) {
    fprintf(m_fpMsgFile, "Node: %d, Heartbeat time out occurred: %i \n", id(), (int)occurred); 
  }
}

bool seDriver::prcOutput(uint16_t idx, uint8_t subidx)
{
    if (idx == 0x7130) { 
      /* Gyro, Accl or Temperature */
      const char * unit[] = {"  ", "Gx", "Gy", "Gz", "Ax", "Ay", "Az", "T", "a1", "a2", "??"};      
      int16_t raw = (int16_t)rpdo_mapped[idx][subidx];
      
      if ( RawOutput ) {
        sprintf(m_strtmp, ",  %s:0x%04x", unit[subidx], (uint16_t)raw);
        if(m_fpLogFile) sprintf(m_strlog, ",0x%04x", (uint16_t)raw);
      } else {
        float val = raw;
        if (subidx == 8) {
          val *= m_AttiScale1; 
        } else if (subidx == 9) { 
          val *= m_AttiScale2;
        } else if (subidx == 10) { 
          val *=1.0f; //< reserverd
        } else if (subidx == 7) {      
          val -= 2634.0f;
          val *= -0.0037918f; 
          val += 25.0f;
        } else if (subidx > 3) { 
          val *= m_AccelScale;
        } else { 
          val *= m_GyroScale;
        }

        if (subidx == 7) {
          if (Fahrenheit) {
            float F = val*1.8000f + 32.00f;
            sprintf(m_strtmp, ",  °F:%+f", F);
            if(m_fpLogFile) sprintf(m_strlog,  ",%+f", F);
          } else {
            sprintf(m_strtmp, ",  ℃:%+f", val);          
            if(m_fpLogFile) sprintf(m_strlog, ",%+f", val);
          }
        } else {
          sprintf(m_strtmp, ",  %s:%+f", unit[subidx], val);          
          if(m_fpLogFile) sprintf(m_strlog, ",%+f", val);          
        }
      }
      return true;
      
    } else if (idx == 0x9130) {          
    /* M-A552AC1
     * Angle, Accl or Temperature
     * TPDO1 Ax and Ay or Ix and Iy
     * TPDO2 Az and Sc or Iz and Sc (counter)
     * TPDO3 Dy and Ms
     * TPDO4 Temperature
     * 
     * Acceleration:
     * The format is Q24 signed 32bit fixed point format: -128 ... 127.999 999 940, res:0.000 000 060
     * Unit: G
     * SF = 0.06 uG/LSB 
     * 
     * Tilt angle:
     * The tilt angle data format is Q29 signed 32bit fixed point format: -4 ... 3.999 999 998, res:0.000 000 002
     * Unit: radian
     * SF = 0.002 urad/LSB
     * 
     * Temperature (C/LSB):
     * T[℃]=SF*a+34.987, where SF = -0.0037918
     * Output range -30 ... 85 C
     */
       
      int32_t raw = (int32_t)rpdo_mapped[idx][subidx];
      
      if ( RawOutput ) {
        if (subidx == 4) { //< Temperature
          sprintf(m_strtmp, ",  %s:0x%08x", m_unitModeStr[subidx], (uint32_t)raw);         
          if(m_fpLogFile) sprintf(m_strlog, ",0x%08x", (uint32_t)raw); 
        } else if (subidx == 1){ //< Ax
          sprintf(m_strtmp_extra, ",  %s:0x%08x", m_unitModeStr[subidx], (uint32_t)raw);
          if(m_fpLogFile) sprintf(m_strlog_extra, ",0x%08x", (uint32_t)raw);    
          return false;  
        } else { //< Ay, Az
          char tmp[24];
          sprintf(tmp, ",  %s:0x%08x", m_unitModeStr[subidx], (uint32_t)raw);
          strcat(m_strtmp_extra, tmp);
          if(m_fpLogFile) {
            sprintf(tmp, ",0x%08x", (uint32_t)raw);    
            strcat(m_strlog_extra, tmp);
          }
          return false; 
        } 
  
      } else {
        float val = 0.0f;

        if (subidx == 4) {      //< Temperature
          val  = -0.0037918f;
          val *= raw; 
          val += 34.987f;
          
          if (Fahrenheit) {
            float F = val*1.8000f + 32.00f;
            sprintf(m_strtmp, ",  °F:%+f", F);
            if(m_fpLogFile) sprintf(m_strlog,  ",%+f", F);
          } else {
            sprintf(m_strtmp, ",  ℃:%+f", val);          
            if(m_fpLogFile) sprintf(m_strlog, ",%+f", val);
          }
                    
        } else if (m_UnitVar == 0x10) { //< Accl

          val = m_AccelScale;
          val *= raw;
      
          if (subidx == 1){
            sprintf(m_strtmp_extra, ",  %s:%+.8fh", m_unitModeStr[subidx], val); 
            if(m_fpLogFile) sprintf(m_strlog_extra, ",%+.8f", val);              
          } else {
            char tmp[24];
            sprintf(tmp, ",  %s:%+.8f", m_unitModeStr[subidx], val);
            strcat(m_strtmp_extra, tmp);
            if(m_fpLogFile) {
              sprintf(tmp, ",%+.8f", val);    
              strcat(m_strlog_extra, tmp);
            }
          }          
          return false;
        } else if (m_UnitVar == 0x20) { //< TiltAngle

          val = m_TiltAngleScale;
          val *= raw;
          
          if (subidx == 1){
            sprintf(m_strtmp_extra, ",  %s:%+.9f", m_unitModeStr[subidx], val); 
            if(m_fpLogFile) sprintf(m_strlog_extra, ",%+.9f", val);              
          } else {
            char tmp[24];
            sprintf(tmp, ",  %s:%+.9f", m_unitModeStr[subidx], val);
            strcat(m_strtmp_extra, tmp);
            if(m_fpLogFile) {
              sprintf(tmp, ",%+.9f", val);    
              strcat(m_strlog_extra, tmp);
            }
          }                   
          return false;
        } else if (m_UnitVar!=0x10 && m_UnitVar!=0x20) {        
          printf("Device %.4s ID%d: Configured as Accelerometer or Tilt angle sensor? (2005[0] set to %02xh)\n", (char*)&m_DeviceName, id(), m_UnitVar);
          quit();
        } else {
          /* unexpected */
          sprintf(m_strtmp, "Error: unexpected data: idx=0x%04x, subidx=0x%02x\n", idx, subidx);            
        }
      }
      return true;      
      
    } else if (idx==0x2101) {
     /* Time 
      * MSM Time Stamp: days (from 1/1/1984 in the Gregorian calendar）
      * Epoch timestamp: 441763200 (seconds from midnight 1/1/1970）
      * Timestamp in milliseconds: 441763200000
      * Date and time (GMT): Sunday, 1 January 1984 00:00:00
      * So, MSM Epoch Time = '441763200000' + 'MSM Time Stamp in ms'
      * MSM Time Stamp is [0x2101][1] is MSM days and [0x2101][2] is ms
      * MSM Epoch Time (ms) = 441763200000 + [0x2101][1]*86400 + [0x2101][2]  
      */   
      if ( subidx == 1 ) {
        /* days */
        m_Days = rpdo_mapped[0x2101][subidx];
      } else if ( subidx == 2 ) {
        /* milliseconds */
        uint32_t msec = rpdo_mapped[0x2101][subidx];     
        if ( RawOutput ) {
          sprintf(m_strtmp, ",  dy:0x%04x,  ms:0x%08x", m_Days, msec);
          if(m_fpLogFile) sprintf(m_strlog, ",0x%04x,0x%08x", m_Days, msec);
        } else {
          m_DaysInSec = m_Days;
          m_DaysInSec *= 86400;     //< 86400 seconds in 1 day
          m_DaysInSec += 441763200; //< differrence between 1984 and 1970 in sec          
          //msec >>= 4; /* bit31-4: milliseconds after 0:00am(midnight) */
          time_t rawtime = (time_t)(m_DaysInSec + msec/1000); /*in sec*/
          struct tm  ts;
          char       buf[80];
          ts = *localtime(&rawtime); 
          
          strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", &ts);          
          sprintf(m_strtmp, ",  %s.%03u", buf, msec%1000);
          if(m_fpLogFile) {
            sprintf(m_strlog, ",%u.%03u", m_DaysInSec + msec/1000, msec%1000);
          }
        }
        
        return true; 
      }

    } else if (idx == 0x2100) {
      /* Trigger counter */
      uint16_t val = rpdo_mapped[0x2100][subidx];
        
      if (m_exVal != val) {                                              
        sprintf(m_strtmp, "\nid:%i,  sw:%u,  hw:%u%s", id(), m_SampleCounter, val, m_strtmp_extra);
        if(m_fpLogFile) {
          if (m_SampleCounter) {
            m_SampleTime += m_SampleIncremTime*(float)(val - m_exVal);
            sprintf(m_strlog, "%s\n%u,%u,%.4f%s", m_comastr+m_comaix, m_SampleCounter, val, m_SampleTime, m_strlog_extra); 
          } else {
            sprintf(m_strlog, "\n%u,%u,%.4f%s", m_SampleCounter, val, m_SampleTime, m_strlog_extra); 
          }          
        }
        m_exVal = val;
        m_SampleCounter++;
        return true; 
      }  
    } else if (idx==0x2022) {
      uint16_t val = rpdo_mapped[0x2022][subidx];
      if ( subidx == 1 ) {
        sprintf(m_strtmp, ",  st:0x%04x", (uint16_t)val);         
        if(m_fpLogFile) sprintf(m_strlog, ",0x%04x", (uint16_t)val);       
      } else if ( subidx == 4 ) {
        /*reserved*/
        return false;
      } else {
        /* unexpected */
        sprintf(m_strtmp, "Error: unexpected data: idx=0x%04x, subidx=0x%02x\n", idx, subidx);  
      }
      return true;
    } else {
      /* unexpected */
      sprintf(m_strtmp, "Error: unexpected data: idx=0x%04x, subidx=0x%02x\n", idx, subidx);  
      return true;    
    }
    
    return false;
}

void seDriver::getSampleTime( void )
{  
  /* Get Transmission Mode */
  m_TransmissionMode = (uint8_t)Wait(AsyncRead<uint8_t>(0x1800, 2));
  
  
  if ( m_TransmissionMode == 0xFE ) { //< sample mode
    uint8_t sps_ix = Wait(AsyncRead<uint8_t>(0x2001, 0));
    float sps_float = m_SampleRateFloat[sps_ix];  
    m_SampleIncremTime = 1.0f/sps_float;
    debug_print("TransmissionMode: %xh, Sample Rate: %s\n", m_TransmissionMode, m_SampleRateStr[sps_ix]);    
    
  } else if ( SyncPeriod == NO ) { //<< Not Set!
    m_SampleIncremTime = 0.0f;
    printf("Warning! Sync Period not set! Is there Sync Producer?\n");
  } else if ( m_TransmissionMode ) { //< sync mode    
    m_SampleIncremTime = SyncPeriod * m_TransmissionMode/1000.0f;
  } else { //< sync mode
    m_SampleIncremTime = SyncPeriod/1000.0f;
    debug_print("SyncPeriod: %u\n", SyncPeriod);
  }
  debug_print("m_SampleIncremTime = %.4f\n", m_SampleIncremTime);
  
}

void seDriver::SetTPDO( uint8_t val )  
{
  if ( TPDOSettings == NO )
    TPDOSettings = val;
  else
    TPDOSettings |= val;
}

void seDriver::EnableTPDO( void )
{
  debug_print("Node %d, m_TPDOSettings: 0x%x  0x%x\n", id(), m_TPDOSettings, TPDOSettings);
  uint32_t set_tpdo = ~m_TPDOSettings;
  uint32_t tpdo_1 = (((set_tpdo>>0)&1)<<31);
  uint32_t tpdo_2 = (((set_tpdo>>1)&1)<<31);
  uint32_t tpdo_3 = (((set_tpdo>>2)&1)<<31);
  uint32_t tpdo_4 = (((set_tpdo>>3)&1)<<31);    

  debug_print("tpdo_1=%d\n", !tpdo_1);
  debug_print("tpdo_2=%d\n", !tpdo_2);
  debug_print("tpdo_3=%d\n", !tpdo_3);
  debug_print("tpdo_4=%d\n", !tpdo_4);    

  Wait(AsyncWrite<uint32_t>(0x1800, 1, tpdo_1+0x40000180+id())); //< enable TPDO1 
  USleep(m_OtherTime);
  Wait(AsyncWrite<uint32_t>(0x1801, 1, tpdo_2+0x40000280+id())); //< enable TPDO2
  USleep(m_OtherTime);
  Wait(AsyncWrite<uint32_t>(0x1802, 1, tpdo_3+0x40000380+id())); //< enable TPDO3 
  USleep(m_OtherTime);     
  Wait(AsyncWrite<uint32_t>(0x1803, 1, tpdo_4+0x40000480+id())); //< enable TPDO4 
  USleep(m_OtherTime); 
     
}

void seDriver::DisableTPDO( void )
{
  if ( m_TPDOSettings == NO ) {
    m_TPDOSettings = 0;
    if ( ((uint32_t)0x80000000 & Wait(AsyncRead<uint32_t>(0x1800, 1))) == 0 ) {
      m_TPDOSettings |= 1<<0;
    }
    if ( ((uint32_t)0x80000000 & Wait(AsyncRead<uint32_t>(0x1801, 1))) == 0 ) {
      m_TPDOSettings |= 1<<1;
    }
    if ( ((uint32_t)0x80000000 & Wait(AsyncRead<uint32_t>(0x1802, 1))) == 0 ) {
      m_TPDOSettings |= 1<<2;
    }
    if ( ((uint32_t)0x80000000 & Wait(AsyncRead<uint32_t>(0x1803, 1))) == 0 ) {
      m_TPDOSettings |= 1<<3;
    }    
  }
  debug_print("m_TPDOSettings: 0x%x\n", m_TPDOSettings);
  Wait(AsyncWrite<uint32_t>(0x1800, 1, 0xC0000180+id())); //< disable TPDO1 
  USleep(m_OtherTime);
  Wait(AsyncWrite<uint32_t>(0x1801, 1, 0xC0000280+id())); //< disable TPDO2
  USleep(m_OtherTime);
  Wait(AsyncWrite<uint32_t>(0x1802, 1, 0xC0000380+id())); //< disable TPDO3    
  USleep(m_OtherTime);     
  Wait(AsyncWrite<uint32_t>(0x1803, 1, 0xC0000480+id())); //< disable TPDO4    
  USleep(m_OtherTime); 
 
} 

void seDriver::OnState(canopen::NmtState st) noexcept //< override
{
  if (st == canopen::NmtState::BOOTUP) {
    debug_print( "+++++ BOOTUP +++++\n");
  } else if (st == canopen::NmtState::STOP) {
    debug_print( "+++++ STOP +++++\n");            
  } else if (st == canopen::NmtState::START) {
    debug_print( "+++++ START +++++\n");
  } else if (st == canopen::NmtState::RESET_NODE) {
    debug_print( "+++++ RESET_NODE +++++\n");
  } else if (st == canopen::NmtState::RESET_COMM) {
    debug_print( "+++++ RESET_COMM +++++\n");      
  } else if (st == canopen::NmtState::PREOP) {
    debug_print( "+++++ PREOP +++++\n");     
  } else if (st == canopen::NmtState::TOGGLE) {
    debug_print( "+++++ TOGGLE +++++\n");
  } else {
    debug_print( "=0x%x !!!!!\n", (int)st);
  }
}

void seDriver::OnCanState(io::CanState new_state,
                io::CanState old_state) noexcept //< override
{
  printme( "Node %d: OnCanState: new_state=%d, old_state=%d\n", id(), (int)new_state, (int)old_state );
  
  if ( openErrMsgFile() ) {
    fprintf(m_fpMsgFile, "Node %d: OnCanState: new_state=%d, old_state=%d\n", id(), (int)new_state, (int)old_state );  
  }
}

void seDriver::ObjectDump( void ) 
{
  uint32_t val;
  
  if ( m_DumpObj==NULL || m_DumpSize==0 ) return;
  if (  m_fpLogFile && MaxSample ) {
    fprintf(m_fpLogFile, "\n%s\nObject Value Dump:%s\n", m_comastr, m_comastr);   
  }
        
  for ( uint32_t i=0; i < m_DumpSize; i++ ) {
    
    switch (m_DumpObj[i].DataType)
    {
      case U32:
        val = Wait(AsyncRead<uint32_t>(m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB));
        if ( m_fpLogFile && MaxSample ) {
          fprintf(m_fpLogFile, "Index:, %04xh, Sub:, %02xh, U32=,%.08xh, :: ,%s%s\n", 
            m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, val, m_DumpObj[i].ObjectName, m_comastr+7);
        }        
        sprintf(m_strtmp, "Index: %04xh, Sub: %02xh, U32=%.08xh :: %s\n", 
          m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, val, m_DumpObj[i].ObjectName);
        break;
        
      case U16:
        val = Wait(AsyncRead<uint16_t>(m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB));
        if ( m_fpLogFile && MaxSample ) {
          fprintf(m_fpLogFile, "Index:, %04xh, Sub:, %02xh, U16=,    %.04xh, :: ,%s%s\n",
            m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, val, m_DumpObj[i].ObjectName, m_comastr+7);
        }        
        sprintf(m_strtmp, "Index: %04xh, Sub: %02xh, U16=    %.04xh :: %s\n", 
          m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, (uint16_t)val, m_DumpObj[i].ObjectName);
        break;    
        
      case U8:
        val = Wait(AsyncRead<uint8_t>(m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB));
        if ( m_fpLogFile && MaxSample ) {
          fprintf(m_fpLogFile, "Index:, %04xh, Sub:, %02xh,  U8=,      %.02xh, :: ,%s%s\n",
            m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, val, m_DumpObj[i].ObjectName, m_comastr+7);
        }        
        sprintf(m_strtmp, "Index: %04xh, Sub: %02xh,  U8=      %.02xh :: %s\n", 
          m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, (uint8_t)val, m_DumpObj[i].ObjectName);
        break; 
        
      case I16:
        val = Wait(AsyncRead<int16_t>(m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB));       
        if ( m_fpLogFile && MaxSample ) {
          fprintf(m_fpLogFile, "Index:, %04xh, Sub:, %02xh, I16=,    %.04xh, :: ,%s%s\n", 
            m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, (uint16_t)val, m_DumpObj[i].ObjectName, m_comastr+7);
        }  
        sprintf(m_strtmp, "Index: %04xh, Sub: %02xh, I16=    %.04xh :: %s\n", 
          m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, (uint16_t)val, m_DumpObj[i].ObjectName); 
        break; 
        
      case I32:
        val = Wait(AsyncRead<int32_t>(m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB));       
        if ( m_fpLogFile && MaxSample ) {
          fprintf(m_fpLogFile, "Index:, %04xh, Sub:, %02xh, I32=,%.08xh, :: ,%s%s\n", 
            m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, (uint32_t)val, m_DumpObj[i].ObjectName, m_comastr+7);
        }  
        sprintf(m_strtmp, "Index: %04xh, Sub: %02xh, I32=%.08xh :: %s\n", 
          m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, (uint32_t)val, m_DumpObj[i].ObjectName); 
        break;         
           
      case VS:
        val = Wait(AsyncRead<uint32_t>(m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB));
        if (4 == strnlen((char*)&val, 4) ) {   
          if ( m_fpLogFile && MaxSample ) {
            fprintf(m_fpLogFile, "Index:, %04xh, Sub:, %02xh,  VS=,     %.4s, :: ,%s%s\n", 
              m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, (char*)&val, m_DumpObj[i].ObjectName, m_comastr+7);
          } 
          sprintf(m_strtmp, "Index: %04xh, Sub: %02xh,  VS=    %.4s  :: %s\n", 
            m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, (char*)&val, m_DumpObj[i].ObjectName);
        } else {
          if ( m_fpLogFile && MaxSample ) {
            fprintf(m_fpLogFile, "Index:, %04xh, Sub:, %02xh, U32=,%.08xh, :: ,%s%s\n",
              m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, val, m_DumpObj[i].ObjectName, m_comastr+7);                     
          }   
          sprintf(m_strtmp, "Index: %04xh, Sub: %02xh, U32=%.08xh :: %s\n", 
            m_DumpObj[i].Addr.ADDR_b.INDEX, m_DumpObj[i].Addr.ADDR_b.SUB, val, m_DumpObj[i].ObjectName);   

        }
        break;            
        
      default:
        sprintf(m_strtmp, "!!! Unexpected DataType %d\n", (int)m_DumpObj[i].DataType);
    }
    
    if ( m_fpLogFile && MaxSample==0 ) fprintf(m_fpLogFile, "%s", m_strtmp); 
    printme("%s", m_strtmp);

  }
  
  if ( m_fpLogFile && MaxSample ) {  
    fprintf(m_fpLogFile, "%s\n", m_comastr);
  }    
  
}
    
void seDriver::startLogRecord( void )
{
  enum { VAR_NORMAL, VAR_6DOF=0x10 /*temp*/, VAR_ATTI=0x20 /*angles*/ };
  
  if ( m_TPDOSettings == NO ) {
    m_TPDOSettings = 0;
    if ( ((uint32_t)0x80000000 & Wait(AsyncRead<uint32_t>(0x1800, 1))) == 0 ) {
      m_TPDOSettings |= 1<<0;
    }
    if ( ((uint32_t)0x80000000 & Wait(AsyncRead<uint32_t>(0x1801, 1))) == 0 ) {
      m_TPDOSettings |= 1<<1;
    }
    if ( ((uint32_t)0x80000000 & Wait(AsyncRead<uint32_t>(0x1802, 1))) == 0 ) {
      m_TPDOSettings |= 1<<2;
    }
    if ( ((uint32_t)0x80000000 & Wait(AsyncRead<uint32_t>(0x1803, 1))) == 0 ) {
      m_TPDOSettings |= 1<<3;
    }    
  }

  // tpdo settings
  uint32_t tpdo_1 = !!(m_TPDOSettings&(1<<0));
  uint32_t tpdo_2 = !!(m_TPDOSettings&(1<<1));
  uint32_t tpdo_3 = !!(m_TPDOSettings&(1<<2));
  uint32_t tpdo_4 = !!(m_TPDOSettings&(1<<3));  
   
  memset(m_comastr, 0, 16);
  if ( m_DeviceName == *(uint32_t*)"A552" ) {
    /* TPDO 1 and 2 always enabled for M-A552AC1x */
    m_comacnt = 9; /*min value, no last coma*/;
    m_comaix  = 1 + tpdo_1*2 + tpdo_2*2 + tpdo_3*2 + tpdo_4 - 1; 
    if (RawOutput && tpdo_3) {
      m_comacnt++;
      m_comaix++;
    }    
  } else {
    if ( m_UnitVar == VAR_6DOF ) {
      m_comacnt = 3+3+3+2+1-1/*no last coma*/;
      m_comaix  = 3 + tpdo_1*3 + tpdo_2*3 + tpdo_3*2 + tpdo_4 - 1;
    } else if ( m_UnitVar == VAR_ATTI ) {    
      m_comacnt = 3+3+3+3+1-1/*no last coma*/;
      m_comaix  = 3 + tpdo_1*3 + tpdo_2*3 + tpdo_3*3 + tpdo_4 - 1;
    } else {
      m_comacnt = 3+3+3+1+1-1/*no last coma*/;
      m_comaix  = 3 + tpdo_1*3 + tpdo_2*3 + tpdo_3 + tpdo_4 - 1;
    }
    if (RawOutput && tpdo_4) {
      m_comacnt++;
      m_comaix++;
    }
  }
 
  debug_print("var: 0x%02x, m_comacnt: %u, m_comaix: %u\n", m_UnitVar, m_comacnt, m_comaix);
  memset(m_comastr, ',', m_comacnt);

  // date and time 
  time_t logstart_time;
  struct tm  timeinfo;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  logstart_time = (time_t)tv.tv_sec;
  timeinfo = *localtime((time_t*) &logstart_time);
  strftime (m_strlog, sizeof(m_strlog), "date:,%Y-%m-%d", &timeinfo); 
  fprintf(m_fpLogFile, "%s", m_strlog); 
  strftime (m_strlog, sizeof(m_strlog), ",start_time:,%H:%M:%S", &timeinfo);
  fprintf(m_fpLogFile, "%s", m_strlog); 
  fprintf(m_fpLogFile, "%lu", tv.tv_usec/1000); 

  // version
  fprintf(m_fpLogFile, ",co-master Version:,%s%s", COMASTER_VERSION, m_comastr+5);  

  // product id  
  fprintf(m_fpLogFile, "\nPROD_ID:,%.4s,HW_VERSION:,%.4s,SW_VERSION:,%.4s%s", 
                        (char*) &m_DeviceName, (char*) &m_HardwareVersion, 
                        (char*) &m_SoftwareVersion, m_comastr+5);
                        
  // sample rate and filter settings
  uint8_t sps_ix = Wait(AsyncRead<uint8_t>(0x2001, 0));
  uint8_t c1 = Wait(AsyncRead<uint8_t>(0x61a1, 1));

  char FiltrStr[16] = {'u','n','k','n','o','w','n',0};
  uint16_t tap = 0;
  char FreqCutStr[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  char FreqCutComa = 0;
 
  for ( uint32_t i=0; i< m_FilterSetSize; i++ ) {
    if (m_FilterSet[i].AI_Filter_Setting_Constant_1 == c1) { 
      
      if ( *(char*)&(m_FilterSet[i].FilterType) == 'M' ) {
        strcpy(FiltrStr, "Moving-Average");
      } else if (*(char*)&(m_FilterSet[i].FilterType) == 'K' ) {
        strcpy(FiltrStr, "Kaiser");
      } else if (*(char*)&(m_FilterSet[i].FilterType) == 'U' ) {
        strcpy(FiltrStr, "User-Defined");
      } 

      tap = m_FilterSet[i].tap;  
      if ( m_FilterSet[i].FreqCut != 0 ) {
        sprintf(FreqCutStr, ",fc[Hz]:,%u", m_FilterSet[i].FreqCut);
        FreqCutComa = 2;
      }
      break;    
    } 
  }  

  if ( m_TransmissionMode == 0xFE ) {
    fprintf(m_fpLogFile, "\nCAN Channel:,%s,Rate[Sps]:,%s,FilterType:,%s,TAP:,%d%s%s", 
                            can_ch, m_SampleRateStr[sps_ix], FiltrStr, tap, FreqCutStr, m_comastr+7+FreqCutComa); 
    fprintf(m_fpLogFile, "\n%s", m_comastr); 
  } else if ( Sync == MASTER_SYNC) {
    fprintf(m_fpLogFile, "\nCAN Channel:,%s,MasterSync[msec]:,%u,FilterType:,%s,TAP:,%d%s%s", 
                            can_ch, SyncPeriod, FiltrStr, tap, FreqCutStr, m_comastr+7+FreqCutComa);   
    fprintf(m_fpLogFile, "\nTransmission Mode:,%d%s", m_TransmissionMode, m_comastr+1); 
  } else if ( Sync < 128) {
    fprintf(m_fpLogFile, "\nCAN Channel:,%s,Slave[%u]Sync[msec]:,%u,FilterType:,%s,TAP:,%d%s%s", 
                            can_ch, Sync, SyncPeriod, FiltrStr, tap, FreqCutStr, m_comastr+7+FreqCutComa);  
    fprintf(m_fpLogFile, "\nTransmission Mode:,%d%s", m_TransmissionMode, m_comastr+1); 
  } else {
    /* unexpected */
    fprintf(m_fpLogFile, "\nCAN Channel:,%s,SyncTime[sec]:,%.4f,FilterType:,%s,TAP:,%d%s%s", 
                            can_ch, m_SampleIncremTime/1000.0f, FiltrStr, tap, FreqCutStr, m_comastr+7+FreqCutComa);
    fprintf(m_fpLogFile, "\n%s", m_comastr); 
  }
  fprintf(m_fpLogFile, "\nComment:%s\n", m_comastr);  
    
  if ((Dump&2) && FiltrStr[0] == 'U') {   
    fprintf(m_fpLogFile, "\n\n%s\nUser Defined Filter Coefficients:%s\n", m_comastr, m_comastr); 
    /* (For tap4: 0 to 3, for tap512: set in the range of 0 to 511) */
    fprintf(m_fpLogFile, "%08xh,", m_DumpFIRCoeff[0]);
    for (uint16_t i=1; i<tap; i++) {
      if ( i%4 ) {
        fprintf(m_fpLogFile, " %08xh,", m_DumpFIRCoeff[i]);
      } else {
        fprintf(m_fpLogFile, "%s\n%08xh,", m_comastr+4, m_DumpFIRCoeff[i]);
      } 
    }
    if ( tap )
      fprintf(m_fpLogFile, "%s\n\n", m_comastr+4);    
  }
  
  if ( Dump & 1 ) {   
    ObjectDump();  
  }
 
  if (MaxSample) {
    
    if (m_DeviceName == *(uint32_t*)"A552") {
      /*
       * Angle, Accl or Temperature
       * TPDO1 Ax and Ay or Ix and Iy
       * TPDO2 Az and Sc or Iz and Sc (counter)
       * TPDO3 Dy and Ms
       * TPDO4 Temperature
       */
      
      /* TPDO 1 and 2 always enabled for M-A552AC1x */
      fprintf(m_fpLogFile, "%s", "\n\nSample No.,Trigger Cnt,time[sec]");        
              
      if ( tpdo_1 ) {
        if (RawOutput) {
          fprintf(m_fpLogFile, ",%s,%s", m_unitModeStr[1], m_unitModeStr[2]);
        } else if (m_UnitVar==0x10 /*Accel*/) {
          fprintf(m_fpLogFile, ",%s[G],%s[G]", m_unitModeStr[1], m_unitModeStr[2]);
        } else if (m_UnitVar==0x20 /*TiltAngle*/) {
          fprintf(m_fpLogFile, ",%s[rad],%s[rad]", m_unitModeStr[1], m_unitModeStr[2]);
        }          
      }
      if ( tpdo_2 ) {
        if (RawOutput) {
          fprintf(m_fpLogFile, ",%s", m_unitModeStr[3]);  
        } else if (m_UnitVar==0x10 /*Accel*/) {
          fprintf(m_fpLogFile, ",%s[G]", m_unitModeStr[3]);
        } else if (m_UnitVar==0x20 /*TiltAngle*/) {
          fprintf(m_fpLogFile, ",%s[rad]", m_unitModeStr[3]);
        }  
      } 
      if ( tpdo_3 ) {
        if (RawOutput) fprintf(m_fpLogFile, "%s", ",days,ms");
        else           fprintf(m_fpLogFile, "%s", ",Ts[SS.ms]");
      }
      if ( tpdo_4 ) {
        if (RawOutput) {
          fprintf(m_fpLogFile, "%s", ",temperature"); 
        } else if (Fahrenheit) {
          fprintf(m_fpLogFile, "%s", ",T[deg.F]"); 
        } else {
          fprintf(m_fpLogFile, "%s", ",T[deg.C]");
        }
      }        
            
    } else {
      if (tpdo_1 || tpdo_2 || tpdo_3 || tpdo_4) {
        fprintf(m_fpLogFile, "%s", "\n\nSample No.,Trigger Cnt,time[sec]");     
      }
      if ( tpdo_1 ) {
        if (RawOutput) fprintf(m_fpLogFile, "%s", ",GyroX,GyroY,GyroZ"); 
        else           fprintf(m_fpLogFile, "%s", ",Gx[dps],Gy[dps],Gz[dps]"); 
      }
      if ( tpdo_2 ) {
        if (RawOutput) fprintf(m_fpLogFile, "%s", ",AcclX,AcclY,AcclZ");  
        else           fprintf(m_fpLogFile, "%s", ",Ax[mG],Ay[mG],Az[mG]");    
      }     
      if ( tpdo_3 ) {
        switch(m_UnitVar)
        {
          default:
          case VAR_NORMAL:
            if (RawOutput) {
              fprintf(m_fpLogFile, "%s", ",temperature"); 
            } else if (Fahrenheit) {
              fprintf(m_fpLogFile, "%s", ",T[deg.F]"); 
            } else {
              fprintf(m_fpLogFile, "%s", ",T[deg.C]");
            }
            break;
            
          case VAR_6DOF:
            if (RawOutput) {
              fprintf(m_fpLogFile, "%s", ",temperature,status"); 
            } else if (Fahrenheit) {            
              fprintf(m_fpLogFile, "%s", ",T[deg.F],st[err]"); 
            } else {
              fprintf(m_fpLogFile, "%s",",T[deg.C],st[err]");
            }
            break;
            
          case VAR_ATTI:
            if (RawOutput) fprintf(m_fpLogFile, "%s", ",a1(Roll),a2(Pitch),status");
            else           fprintf(m_fpLogFile, "%s", ",a1(Roll)[rad],a2(Pitch)[rad],st[err]");
            break;
        }            
      } 
      if ( tpdo_4 ) {
        if (RawOutput) fprintf(m_fpLogFile, "%s", ",days,ms");
        else           fprintf(m_fpLogFile, "%s", ",Ts[SS.ms]");
      }
    } 
    fprintf(m_fpLogFile, "%s", m_comastr+m_comaix); 
  }                    
}

void seDriver::OnCommand(canopen::NmtCommand cs) noexcept //<override 
{
  if (cs == canopen::NmtCommand::START) {      
    debug_print( "--NmtCommand::START, id=%d\n", id());           
  } else if (cs == canopen::NmtCommand::STOP) {
    debug_print( "--NmtCommand::STOP, id=%d\n", id());
  } else if (cs == canopen::NmtCommand::ENTER_PREOP) {
    debug_print( "--NmtCommand::ENTER_PREOP, id=%d\n", id()); 
  } else if (cs == canopen::NmtCommand::RESET_NODE) {
    debug_print( "--NmtCommand::RESET_NODE, id=%d\n", id());
  } else if (cs == canopen::NmtCommand::RESET_COMM) {
    debug_print( "--NmtCommand::RESET_COMM, id=%d\n", id());
  } else {
    debug_print( "--NmtCommand::0x%x, id=%d\n", (int)cs, id());
  }
}

bool seDriver::openErrMsgFile( void )
{
  bool retval = false;
  
  if ( m_fpMsgFile==NULL) {
    char file_name[128];
    sprintf(file_name, "%.4s ID%d %s.msg", (char*) &m_DeviceName, id(), AppStartTime);
    
    if ((m_fpMsgFile = fopen(file_name, "wb")) == NULL) {
      fprintf(stderr, "Error: opening error message file");
    } else {
      retval = true;
    }              
  } else {
    retval = true;
  }  
  
  return retval;
}

void seDriver::getModelFeatures( void )
{
    
  switch (m_DeviceName)
  {
    default:
    case 0x54353547: //< "G55T"
      m_GyroScale        = 0.008f; //< [dps/LSB]
      m_AccelScale       = 0.2f;   //< [ mG/LSB]
      m_DumpSize         = SensUnit_M_G550PC2x_sz;
      m_DumpObj          = SensUnit_M_G550PC2x;      
      m_SampleRateFloat  = fSampleRates_M_G550PC2x;
      m_SampleRateStr    =  SampleRates_M_G550PC2x;  
      m_FilterSet        = FilterSet_M_G550PC2x; 
      m_FilterSetSize    = FilterSet_M_G550PC2x_sz;
      m_ChangeModeTime   = 3000000; //< 3 sec
      m_SaveParamTime    = 1030000; //< 30 msec + 1 sec before reset      
      m_RestoreParamTime = 1000000;
      m_OtherTime        =    1000; //< 1 msec  
         
      break;
      
    case 0x32353547: //< "G552"
      switch ( m_HardwareVersion )
      {
        case 0x30314350: //< "PC10"
          m_GyroScale        = 0.01515f;    //< [dps/LSB]
          m_AccelScale       = 0.4f;        //< [ mG/LSB]
          m_AttiScale1       = 0.00012207f; //< [rad/LSB] (Roll)
          m_AttiScale2       = 0.00699411f; //< [rad/LSB] (Pitch)
          m_DumpSize         = SensUnit_M_G552PC1x_sz;
          m_DumpObj          = SensUnit_M_G552PC1x;                    
          // Same as G550:
          m_SampleRateFloat  = fSampleRates_M_G550PC2x;
          m_SampleRateStr    =  SampleRates_M_G550PC2x;  
          m_FilterSet        = FilterSet_M_G550PC2x; 
          m_FilterSetSize    = FilterSet_M_G550PC2x_sz; 
          m_ChangeModeTime   = 3000000; //< 3 sec 
          m_SaveParamTime    = 1200000; //< 0.2 sec for exec + 1 sec before reset          
          m_RestoreParamTime = 1100000; //< 0.1 sec for exec + 1 sec before reset
          m_ApplyParamTime   = 1000000; //< 1 sec        
          m_OtherTime        =    1000; //< 1 msec             
          break;
      
        case 0x30374350: //< "PC70"
          m_GyroScale        = 0.01515f; //< [dps/LSB] 
          m_AccelScale       = 0.4f;     //< [ mG/LSB]
          m_DumpSize         = SensUnit_M_G552PC7x_sz;
          m_DumpObj          = SensUnit_M_G552PC7x;          
          m_SampleRateFloat  = fSampleRates_M_G552PC7x;
          m_SampleRateStr    =  SampleRates_M_G552PC7x;  
          m_FilterSet        = FilterSet_M_G552PC7x; 
          m_FilterSetSize    = FilterSet_M_G552PC7x_sz;
          m_ChangeModeTime   = 3000000; //< 3 sec
          m_SaveParamTime    = 1200000; //< 0.2 sec for exec + 1 sec before reset          
          m_RestoreParamTime = 1100000; //< 0.1 sec for exec + 1 sec before reset
          m_ApplyParamTime   = 1000000; //< 1 sec (reserved)        
          m_OtherTime        =    1000; //< 1 msec 
          break;
      }
      break;
      
    case 0x32353541: //< "A552"
      m_AccelScale       = 0.00000006f; //< [ G/LSB]   
      m_TiltAngleScale   = 0.000000002f;
      m_DumpSize         = SensUnit_M_A552AC1x_sz;
      m_DumpObj          = SensUnit_M_A552AC1x;      
      m_SampleRateFloat  = fSampleRates_M_A552AC1x;
      m_SampleRateStr    =  SampleRates_M_A552AC1x;  
      m_FilterSet        = FilterSet_M_A552AC1x; 
      m_FilterSetSize    = FilterSet_M_A552AC1x_sz;
      m_ChangeModeTime   = 5000000; //< 5 sec
      m_SaveParamTime    =  400000; //< 0.2 sec for exec + 0.2 sec before reset 
      m_RestoreParamTime = 1100000; //< 0.1 sec for exec + 1 sec before reset
      m_ApplyParamTime   = 1000000; //< 1 sec
      m_OtherTime        =    1000; //< 1 msec 
      
      break;      
  }
}

void seDriver::quit( void ) 
{   
  debug_print("UnitCnt: %u\n", UnitCnt);
  if ( UnitCnt ) UnitCnt--;
  if ( UnitCnt == 0 ) {   
    pid_t iPid = getpid(); 
    debug_print("iPid %d\n", iPid);
    kill(iPid, SIGINT);          
  } 
}
  
