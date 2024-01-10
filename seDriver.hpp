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
 
#include <lely/coapp/node.hpp>
#include <cstring>
#include <lely/can/net.hpp>
#include <lely/co/time.h>
#include "seMaster.hpp"
#include <lely/coapp/fiber_driver.hpp>
#include <sys/time.h>
#include "seSensingUnitsObj.h"


typedef struct {
  uint8_t  id;              //< Slave's ID
  uint32_t Sync;            //< Enable Sync Producer
} seNode;

using namespace std::chrono_literals;
using namespace lely;
using namespace lely::ev;
using namespace lely::io;
using namespace lely::canopen;


#define NO          ((uint32_t)-1)
#define READ        (           1)
#define WRITE       (           2)
#define MASTER_SYNC (           0)

#define SHOW_ALL      (0xffff0000)
  
// This driver inherits from FiberDriver, which means that all CANopen event
// callbacks, such as OnBoot, run as a task inside a "fiber" (or stackful
// coroutine). 
class seDriver : public lely::canopen::FiberDriver { 

 public:
 
  using FiberDriver::FiberDriver;
  
  seDriver(ev_exec_t* exec, seMaster& master, uint8_t id);
  ~seDriver();
  
  static void setRawOutput( bool val );
  static uint32_t tryReadWriteDataType( char * s );
  static int tryNMTcmd( char * s );
  static int trySampleRate( char * sr );  
  static int tryCANbitRate( char * br );
  static void str2upcase( char * s );
  static void SetTPDO( uint8_t val );
  
  static uint32_t HeartBeatSlave;
  static uint32_t HeartBeatMaster;
  static uint32_t MaxSample;
  static bool RawOutput;
  static uint32_t NMT;
  static bool SaveParams;
  static uint32_t TimeStamp;
  static bool LogFile;
  static char AppStartTime[32];  
  static uint32_t DisplayNode;
  static bool Fahrenheit;
  
  static uint32_t ReadWrite;
  static uint32_t ReadWriteDataType;
  static uint32_t ReadWriteIndex; 
  static uint32_t ReadWriteSub;
  static uint32_t ReadWriteVal;
  static uint32_t UnitCnt;
  static uint32_t UnitBootCnt;
  static uint32_t Sync;
  static uint32_t SyncPeriod;
  static uint32_t SyncOverflow;
  static uint32_t TPDOSettings;
  static uint32_t TPDOMode;
  static uint32_t TPDOverflowCnt;  
  static uint32_t FIRCoeff[512];
  static   char * SampleRatePzs;
  /*
  ** This parameters can only be modified when the sensor unit is 
  ** in pre-operational mode. 
  */
  static uint32_t NewNodeID;
  static uint32_t CANbitRateIx;
  static uint32_t LoggingMode;
  static uint32_t Filter;
  static bool     RestoreAllSettings;
  static uint32_t Dump;
  static uint32_t UDFParam;
  static uint32_t ApplyChanges;
  static bool     UDFFile;



  private:
  
  void OnBoot(canopen::NmtState st, char es, const std::string& what) noexcept;// override;
  void OnConfig(std::function<void(std::error_code ec)> res) noexcept;
  void OnDeconfig(std::function<void(std::error_code ec)> res) noexcept;
  void OnTime(const ::std::chrono::system_clock::time_point& abs_time) noexcept;
  void OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept;
  void OnHeartbeat(bool occurred) noexcept;
  void OnState(canopen::NmtState st) noexcept;
  void OnCanState(io::CanState new_state, io::CanState old_state) noexcept;
  void OnCommand(canopen::NmtCommand cs) noexcept;
  void DisableSyncProducer(void);
  void SetSyncProducer(void);
  void EnableSyncProducer(void);
  void EnableTPDO(void);
  void DisableTPDO(void);
  void ObjectDump(void);  
  void DumpUDF(void);
  void getFlagOptions(void);
  void Read(void);
  void Write(void);
  void getDataType(void);
  void getSampleTime( void );
  void setFilter(void);
  bool prcOutput(uint16_t idx, uint8_t subidx);
  uint32_t setSampleRate(char * srate);  
  uint32_t getCANbitRate(uint32_t ix);
  void quit(void);
  void getModelFeatures(void);
  void startLogRecord(void);
  bool openErrMsgFile(void);
  void setUDFParam(void);
  void applyParam(void);

  
  FILE  *  m_fpLogFile;
  FILE  *  m_fpMsgFile;
  bool     m_Error;
  bool     m_ShowMe;
  bool     m_InitialBoot;
  bool     m_SampleProcessing;
  uint16_t m_SampleCounter;
  canopen::NmtState m_BootState;
  
  uint32_t m_DeviceName;
  uint32_t m_HardwareVersion;
  uint32_t m_SoftwareVersion;
  
  uint16_t m_exVal;
  uint16_t m_Days;
  uint32_t m_DaysInSec;  
  
  char  m_strtmp[1000];
  char  m_strlog[1000];
  char  m_strtmp_extra[64];
  char  m_strlog_extra[64];  
  char  m_comastr[16];
  uint32_t m_comaix;
  
  float m_GyroScale; 
  float m_AccelScale;
  float m_AttiScale1;
  float m_AttiScale2;
  float m_TiltAngleScale;  
  
  uint32_t      m_DumpSize;
  seObj      *  m_DumpObj;  
  uint32_t      m_OptionSize; 
  uint32_t      m_FilterSetSize;
  filterSet  *  m_FilterSet; 
  const char ** m_SampleRateStr;  
  const float * m_SampleRateFloat;
  
  float m_SampleTime; 
  float m_SampleIncremTime; 
  uint8_t m_TransmissionMode;
  uint8_t m_UnitVar;
  const char ** m_unitModeStr;
  uint32_t m_comacnt;
  uint32_t m_TPDOSettings;
  uint32_t m_NMT; 
  uint32_t m_DumpFIRCoeff[512];
  uint32_t m_ChangeModeTime;
  uint32_t m_RestoreParamTime;
  uint32_t m_SaveParamTime;
  uint32_t m_ApplyParamTime;
  uint32_t m_OtherTime;
};

