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

/*
 * g++ -std=c++14 -Wall -Wextra -pedantic -g -O2 $(pkg-config --cflags liblely-coapp) master.cpp seDriver.cpp M-G550PC2x.c M-G552PC1x.c M-G552PC7x.c M-A552AC1x.c -o build/co-master $(pkg-config --libs liblely-coapp) -DNDEBUG
 */


#include <lely/ev/loop.hpp>

#if defined(__linux__)
#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#else
#error This file requires Linux.
#endif
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/sigset.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/can/net.hpp>
#include <lely/co/time.h>

#include <lely/coapp/fiber_driver.hpp>
#include <iostream>
#include <lely/coapp/lss_master.hpp>
#include <lely/coapp/master.hpp>
#include <unistd.h>

#include <lely/coapp/node.hpp>

#include <sys/time.h>
#include <stdio.h>
#include <getopt.h>

#include "seMaster.hpp"
#include "seDriver.hpp"
#include "debug_print.h"


using namespace std::chrono_literals;
using namespace lely;
using namespace lely::ev;
using namespace lely::io;
using namespace lely::canopen;

#define MASTER_ID  125
#define SLAVES_MAX     (127) //< from 1 to 127 with master ID is included
seDriver * slave[SLAVES_MAX]; 
uint8_t UnitID[SLAVES_MAX]; 
char can_ch[32]   = "can0";
char dcf_file[1000] = "master.dcf";

void usage(char * execName) 
{
  printf("Usage: %s [OPTION]\n", execName);    
  printf("  -i, --id        \tSlave's node ID that are presented on CAN network\n");
  printf("                  \tRange: 1 - 127 or 'all'\n");  
  printf("                  \tExamples:\n");  
  printf("                  \t    %s -i 3 -i 1 -i 7\n", execName);  
  printf("                  \t    %s -i 3 1 7\n", execName); 
  printf("                  \t***Both examples are equivalent to each other\n");  
  printf("                  \tExample with 'all':\n"); 
  printf("                  \t    %s -i all\n", execName); 
  printf("                  \t***In this case will be created all(127-1) possible IDs for slaves but ID that belongs to master\n"); 
  printf("\n");
  printf("  -I, --newid     \tChange node ID\n");  
  printf("                  \tExample:\n"); 
  printf("                  \t 1. Change node ID 1 to 5, save and reset:\n");
  printf("                  \t    %s -i 1 -I 5 --save --reset\n", execName); 
  printf("                  \t***The --save and --reset commands are required to make the changes permanent and valid.\n"); 
  printf("\n");  
  printf("  -h, --hearts    \tSlave's heartbeat in milliseconds(default: not touch)\n"); 
  printf("                  \tExamples:\n");
  printf("                  \t 1. Make nodes 3 and 4 to produce heartbeats every 1 sec:\n");  
  printf("                  \t    %s -i 3 4 -h 1000\n", execName);    
  printf("                  \t 2. Disable heartbeats on nodes 3 and 4:\n");  
  printf("                  \t    %s -i 3 4 -h 0\n", execName);   
  printf("\n");  
  printf("  -H, --heartm    \tHeartbeat consumer on the master in milliseconds(default: 0 /disabled)\n"); 
  printf("                  \tExample, enable heartbeat consumer in %s every 2 sec\n", execName); 
  printf("                  \t%s -i 3 4 -h 1000 -H 2000\n", execName); 
  printf("\n");  
  printf("  -T, --tpdo      \tEnable/disable TPDO on all listed slaves (default: keep as is, max 4)\n"); 
  printf("                  \tParameter's order: --tpdo [control] [type] [n]\n"); 
  printf("                  \tcontrol: <dec> (default: not touch)\n"); 
  printf("                     \ttype: 'sync' or 'smpl' (default: not touch)\n"); 
  printf("                        \tn: <dec> 0-240 for 'sync' only (default: 1) \n");
  printf("                  \tExample:\n"); 
  printf("                  \t 1. Enable TPDO 1 and disable 2,3,4 on Node 3 and 4:\n");
  printf("                  \t%s -i 3 4 -x 10 --tpdo 1\n", execName);   
  printf("                  \t 2. Enable all TPDO 1, 2, 3 and 4 (no matter order) on Node 3 and 4:\n");
  printf("                  \t%s -i 3 4 -x 10 --tpdo 1234\n", execName); 
  printf("                  \t%s -i 3 4 -x 10 --tpdo 2314\n", execName); 
  printf("                  \t%s -i 3 4 -x 10 --tpdo 2431\n", execName); 
  printf("                  \t 3. Disable all TPDO on Node 3 and 4:\n");
  printf("                  \t%s -i 3 4 -x 10 --tpdo 0\n", execName);  
  printf("                  \t 4. Enable TPDO 1234 and switch transmission type to sync consumer mode on Node 3 and 4:\n");
  printf("                  \t%s -y 1000 -i 3 4 -x 10 --tpdo 1234 sync\n", execName);    
  printf("                  \t 5. Enable TPDO 1234 and switch to sync consumer mode (with n=2 times SYNC msg) on Node 3 and 4:\n");
  printf("                  \t%s -y 1000 -i 3 4 -x 10 --tpdo 1234 sync 2\n", execName); 
  printf("                  \t 6. Switch enabled TPDO to sync consumer mode (with n=2 times SYNC msg) Node 3 and 4:\n");
  printf("                  \t%s -y 1000 -i 3 4 -x 10 --tpdo sync 2\n", execName);   
  printf("                  \t 7. Switch enabled TPDO to sample mode on Node 3 and 4:\n");
  printf("                  \t%s -i 3 4 -x 10 --tpdo smpl\n", execName);    
  printf("\n");  
  printf("  -y, --sync      \tEnables SYNC message producer on master or preceding Node ID(default: not touch)\n"); 
  printf("                  \tPlacement --sync after node id (-i n) makes this node a sync producer\n"); 
  printf("                  \tPlacement --sync before all nodes' id (-i n) makes master a sync producer\n");   
  printf("                  \tParameter's order: --sync period [overflow]\n"); 
  printf("                    \tperiod: <dec>\n"); 
  printf("                  \toverflow: <dec>\n");   
  printf("                  \tExamples:\n"); 
  printf("                  \t 1. SYNC producer on Node 3 and other nodes (7 and 4) are sync consumers:\n");
  printf("                  \t    %s -i 7 4 3 --sync 1000 --tpdo sync 1 --log -x 3\n", execName); 
  printf("                  \t    %s -i 3 --sync 1000 -i 7 4 --tpdo sync --log -x 3\n", execName);   
  printf("                  \t 2. SYNC producer on Master, other nodes (7, 4 and 3) are sync consumers:\n");
  printf("                  \t    %s --sync 1000 -i 3 7 4 --tpdo sync 3 --log -x 3\n", execName);   
  printf("\n");  
  printf("  -L, --load      \tLoads factory's default values(no parameter)\n"); 
  printf("                  \tExample:\n"); 
  printf("                  \t    %s -i 3 --load\n", execName);   
  printf("                  \t    %s -i 3 --load --save --reset\n", execName);  
  printf("                  \t***The --save and --reset commands are required to make the changes permanent and valid\n");
  printf("\n");    
  printf("  -b, --canbitrate\tCAN bitrates 1M/ 800k/ 500k/ 250k/ 125k/ 50k/ 20k/ 10k bps(default:250k)\n");
  printf("                  \tEqual: -b 50k, -b 50000, --canbitrate 50k or --canbitrate 50000\n");
  printf("                  \tExamples:\n");   
  printf("                  \t 1. Set CAN bitrate to 250k on Node 3 and 4, save and reset:\n");
  printf("                  \t    %s -i 3 4 --canbitrate 250k --save --reset\n", execName); 
  printf("                  \t 2. Change ID to 7 on node 3, set CAN bitrate to 250k, save and reset:\n");
  printf("                  \t    %s -i 3 --newid 7 --canbitrate 250k --save --reset\n", execName); 
  printf("                  \t***The --save and --reset commands are required to make the changes permanent and valid\n");   
  printf("\n");   
  printf("  -R, --read      \tRead object value located at index and subindex\n"); 
  printf("                  \tParameter's order: Index SubIndex [DataType]\n"); 
  printf("                     \tIndex: <hex>\n"); 
  printf("                  \tSubIndex: <hex>\n"); 
  printf("                  \tDataType: U8, U32, I16, U16 or VS (optional)\n");    
  printf("                  \tExamples to read U16 value from node id=3, index=2100h, subindex=0h:\n");    
  printf("                  \t    %s -i 3 -R 2100 0 U16\n", execName);  
  printf("                  \t    %s -i 3 -R 2100 0\n", execName);   
  printf("\n");  
  printf("  -W, --write     \tWrite object value located at index and subindex\n"); 
  printf("                  \tParameter's order: Index SubIndex Value [DataType]\n");  
  printf("                     \tIndex: <hex>\n"); 
  printf("                  \tSubIndex: <hex>\n"); 
  printf("                     \tValue: <hex>\n"); 
  printf("                  \tDataType: U8, U32, I16, U16 or VS (optional)\n");   
  printf("                  \tExample to write U16 value=A5 to node id=3, index=2100h, subindex=0h:\n");    
  printf("                  \t    %s -i 3 -W 2100 0 A5 U16\n", execName); 
  printf("                  \t    %s -i 3 -W 2100 0 A5\n", execName);   
  printf("                  \t***Only -R or -W can be used at a time. The last listed preempts\n"); 
  printf("\n");   
  printf("  -t, --time      \tMaster produces TIME messages with period in msec, 0 is for a single shot(default: no TIME)\n"); 
  printf("                  \tExample:\n");  
  printf("                  \t    %s -i 3 -t 0 -x 10\n", execName);   
  printf("\n");   
  printf("  -s, --samplerate\tSample rate 1000/ 500/ 250/ 125/ 62.5/ 31.25/ 15.625/ 400/ 200/\n");
  printf("                  \t            100/ 80/ 50/ 40/ 25/ 20 sps(default:100)\n"); 
  printf("                  \tExample:\n");
  printf("                  \t    %s -i 3 -i 4 --samplerate 15.625 -x 5\n", execName);
  printf("                  \t***Assumed the nodes are in sample mode\n"); 
  printf("\n");     
  printf("  -f, --filter    \tSet AI filter tap Constant 1(default: not touch)\n"); 
  printf("                  \tOptions: 1-19, aver, k32, k64, k128, k512 (aka: Moving Average, Kaiser 32, 64, 128, 512)\n");   
#ifdef RESERV_UDF
  printf("                  \tUser Defined Filter (UDF): U4, U64, U128, U512\n");     
  printf("                  \tUDF optional parameters: file.txt(FIR coeff), save, load, erase, verify(to/from non-volatile memory)\n");   
#endif    
  printf("                  \tExamples:\n");    
  printf("                  \t 1. Manual filter assignment, C1 to 9, for all listed nodes (ID 3 and 4):\n"); 
  printf("                  \t    %s -i 3 -i 4 -f 9 --log --dump -x 100\n", execName);  
  printf("                  \t 2. Auto average assignment for all listed nodes:\n"); 
  printf("                  \t    %s -i 3 -i 4 -f aver --log --dump -x 100\n", execName);  
  printf("                  \t 3. Auto Kaiser 32 assignment for all listed nodes:\n"); 
  printf("                  \t    %s -i 3 -i 4 -f k32 --log --dump -x 100\n", execName);
#ifdef RESERV_UDF
  printf("                  \t 4. User Defined Filter U4 for all listed nodes:\n"); 
  printf("                  \t    %s -i 3 -i 4 -f U4 fircoeff.txt --log --apply --dump -x 100\n", execName);  
  printf("                  \t 5. Save User Defined Filter U512 for all listed nodes:\n"); 
  printf("                  \t    %s -i 3 -i 4 -f U512 fircoeff.txt save --log --apply --dump -x 100\n", execName);  
  printf("                  \t 6. Load User Defined Filter U512 for all listed nodes:\n"); 
  printf("                  \t    %s -i 3 -i 4 -f U512 load --log --apply --dump -x 100\n", execName); 
#endif 
  printf("                  \t***Auto (aver, k32, k64, k128) only works for the sample mode\n");   
  printf("                  \t   Use manual filter assignment for the sync transmission mode\n"); 
  printf("\n");
  printf("  -a, --apply     \tApply parameters of associated measurement(if device has object 2005h)\n"); 
  printf("                  \tOptional parameter: check device specification for object 2005h\n");  
  printf("                  \tExample applies parameters for current sensor type:\n");  
  printf("                  \t    %s -i 3 --tpdo 1234 --apply\n", execName);  
  printf("                  \tExample (for M-A552AC1) applies parameters as Accelerometer:\n"); 
  printf("                  \t    %s -i 3 --tpdo 1234 --apply 11\n", execName);  
  printf("\n");
  printf("  -S, --save      \tSave current device settings to non-volatile memory(no argument)\n"); 
  printf("                  \tExamples:\n");  
  printf("                  \t    %s -i 3 --tpdo 1234 --save\n", execName);   
  printf("                  \t    %s -i 3 -W 2005 0 21 --tpdo 1234 --save --reset\n", execName);
  printf("                  \t    %s -i 3 -W 1F80 0 C --tpdo 1234 --save --reset -x 10\n", execName);
  printf("                  \t***The --reset command is required to make the changes valid\n");
  printf("\n");   
#ifdef RESERV_UDF 
  printf("  -D, --dump      \tDump object's settings to console or log file(optional arguments: UDF, all)\n"); 
  printf("                  \tOptional argument: UDF (user defined FIR settings (available for some devices)\n"); 
  printf("                  \tOptional argument: all (dump both)\n"); 
#else
  printf("  -D, --dump      \tDump object's settings to console or log file\n");
#endif
  printf("                  \tExamples:\n"); 
  printf("                  \t    %s -i 3 --dump\n", execName);
#ifdef RESERV_UDF
  printf("                  \t    %s -i 3 --dump udf\n", execName);
#endif
  printf("                  \t    %s -i 3 --dump --log\n", execName);  
  printf("                  \t    %s -i 3 --tpdo 1234 smpl -x 10 --dump\n", execName);
  printf("\n");  
  printf("  -u, --reset     \tReset nodes(no argument)\n"); 
  printf("                  \tExamples:\n"); 
  printf("                  \t    %s -i 3 --reset\n", execName);
  printf("                  \t    %s -i 3 4 2 --reset\n", execName);
  printf("\n");  
  printf("  -x, --samplemax \tQuit after N specified samples(default:0, no limit:-1)\n"); 
  printf("                  \tExample, output 200 samples to console:\n");
  printf("                  \t    %s -i 4 -x 200\n", execName);   
  printf("                  \tExample, unlimited output to log files with dump for ID 3 and 4:\n");
  printf("                  \t    %s -i 3 -i 4 -x -1 --dump --log\n", execName); 
  printf("                  \t***Ctrl+c to quit\n"); 
  printf("\n");    
  printf("  -d, --display   \tDisplay samples output on console (default: first node ID in argument line)\n");
  printf("                  \tOptions: 0-127, 'all'\n");
  printf("                  \tExamples:\n");    
  printf("                  \t 1. Show on Console only samples output of ID 3. Log to files ID 3 and 4:\n"); 
  printf("                  \t    %s -i 3 -i 4 -x 100 -d 4 --log\n", execName);  
  printf("                  \t 2. No samples output to Console. Log to files ID 3 and 4:\n"); 
  printf("                  \t    %s -i 3 -i 4 -x 100 -d 0 --log\n", execName);  
  printf("                  \t 3. Show samples output on Console for all nodes:\n"); 
  printf("                  \t    %s -i 3 -i 4 -i 2 -x 100 -d all\n", execName);  
  printf("                  \t 4. Show the first ID (3) samples output on Console and Ignore others (default):\n"); 
  printf("                  \t    %s -i 3 4 2 -x 100\n", execName);  
  printf("\n");     
  printf("  -r, --raw       \tOutput samples in raw format (no argument, default: no)\n"); 
  printf("                  \tExample:\n");  
  printf("                  \t    %s -i 3 -x 10 --raw\n", execName);  
  printf("\n");  
  printf("  -l, --log       \tWrite sensor samples to CSV file. Each ID has its own log file (no argument, default: no)\n"); 
  printf("                  \tExample:\n"); 
  printf("                  \t    %s -i 3 -i 4 -x 10 --dump --log\n", execName); 
  printf("                  \t***LogFile name is based on model name, node id and time\n"); 
  printf("\n");   
  printf("  -F, --fahrenheit\tOutput temperature in Fahrenheit(no argument, default: Celsius)\n"); 
  printf("                  \tExample:\n"); 
  printf("                  \t    %s -i 3 -i 4 -x 10 --fahrenheit --log\n", execName); 
  printf("\n"); 
  printf("  -c, --socan     \tSocketcan channel(default:'%s')\n", can_ch);
  printf("                  \tExample:\n");  
  printf("                  \t    %s -c can0 -i 3 -x 10\n", execName); 
  printf("\n");  
  printf("  -g, --dcf       \tDevice Configuration File name(default: master.dcf)\n");
  printf("                  \tExample:\n");  
  printf("                  \t    %s --dcf master_and_slaves.dcf -i 3 -x 10\n", execName);   
  printf("\n");
  printf("  -v, --version   \t%s version\n", execName); 
  printf("\n");  
  printf("  -?, --help      \tThis info\n"); 
  printf("\n");   

}

void parse_options(int argc, char *argv[]) 
{
  int c;
  uint32_t ix = 0;

  int option_index = 0;
  uint32_t prm;
  
  static struct option long_options[] =
    { 
      {     "apply",        no_argument, 0, 'a'}, 
      {"canbitrate",  required_argument, 0, 'b'}, 
      {     "socan",  required_argument, 0, 'c'}, 
      {   "display",        no_argument, 0, 'd'}, 
      {      "dump",        no_argument, 0, 'D'},     
      {        "id",  required_argument, 0, 'i'}, 
      {    "filter",  required_argument, 0, 'f'}, 
      {"fahrenheit",        no_argument, 0, 'F'},
      {       "dcf",  required_argument, 0, 'g'}, 
      {    "hearts",  required_argument, 0, 'h'}, 
      {    "heartm",  required_argument, 0, 'H'}, 
      {       "log",        no_argument, 0, 'l'}, 
      {      "load",        no_argument, 0, 'L'},    
      {       "nmt",  required_argument, 0, 'n'},
      {       "raw",        no_argument, 0, 'r'},
      {"samplerate",  required_argument, 0, 's'},
      {     "times",  required_argument, 0, 't'},
      {      "tpdo",  required_argument, 0, 'T'},      
      {      "save",        no_argument, 0, 'S'},
      {     "newid",  required_argument, 0, 'I'},      
      { "samplemax",  required_argument, 0, 'x'},
      {      "sync",  required_argument, 0, 'y'},
      {      "read",  required_argument, 0, 'R'},
      {     "write",  required_argument, 0, 'W'},
      {     "reset",        no_argument, 0, 'u'},
      {   "version",        no_argument, 0, 'v'},
      {      "help",        no_argument, 0, '?'},
      {0, 0, 0, 0}
    };  

  while ((c = getopt_long(argc, argv, "ab:c:d:Di:I:f:Fg:h:H:lLn:rR:s:St:T:vW:x:y:uv?", long_options, NULL)) != -1) {

    switch (c)
    {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf ("option %s", long_options[option_index].name);
      if (optarg)
        printf (" with arg %s", optarg);
      printf ("\n");
      exit(0);
      break;
    case 'a':
      //> ./co-master -i 3 -apply 
      seDriver::ApplyChanges = 0;
      if ( argc - optind > 0 ) {
        if ( *(argv[optind]) != '-') {
          seDriver::ApplyChanges = std::stoul(argv[optind], nullptr, 16);       
        }
      } 
      debug_print( "ApplyChanges: %.02xh\n", (uint8_t)seDriver::ApplyChanges);
      break;     
    case 'R':
      //> ./co-master -i 3 -R 1000 0 u32
      //> -R Index<hex> SubIndex<hex> [DataType]
      seDriver::ReadWriteIndex = std::stoul(optarg, nullptr, 16);
      debug_print( "ReadWriteIndex: %.04xh\n", (uint16_t)seDriver::ReadWriteIndex);         
      if ( argc - optind > 0 ) {
        if ( *(argv[optind]) != '-') {
          seDriver::ReadWriteSub = std::stoul(argv[optind], nullptr, 16);
          debug_print( "ReadWriteSub: %.02xh\n", (uint8_t)seDriver::ReadWriteSub);      
          seDriver::ReadWrite = READ;         
        }
      }
      if ( argc - optind > 1 ) {          
        if ( *(argv[optind+1]) != '-' ) {
          if ( seDriver::tryReadWriteDataType(argv[optind+1]) == NO ) {
            seDriver::ReadWrite = NO;
          }
        }
      }   
      debug_print("ReadWriteDataType: %.4s (%.08xh)\n", (char*)&seDriver::ReadWriteDataType, (uint32_t)seDriver::ReadWriteDataType);    
      if (seDriver::ReadWrite != READ) {
        seDriver::ReadWriteDataType = NO;
        seDriver::ReadWriteIndex = NO;
        seDriver::ReadWriteSub = NO;
        fprintf(stderr, "Error: -R, --read: input params failed\n" );
        exit(0);
      }      
      break; 
    case 'W':
      //> ./co-master -i 3 -W 1000 0 1 u32
      //> -W Index<hex> SubIndex<hex> Value [DataType]
      seDriver::ReadWriteIndex = std::stoul(optarg, nullptr, 16);
      debug_print( "ReadWriteIndex: %.04xh\n", (uint16_t)seDriver::ReadWriteIndex);       
      if ( argc - optind > 1 ) {
        if ( *(argv[optind]) != '-') {
            seDriver::ReadWriteSub = std::stoul(argv[optind], nullptr, 16);
            debug_print( "ReadWriteSub: %.02xh\n", (uint8_t)seDriver::ReadWriteSub); 
        }  
        if ( *(argv[optind+1]) != '-') {
            try {
                seDriver::ReadWriteVal = std::stoul(argv[optind+1], nullptr, 16);
                debug_print( "ReadWriteVal: %.08xh\n", (uint32_t)seDriver::ReadWriteVal); 
            } catch (std::invalid_argument const& ex) {
                seDriver::ReadWriteVal = *(uint32_t*)argv[optind+1];
                debug_print( "ReadWriteVal: %.4s\n", (char*)&seDriver::ReadWriteVal); 
                seDriver::ReadWriteDataType = VS;                  
            }              
            seDriver::ReadWrite = WRITE; 
        } 
      }            
      if ( argc - optind > 2 ) {              
        if ( *(argv[optind+2]) != '-') { 
          if ( seDriver::tryReadWriteDataType(argv[optind+2]) == NO ) {
            seDriver::ReadWrite = NO;
          }            
        }
      }
      debug_print("ReadWriteDataType: %.4s (%.08xh)\n", (char*)&seDriver::ReadWriteDataType, (uint32_t)seDriver::ReadWriteDataType); 
        
      if (seDriver::ReadWrite != WRITE) {
        seDriver::ReadWriteDataType = AU;
        seDriver::ReadWriteIndex = NO;
        seDriver::ReadWriteSub = NO;
        seDriver::ReadWriteVal = NO;  
        fprintf(stderr, "Error: --write params failed\n" );  
        exit(0);      
      } 
      break;             
    case 'L':
        seDriver::RestoreAllSettings = true;       
      break;  
    case 'T':
      prm = optind-1;
      seDriver::str2upcase(argv[prm]);
      if (strcmp(optarg, "SYNC") && strcmp(optarg, "SMPL")) { 
        /* control */       
        if (strchr(argv[prm], '1')) seDriver::SetTPDO(1<<0); //< enable tpdo 1
        if (strchr(argv[prm], '2')) seDriver::SetTPDO(1<<1); //< enable tpdo 2
        if (strchr(argv[prm], '3')) seDriver::SetTPDO(1<<2); //< enable tpdo 3
        if (strchr(argv[prm], '4')) seDriver::SetTPDO(1<<3); //< enable tpdo 4
        if (strchr(argv[prm], '0')) seDriver::SetTPDO(0);    //< disable all tpdo
        prm++;
      }
      if ( argc - prm > 0 ) {            
        if ( *(argv[prm]) != '-') { 
          seDriver::str2upcase(argv[prm]);
          if (!strcmp(argv[prm], "SYNC")) {
            prm++;
            seDriver::TPDOMode = 1;
            if ( argc - prm > 0 ) {              
              if ( *(argv[prm]) != '-') {  
                seDriver::TPDOMode = std::stoul(argv[prm], nullptr, 10);     
              }
            }
            debug_print( "TPDOMode: %xh\n", seDriver::TPDOMode);
          } else if (!strcmp(argv[prm], "SMPL")) { 
            seDriver::TPDOMode = 0xFE;          
          }
        }
      }  
      if ( seDriver::TPDOMode==NO && seDriver::TPDOSettings==NO ) {
        fprintf(stderr, "Error: --tpdo params failed\n" ); 
        exit(0);   
      }  
      break;
    case 'b':
      if (seDriver::tryCANbitRate(optarg) < 0) {
        usage(argv[0]);
        exit(0);
      }
      break;     
    case 'c':
      strcpy(can_ch, optarg);
      break; 
    case 'd':  
      seDriver::str2upcase(optarg);
      if (!strcmp(optarg, "ALL")) {
        seDriver::DisplayNode = SHOW_ALL; 
        debug_print("DisplayNode: all\n");
      } else {      
        seDriver::DisplayNode = (uint8_t)atoi(optarg); 
      }                           
      break;  
    case 'D':  
      if ( argv[optind] && *(argv[optind]) != '-' ) {
        seDriver::str2upcase(argv[optind]);
        if (!strcmp(argv[optind], "UDF")) {
          seDriver::Dump = 2; /* UDF */
        } else if (!strcmp(argv[optind], "ALL")) {
          seDriver::Dump = 3; /* ALL */
        }
      } 
      if ( seDriver::Dump == 0 ) {
        seDriver::Dump = 1; /* REG */
      }   
      break;        
    case 'f':  
      seDriver::str2upcase(optarg);
      if (!strcmp(optarg, "AVER") || !strcmp(optarg, "AVERAGE")) {
        seDriver::Filter = MA;
        debug_print("Filter: %.4s\n", (char*)&seDriver::Filter);
      } else if (!strcmp(optarg, "K32")) {
        seDriver::Filter = K32;
        debug_print("Filter: %.4s\n", (char*)&seDriver::Filter);
      } else if (!strcmp(optarg, "K64")) {
        seDriver::Filter = K64; 
        debug_print("Filter: %.4s\n", (char*)&seDriver::Filter);  
      } else if (!strcmp(optarg, "K128")) {
        seDriver::Filter = K128; 
        debug_print("Filter: %.4s\n", (char*)&seDriver::Filter);       
      } else if (!strcmp(optarg, "K512")) {
        seDriver::Filter = K512;
        debug_print("Filter: %.4s\n", (char*)&seDriver::Filter);     
      } else if (!strcmp(optarg, "U4"  )  || !strcmp(optarg, "U64") ||
                 !strcmp(optarg, "U128") || !strcmp(optarg, "U512") ) {
        char ts[128];
        memcpy( &(seDriver::Filter), optarg, strlen(optarg));
        
        for (int i=optind; i<=argc && argv[i] && *(argv[i]) != '-'; i++) {
          strcpy(ts, argv[i]);
          seDriver::str2upcase(ts);
          debug_print("argv[i]: %s\n", ts); 
          if (strcmp(ts, "SAVE") == 0) {            
            seDriver::UDFParam = UDF_SAVE; 
          } else if (strcmp(ts, "LOAD") == 0) {
            seDriver::UDFParam = UDF_LOAD; 
          } else if (strcmp(ts, "ERASE") == 0) {
            seDriver::UDFParam = UDF_ERASE; 
          } else if (strcmp(ts, "VERIFY") == 0) {
            seDriver::UDFParam = UDF_VERIFY; 
          } else {
            FILE *fp;            
            if ((fp = fopen(argv[i],"r")) == NULL){
              fprintf(stderr, "Error: opening FIR coeff file\n");
              exit(0);
            }
            uint32_t * fircoef = seDriver::FIRCoeff;
            for (uint16_t i=0; i<512; i++) {
              if ( fscanf(fp, "%x ", fircoef) == 1 ) {
                debug_print("fircoef[%u]=0x%08x\n", i, *fircoef);
                fircoef++;
              } else if (i < 4) {
                fprintf(stderr, "Error: reading FIR coeff file too short\n");
                fclose(fp);
                exit(0);
              } else {
                break;
              }
            }
            seDriver::UDFFile = true;            
          }
        }    
      } else {        
        seDriver::Filter = atoi(optarg);  
        debug_print("Filter: %xh\n", seDriver::Filter);
        if (seDriver::Filter > 0x14) {
          fprintf(stderr, "Error: filter setting over range: %u\n", seDriver::Filter);
          exit(0);
        }        
      }
      break; 
    case 'F':
      seDriver::Fahrenheit = true;
      break;        
    case 'g':
      strcpy(dcf_file, optarg);
      break;                       
    case 'i':
      seDriver::str2upcase(optarg);
      if ( !strcmp(optarg, "ALL") && ix==0 ) {
        debug_print("\nID: ");
        for ( ; ix < SLAVES_MAX; ix++ ) {
          UnitID[ix] = ix+1;
          debug_print("%d ", UnitID[ix]);          
        }
        debug_print("\n");
      } else {
        for (int i=optind-1; i<=argc && argv[i] && *(argv[i]) != '-'; i++) {
          debug_print("argv[i]: %s\n", argv[i]); 
          try { 
            UnitID[ix] = std::stoul(argv[i], nullptr, 10);
            debug_print("UnitID[ix]: %d\n", UnitID[ix]); 
            if ( UnitID[ix] == 0 || UnitID[ix] > SLAVES_MAX ) {
              fprintf(stderr, "Error: wrong ID number: %d\n", UnitID[ix]);
              exit(0);
            }
            ix++; 
          } catch  (std::invalid_argument const& ex) {
            fprintf(stderr, "Error: wrong ID number: %s\n", argv[i]);
            exit(0);
          }      
        }
      }
      break;  
    case 'I':         
      if ( ix > 1 ) {
        fprintf(stderr, "Error: Only one Node ID can be changed at a time (--newid)\n");
        exit(0);        
      } else {                       
        try { 
          seDriver::NewNodeID = std::stoul(optarg, nullptr, 10);
          if ( seDriver::NewNodeID == 0 || seDriver::NewNodeID > 127) {
            fprintf(stderr, "Error: wrong ID number: %u\n", seDriver::NewNodeID);
            exit(0);
          } 
        } catch  (std::invalid_argument const& ex) {
          fprintf(stderr, "Error: wrong ID number: %s\n", optarg);
          exit(0);
        }
      }
      break; 
    case 'l':
      seDriver::LogFile = true;
      break;  
    case 'u':   
      seDriver::NMT = 129; //< RESET_NODE
      break;             
    case 'n':   
      if ( seDriver::tryNMTcmd(optarg) < 0 ) {
        usage(argv[0]);
        exit(0);
      }
      break;     
    case 'r':
      seDriver::RawOutput = true;
      break;
    case 's':                                  
      if ( seDriver::trySampleRate(optarg) < 0 ) {
        usage(argv[0]);
        exit(0);
      }        
      break; 
    case 't':                                  
      seDriver::TimeStamp = atoi(optarg);  
      break;   
    case 'h':
      seDriver::HeartBeatSlave = (uint16_t)atoi(optarg);       
      break;
    case 'H':
      seDriver::HeartBeatMaster = (uint16_t)atoi(optarg);       
      break; 
    case 'S':                                  
      seDriver::SaveParams = true; 
      break;                    
    case 'x':                                  
      seDriver::MaxSample = (uint32_t)atoi(optarg); 
      break;           
    case 'y':
      if ( seDriver::Sync != NO ) {
        fprintf(stderr, "Error: Only one SYNC producer allowed\n");
        exit(0);
      }                               
      if ( ix > 0 ) {  
        seDriver::Sync = UnitID[ix-1];
        seDriver::SyncPeriod = atoi(optarg);
        if ( argv[optind] && *(argv[optind]) != '-') {
          seDriver::SyncOverflow = atoi(argv[optind]);            
        }
        debug_print("Sync Slave %d, period: %u, overflow: %u\n", UnitID[ix-1], seDriver::SyncPeriod, seDriver::SyncOverflow);
      } else {
        seDriver::Sync = MASTER_SYNC;
        seDriver::SyncPeriod = atoi(optarg);
        if ( argv[optind] && *(argv[optind]) != '-') {
          seDriver::SyncOverflow = atoi(argv[optind]);            
        }        
        debug_print("Master Sync, period: %u, overflow: %u\n", seDriver::SyncPeriod, seDriver::SyncOverflow);
      } 
      break;
   case 'v':
        printf("%s\n", COMASTER_VERSION);
        exit(0);
      break;
   case '?':
        usage(argv[0]);
        exit(0);
      break;
      
    default:
      printf("unknown param:%c\n", c);
      usage(argv[0]);
      exit(0);
    }
  }
  
  if ( ix < 1 ) {
    fprintf(stderr, "Error: Slave ID unknown (-i n1)\n");
    exit(0);
  }
  if ( UnitID[0] > 127 || UnitID[0]==0 ) { //< if nothing was assigned
    fprintf(stderr, "Error: wrong ID number: %d\n", UnitID[0]);
    exit(0);
  }  
  if ( ix > 1 && seDriver::NewNodeID != NO ) {
    fprintf(stderr, "Error: Only one Node ID can be changed at a time (--newid)\n");
    exit(0);      
  }
  
  if ( seDriver::RestoreAllSettings || seDriver::CANbitRateIx != NO || seDriver::NewNodeID != NO ) {
    
    char str[256] = {0,};
    if ( seDriver::RestoreAllSettings ) 
      printf("Restore All Factory Default Settings requested (--load)\n");
    if (seDriver::NewNodeID != NO)
      printf("Node ID change requested (--newid)\n");
    if (seDriver::CANbitRateIx != NO)
      printf("Can Bitrate requested (--canbitrate)\n");
    
    if ( seDriver::HeartBeatSlave  != NO ) strcat( str, "\n      --hearts" );    
    if ( seDriver::HeartBeatMaster != NO ) strcat( str, "\n      --heartm" );
    if ( seDriver::TPDOSettings != NO || 
                 seDriver::TPDOMode != NO) strcat( str, "\n        --tpdo" );
    if ( seDriver::Sync != NO )            strcat( str, "\n        --sync" );
    if ( seDriver::ReadWrite == READ )     strcat( str, "\n        --read" );
    if ( seDriver::ReadWrite == WRITE )    strcat( str, "\n       --write" );    
    if ( seDriver::TimeStamp != NO )       strcat( str, "\n        --time" );
    if ( seDriver::SampleRatePzs != NULL ) strcat( str, "\n  --samplerate" );
    if ( seDriver::Filter != 0 )           strcat( str, "\n      --filter" );
    if ( seDriver::Dump != 0 )             strcat( str, "\n        --dump" );
    if ( seDriver::MaxSample != 0 )        strcat( str, "\n   --samplemax" );
    if ( seDriver::RawOutput == true )     strcat( str, "\n         --raw" );    
    if ( seDriver::Fahrenheit == true )    strcat( str, "\n  --fahrenheit" );
    
    if ( *str != 0 ) {
      printf("-Warning! The following command(s) will be ignored:%s\n", str );
    }     
  }
}

int main(int argc, char **argv) 
{

  time_t app_start_time;
  struct tm timeinfo;
  struct timeval tv;
  
  gettimeofday(&tv,NULL);
  app_start_time = (time_t)tv.tv_sec;
  timeinfo = *localtime((time_t*) &app_start_time);
  strftime(seDriver::AppStartTime, sizeof(seDriver::AppStartTime), "%Y-%m-%d %H:%M:%S", &timeinfo); 
  sprintf(&seDriver::AppStartTime[strlen(seDriver::AppStartTime)], ".%lu", tv.tv_usec/1000);
  debug_print("%s\n", seDriver::AppStartTime);
  
  memset(UnitID, 0xff, SLAVES_MAX);
  parse_options(argc, argv);
  
  // Initialize the I/O library. This is required on Windows, but a no-op on
  // Linux (for now).
  io::IoGuard io_guard;

  // Create an I/O context to synchronize I/O services during shutdown.
  io::Context ctx;
  // Create an platform-specific I/O polling instance to monitor the CAN bus, as
  // well as timers and signals.
  io::Poll poll(ctx);
  // Create a polling event loop and pass it the platform-independent polling
  // interface. If no tasks are pending, the event loop will poll for I/O
  // events.
  ev::Loop loop(poll.get_poll());
  // I/O devices only need access to the executor interface of the event loop.
  auto exec = loop.get_executor();
  
  // Create a timer using a monotonic clock, i.e., a clock that is not affected
  // by discontinuous jumps in the system time.
#if 0  
  io::Timer timer(poll, exec, CLOCK_MONOTONIC);
#else  
  io::Timer timer(poll, exec, CLOCK_REALTIME);
#endif  
  
  // Create a virtual SocketCAN CAN controller and channel, and do not modify
  // the current CAN bus state or bitrate.
  io::CanController ctrl(can_ch); 
  io::CanChannel chan(poll, exec);
  chan.open(ctrl);

  // Create a CANopen master with node-ID (MASTER_ID). The master is asynchronous, which
  // means every user-defined callback for a CANopen event will be posted as a
  // task on the event loop, instead of being invoked during the event
  // processing by the stack.  
  seMaster master(timer, chan, dcf_file, "", MASTER_ID); 
  if (seDriver::DisplayNode == NO) {
    seDriver::DisplayNode = UnitID[0];
  }
  if ( master.id()==1 && UnitID[0]==1) {
    printf("Reassign Master ID to the range: 2-127\n");    
  }
  
  seDriver slave_fixed(exec, master, UnitID[0]);
  slave[0] = NULL;
  for ( int i=1; i<SLAVES_MAX; i++ ) {
    slave[i] = NULL;
    if (master.id() == UnitID[i]) continue;
    if ( UnitID[i] != (uint8_t)NO ) {
      slave[i] = new seDriver(exec, master, UnitID[i]);
    } 
  }
  // Create a signal handler.
  io::SignalSet sigset(poll, exec);
  // Watch for Ctrl+C or process termination.
  sigset.insert(SIGHUP);
  sigset.insert(SIGINT);
  sigset.insert(SIGTERM);

  // Submit a task to be executed when a signal is raised. We don't care which.
  sigset.submit_wait([&](int /*signo*/) {
    // If the signal is raised again, terminate immediately.
    sigset.clear();
    printf("\n");
    if ( seDriver::MaxSample ) master.Command(canopen::NmtCommand::STOP);
    // Tell the master to start the deconfiguration process for all nodes, and
    // submit a task to be executed once that process completes.     
    master.AsyncDeconfig().submit(exec, [&]() {      
      // Perform a clean shutdown.
      debug_print("delete all drivers allocated by new operator\n");
      for ( int i=1; i<SLAVES_MAX; i++ ) {
        if ( slave[i] ) {        
          delete slave[i];
        }
      }
      debug_print("ctx.shutdown()\n");     
      ctx.shutdown();
    });           
  }); 

#ifndef NDEBUG
  master.OnTime([](const ::std::chrono::system_clock::time_point& abs_time) {
    timespec time = ::lely::util::to_timespec(abs_time);
    printf("===================== tv_sec:%ld, tv_nsec:%lu\n", time.tv_sec, time.tv_nsec);  
  
  });  
#endif
    
  master.Reset();
  loop.run();

  return 0;
}
