HW PREREQUISTES
===============
1. PCAN-USB Adapter or compatible
2. Epson IMU CANopen Device (i.e. M-G550PC2, M-G552PC1, M-G552PC7, M-A552AC1, etc)


BUILD AND INSTALL LELY LIBRARY
==============================
Follow Lely CANopen instructions*:
https://opensource.lely.com/canopen/docs/installation/

*If you want to avoid excessive debugging messages from the Lely library then you can include additional switches during configuration
At the stage in the instruction:
../configure --disable-cython
do this instead: 
../configure --disable-cython --disable-daemon --disable-diag CPPFLAGS=-DNDEBUG


BUILD CO-MASTER
===============
1. Go to 'co-master' directory, create directory 'build', compile the executeable, then goto the build directory:
mkdir build
g++ -std=c++14 -Wall -Wextra -pedantic -g -O2 $(pkg-config --cflags liblely-coapp) master.cpp seDriver.cpp M-G550PC2x.c M-G552PC1x.c M-G552PC7x.c M-A552AC1x.c -o build/co-master $(pkg-config --libs liblely-coapp) -DNDEBUG
cd build

2. Copy here one of master.dcf file from dcf folder (or create your own):
cp ../dcf/M-G550PC2_Sample/master.dcf .


DEVICE CONFIGURATION FILE
==========================
The co-master software uses and requires a Device Configuration File (DCF). The Lely library creates required objects and services such as SYNC, TIME, EMCY and others based on reading DCF provided to co-master.

There is dcfgen tool that can be used to generate DCF from Electronic Data Sheet (EDS) file (provided by manufacturer):
https://opensource.lely.com/canopen/docs/dcf-tools/

Please note that Receive-PDO (1,2,3,4) is not created in co-master if Transmit-PDO (1,2,3,4) disabled in DCF (or eds file). If Receive-PDO was created in co-master then the stream can be controlled through corresponded in slave's Transmit-PDO by enabling/disabling a co-master command, but if Receive-PDO was not created then co-master cannot receive data from slave. So, it makes sense to keep all Transmit-PDO enabled in eds files, if your DCF is not a final version.

Please note that current version of dcfgen tool does not generates TIME service. So, if you have plans to set time in slaves by sending TIME COB-ID than replace master.dcf.em file in your install dcfgen directory with the modified one located in co-master folder.

For example:
sudo cp master.dcf.em /usr/lib/python3/dist-packages/dcfgen/data/
* The master.dcf.em is the original work of Lely CANopen. The modified file is redistributed as per: https://www.apache.org/licenses/LICENSE-2.0

There are few examples of master.dcf files located in folders under co-master/dcf. You can modify it or use as is if it meets your requirements.

For example, you can modify file, co-master/dcf/M-G550PC2_Sample/master_M-G550PC2.yml, in a text editor and then generate the output file, master.dcf, by running this command in console:
dcfgen -r -v master_M-G550PC2.yml


CREATE CAN NETWORK
==================
Add device can0:
sudo ip link add dev can0 type can

Set default can bitrate for can0 to desire speed, i.e. 250000bps:
sudo ip link set can0 type can bitrate 250000

Bring can0 link up:
sudo ip link set up can0

If you want to monitor CAN network on can0:
candump can0

Please note that if you experience a CAN network instability which may be caused by high CAN bus utilization, you can try bringing the CAN network (i.e. can0) down and then up again:
sudo ip link set down can0
sudo ip link set up can0

NOTE: To mitigate instabilities due to high CAN bus utilization, try to 1) reduce the number of CAN nodes on the network, 2) reduce the CANopen device output rates, 3) increase the CAN bitrate, i.e. 1000000bps 


COMMANDS
========
1. Commands have a fixed order of execution no matter of their call line position.

   The order of execution:
   --hearts
   --heartm
   --tpdo 
   --samplerate
   --filter
   --write
   --time
   --apply
   --save
   --read
   --reset
   --dump
   --samplemax
   --sync

2. Three commands (--load, --newid and --canbitrate) will ignore or cancel all other commands (except --save, --reset, and --read) listed above.
 - Load Factory Default Values: (--id --load)
 - Change Node ID: (--id n1 --newid n2)
 - Change CAN bitrate: (--id n1 --canbitrate value)

   The order of execution:
   --load
   --newid
   --canbitrate

3. The Sync (--sync) is the command for which its position in the call does matter: 
 - Placement --sync before all node's id (-i n) makes master a sync producer
 - Placement --sync after node id (-i n) makes this node a sync producer


USAGE CO-MASTER
===============
Create and copy your master.dcf file into the build directory where co-master is.
Now you can run co-master.

Examples.

There are 4 units on CAN network that used in the Examples:
G552PC70, G552PC10 and two of G550PC2 (which is G55T in this example)

The Examples used with co-master/dcf/Multiple_MSM_Sample/master.dcf file that has the following nodes entries: 
Master ID: 125
Slave ID for M-G552PC70_Sample_6dof: 5
Slave ID for M-G552PC10_Sample_atti: 9
Slave ID for M-G550PC2: 1, 101, 4, 7

Note that ID 101 (65h) is a factory default for the unit, M-G550PC2 (G55T), used in the Examples.

1. Node ID info
~~~~~~~~~~~~~~~
Let's call slave id (-i, --id) with all ID that exist the master.dcf file.
The co-master shows outputs from 4 nodes with ID 9, 4, 5 and 7, and waits for the reply from nodes that do not exist (ID 1 and 101).
Push ctrl+C to stop waiting and exit.

sudo ./co-master -i 5 9 1 101 4 7
Node 9: Manufacturer Device Name: 0x32353547, 'G552'
Node 9: Manufacturer Hardware Version: 0x30314350, 'PC10'
Node 9: Manufacturer Software Version: 0x30302e31, '1.00'
Node 4: Manufacturer Device Name: 0x54353547, 'G55T'
Node 4: Manufacturer Hardware Version: 0x30304132, '2A00'
Node 4: Manufacturer Software Version: 0x30312e31, '1.10'
Node 5: Manufacturer Device Name: 0x32353547, 'G552'
Node 5: Manufacturer Hardware Version: 0x30374350, 'PC70'
Node 5: Manufacturer Software Version: 0x30302e31, '1.00'
Node 7: Manufacturer Device Name: 0x54353547, 'G55T'
Node 7: Manufacturer Hardware Version: 0x30304132, '2A00'
Node 7: Manufacturer Software Version: 0x30312e31, '1.10'


You also can use 'all' if nodes id unknown. In this case the co-master will create all(127-1) possible IDs for slaves except the ID that belongs to master:

sudo ./co-master -i all
Node 7: Manufacturer Device Name: 0x54353547, 'G55T'
Node 7: Manufacturer Hardware Version: 0x30304132, '2A00'
Node 7: Manufacturer Software Version: 0x30312e31, '1.10'
Node 4: Manufacturer Device Name: 0x54353547, 'G55T'
Node 4: Manufacturer Hardware Version: 0x30304132, '2A00'
Node 4: Manufacturer Software Version: 0x30312e31, '1.10'
Node 5: Manufacturer Device Name: 0x32353547, 'G552'
Node 5: Manufacturer Hardware Version: 0x30374350, 'PC70'
Node 5: Manufacturer Software Version: 0x30302e31, '1.00'
Node 9: Manufacturer Device Name: 0x32353547, 'G552'
Node 9: Manufacturer Hardware Version: 0x30314350, 'PC10'
Node 9: Manufacturer Software Version: 0x30302e31, '1.00'
^C

Push ctrl+C to exit.

2. Restoring Factory Default values
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The load command (-L, --load) can be called for multiple IDs but then units will have their ID set to factory default value. To avoid the potential CAN network ID conflict after --load with multiple IDs, do --load without --save and --reset, change their Node ID one by one, and then call --save and --reset commands. Another option is to call --load for one node only (with --save and --reset commands).
The more robust option to avoid CAN ID conflict is to configure only with one CANopen device connected to the CAN network at a time to ensure unique CAN IDs are assigned to each CANopen device.  

sudo ./co-master -i 4 --load --save --reset
Restore All Factory Default Settings requested (--load)
Node 4: Loading OD with factory default values
Node 4: Saving OD settings to non-volatile memory
Node 4: Resetting...

Now node 4 should have ID 101 (default). Let's check it:

sudo ./co-master -i 101
Node 101: Manufacturer Device Name: 0x54353547, 'G55T'
Node 101: Manufacturer Hardware Version: 0x30304132, '2A00'
Node 101: Manufacturer Software Version: 0x30312e31, '1.10'


3. Change Node ID
~~~~~~~~~~~~~~~~~
It is possible to call --newid (-I) command without --save and --reset commands, which could be useful to do other changes, like changing CAN bitrare (--canbitrate), and then use --save and --reset to make the changes permanent and valid.

Let's change ID from 101 to 1:

sudo ./co-master -i 101 --newid 1 --save --reset
Node ID change requested (--newid)
Node 101: ID changing to 1
Node 101: Saving OD settings to non-volatile memory
Node 101: Resetting...

Now you need to update your master.dcf file with the new ID (if you have not done already done so). Otherwise, co-master will not create resources for that new ID. In the Examples the master.dcf has the entries for node ID 1 already from the very beginning. 

Let's check our new Node ID 1:

sudo ./co-master -i 1
Node 1: Manufacturer Device Name: 0x54353547, 'G55T'
Node 1: Manufacturer Hardware Version: 0x30304132, '2A00'
Node 1: Manufacturer Software Version: 0x30312e31, '1.10'


4. Change CAN bitrate
~~~~~~~~~~~~~~~~~~~~~
There are 4 nodes in CAN network in the Examples. Let's change the default CAN bitrate 250k to 500k. 

sudo ./co-master -i 1 9 5 7 --canbitrate 500k --save
Can Bitrate requested (--canbitrate)
Node 9: Changing CAN Bitrate. The --reset command is required to make the changes valid.
Node 9: Saving OD settings to non-volatile memory
Node 1: Changing CAN Bitrate. The --reset command is required to make the changes valid.
Node 1: Saving OD settings to non-volatile memory
Node 5: Changing CAN Bitrate. The --reset command is required to make the changes valid.
Node 5: Saving OD settings to non-volatile memory
Node 7: Changing CAN Bitrate. The --reset command is required to make the changes valid.
Node 7: Saving OD settings to non-volatile memory


Now power off all nodes.

The bitrate of CAN network must be the same as the nodes canbitrate settings. Let's change CAN network bit rate (better in a separate terminal):

sudo ip link set down can0
sudo ip link set can0 type can bitrate 500000
sudo ip link set up can0
candump can0

Power on all nodes.

Now send --reset command to all units and see if all responded ('-d all' is to show all responds):

sudo ./co-master -i 1 9 5 7 --reset -d all
Node 7: Resetting...
Node 1: Resetting...
Node 5: Resetting...
Node 9: Resetting...
Node 7: The Reset Occurred.
Node 1: The Reset Occurred.
Node 9: The Reset Occurred.
Node 5: The Reset Occurred.


5. Read and Write OD settings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
a). Read manufacture device name as VS4 type (default):

sudo ./co-master -i 5 -R 1008 0
Node: 5, Index: 1008h, Sub: 00h,  VS=    G552

b). Read manufacture device name as U32 type:

sudo ./co-master -i 5 -R 1008 0 U32
Node: 5, Index: 1008h, Sub: 00h, U32=32353547h

c). Read manufacture device name from multiple nodes:

sudo ./co-master -i 1 9 5 7 -R 1008 0 -d all
Node: 9, Index: 1008h, Sub: 00h,  VS=    G552
Node: 5, Index: 1008h, Sub: 00h,  VS=    G552
Node: 7, Index: 1008h, Sub: 00h,  VS=    G55T
Node: 1, Index: 1008h, Sub: 00h,  VS=    G55T

d). Writes
The model 'G552PC70', nodes 5 in this Examples, has OD 2005h 00h. This OD applies sensor type parameters.
For example, when filter settings are changed the OD[2005h,00h] should be set to apply* these parameters.

sudo ./co-master -i 5 -f aver
sudo ./co-master -i 5 -W 2005 0 11

or

sudo ./co-master -i 5 --filter aver -W 2005 0 11
__________________________________________________________
*You can also use --apply command instead of -W 2005 0 11.

sudo ./co-master -i 5 --filter aver --apply 

or 

sudo ./co-master -i 5 --filter aver --apply 11


6. SAMPLE MODE
~~~~~~~~~~~~~~
The nodes default configuration is the sample mode. 
To get sampling with the existing configuration, the co-master has to be run with --samplemax (or -x) command that sets the sample number. The co-master will exit upon sample counter reaching this number.
If you want an endless output specify -x -1. The ctrl+C do break and exit.

Let's do sampling for 5 samples:

sudo ./co-master -i 1 -x 5

id:1,  sw:0,  hw:21005,  Gx:-0.192000,  Gy:-0.216000,  Gz:+0.040000,  Ax:+88.800003,  Ay:-179.000000,  Az:-983.400024,  Sun 2000-12-31 16:44:40.000
id:1,  sw:1,  hw:21006,  Gx:+0.088000,  Gy:-0.440000,  Gz:+0.048000,  Ax:+84.000000,  Ay:-179.000000,  Az:-979.600037,  Sun 2000-12-31 16:44:40.008
id:1,  sw:2,  hw:21007,  Gx:+0.296000,  Gy:-0.184000,  Gz:+0.000000,  Ax:+87.400002,  Ay:-178.400009,  Az:-976.200012,  Sun 2000-12-31 16:44:40.016
id:1,  sw:3,  hw:21008,  Gx:+0.200000,  Gy:-0.128000,  Gz:-0.064000,  Ax:+87.200005,  Ay:-179.000000,  Az:-976.799988,  Sun 2000-12-31 16:44:40.024
id:1,  sw:4,  hw:21009,  Gx:-0.120000,  Gy:-0.024000,  Gz:-0.056000,  Ax:+87.400002,  Ay:-177.199997,  Az:-980.000000,  Sun 2000-12-31 16:44:40.032


It is possible to call multiple nodes. You can try:
sudo ./co-master -i 1 9 5 7 -x 5

Or if you want to see outputs from all nodes on terminal:

sudo ./co-master -i 1 9 5 7 -x 5 -d all

You can use --log to get samples and node's settings into separate files:

sudo ./co-master -i 1 9 5 7 -x 5 --log

This run should create 4 log files with names based on model number, node id, date and time of co-master run start.
For an example:
G552 ID9 2021-10-08 16:32:13.386.csv
G552 ID5 2021-10-08 16:32:13.386.csv
G55T ID7 2021-10-08 16:32:13.386.csv
G55T ID1 2021-10-08 16:32:13.386.csv


7. TPDO and SYNC
~~~~~~~~~~~~~~~~
The --tpdo command has the following parameters: control, type and n.

A). 'control' parameter

The 'control' is optional parameter and if used must be after --tpdo.
The control parameter enables/disables tpdo on slaves by its number (Each tpdo 1, 2, 3 or 4 send different data). The TPDO with listed number gets enabled TPDO and not listed ones gets disabled:

Enable only TPDO1 and disable others:
sudo ./co-master -i 1 --tpdo 1 -x 5
id:1,  sw:0,  hw:19325,  Gx:-0.032000,  Gy:-0.128000,  Gz:-0.016000
id:1,  sw:1,  hw:19326,  Gx:-0.088000,  Gy:-0.128000,  Gz:-0.032000
id:1,  sw:2,  hw:19327,  Gx:-0.040000,  Gy:-0.096000,  Gz:-0.032000
id:1,  sw:3,  hw:19328,  Gx:-0.040000,  Gy:-0.064000,  Gz:-0.024000
id:1,  sw:4,  hw:19329,  Gx:+0.000000,  Gy:-0.080000,  Gz:-0.024000

Enable all TPDO 1,2,3,4 and disable others (order in tpdos does not matter):
sudo ./co-master -i 1 --tpdo 2143 -x 5
id:1,  sw:0,  hw:20858,  Gx:-0.024000,  Gy:-0.112000,  Gz:-0.040000,  Ax:+344.000000,  Ay:-154.199997,  Az:-932.600037,  ℃:+24.366770,  Sun 2000-12-31 16:31:37.084
id:1,  sw:1,  hw:20859,  Gx:-0.016000,  Gy:-0.104000,  Gz:-0.008000,  Ax:+343.800018,  Ay:-153.400009,  Az:-932.200012,  ℃:+24.366770,  Sun 2000-12-31 16:31:37.092
id:1,  sw:2,  hw:20860,  Gx:-0.080000,  Gy:-0.120000,  Gz:-0.016000,  Ax:+342.800018,  Ay:-153.600006,  Az:-931.400024,  ℃:+24.366770,  Sun 2000-12-31 16:31:37.100
id:1,  sw:3,  hw:20861,  Gx:-0.032000,  Gy:-0.136000,  Gz:+0.000000,  Ax:+342.000000,  Ay:-153.600006,  Az:-931.799988,  ℃:+24.366770,  Sun 2000-12-31 16:31:37.108
id:1,  sw:4,  hw:20862,  Gx:-0.064000,  Gy:-0.080000,  Gz:+0.024000,  Ax:+342.200012,  Ay:-153.400009,  Az:-931.600037,  ℃:+24.366770,  Sun 2000-12-31 16:31:37.116

Disable all TPDO:
sudo ./co-master -i 1 --tpdo 0 -x 5
This run will not do output. Press ctrl+C to exit.

The tpdos settings valid only for the current run if not saved. If you want to make the changes permanent use --save command.
sudo ./co-master -i 1 --tpdo 12 -x 5 --save
Node 1: Saving OD settings to non-volatile memory. The --reset command might be required to make the changes valid

id:1,  sw:0,  hw:21270,  Gx:-0.016000,  Gy:-0.128000,  Gz:-0.032000,  Ax:+341.600006,  Ay:-154.000000,  Az:-932.200012
id:1,  sw:1,  hw:21271,  Gx:-0.040000,  Gy:-0.120000,  Gz:-0.024000,  Ax:+341.600006,  Ay:-154.600006,  Az:-932.200012
id:1,  sw:2,  hw:21272,  Gx:-0.016000,  Gy:-0.096000,  Gz:-0.016000,  Ax:+341.200012,  Ay:-155.600006,  Az:-932.799988
id:1,  sw:3,  hw:21273,  Gx:-0.016000,  Gy:-0.096000,  Gz:-0.040000,  Ax:+341.399994,  Ay:-154.400009,  Az:-932.799988
id:1,  sw:4,  hw:21274,  Gx:-0.016000,  Gy:-0.120000,  Gz:-0.032000,  Ax:+341.800018,  Ay:-154.199997,  Az:-932.799988

Now you can run same (saved) parameters with a shorter call ('-x n' never gets saved):
sudo ./co-master -i 1 -x 5
id:1,  sw:0,  hw:21277,  Gx:-0.048000,  Gy:-0.056000,  Gz:-0.032000,  Ax:+342.800018,  Ay:-154.800003,  Az:-933.799988
id:1,  sw:1,  hw:21278,  Gx:+0.048000,  Gy:-0.104000,  Gz:+0.000000,  Ax:+341.800018,  Ay:-153.600006,  Az:-933.000000
id:1,  sw:2,  hw:21279,  Gx:+0.024000,  Gy:-0.040000,  Gz:+0.032000,  Ax:+341.200012,  Ay:-154.199997,  Az:-930.000000
id:1,  sw:3,  hw:21280,  Gx:+0.040000,  Gy:-0.120000,  Gz:-0.024000,  Ax:+340.800018,  Ay:-153.199997,  Az:-928.400024
id:1,  sw:4,  hw:21281,  Gx:-0.008000,  Gy:-0.080000,  Gz:-0.008000,  Ax:+341.399994,  Ay:-153.800003,  Az:-932.400024


B). 'type and n' parameter

The 'type' sets the Transmission Type that put nodes into sample or sync consumer mode.
The 'type' is optional parameter and can be either 'sync' or 'smpl' (default).

The 'n' is the value for the sync consumer mode: 
  00h=synchronous mode (by every SYNC message)
  01h-F0h=synchronous mode (by n times SYNC messages)

If 'n' set to 0 or 1 then nodes transmit samples on every sync received. Otherwise, it does on every n sync.

This run does not differ from the previous one as the sample mode is set by default:
sudo ./co-master -i 1 --tpdo smpl -x 5
id:1,  sw:0,  hw:21659,  Gx:-0.048000,  Gy:-0.104000,  Gz:-0.008000,  Ax:+341.800018,  Ay:-154.600006,  Az:-932.600037
id:1,  sw:1,  hw:21660,  Gx:-0.008000,  Gy:-0.128000,  Gz:+0.008000,  Ax:+341.600006,  Ay:-154.600006,  Az:-932.200012
id:1,  sw:2,  hw:21661,  Gx:-0.040000,  Gy:-0.112000,  Gz:+0.008000,  Ax:+342.000000,  Ay:-154.400009,  Az:-932.200012
id:1,  sw:3,  hw:21662,  Gx:-0.064000,  Gy:-0.088000,  Gz:-0.008000,  Ax:+342.800018,  Ay:-154.199997,  Az:-932.000000
id:1,  sw:4,  hw:21663,  Gx:-0.024000,  Gy:-0.136000,  Gz:-0.008000,  Ax:+341.399994,  Ay:-154.800003,  Az:-932.799988


C). SYNC (MASTER PRODUCER AND SLAVES CONSUMERS)
There must be a sync producer on CAN network to get samples from nodes that set to sync consumer mode.

NOTE: The following examples are for instructional purpose and not meant as practical usage cases. 
The run sets master as sync producer with sync interval 1000 ms and node ID 5 does transmission on every sync message (every 1 sec):

sudo ./co-master --sync 1000 -i 5 --tpdo 124 sync -x 5 --save
Node 5: Saving OD settings to non-volatile memory. The --reset command might be required to make the changes valid
Master SYNC producer: 1000 msec

id:5,  sw:0,  hw:46846,  Gx:+0.090900,  Gy:+0.166650,  Gz:-0.015150,  Ax:+118.800003,  Ay:+114.000000,  Az:+989.200012,  Wed 1969-12-31 23:25:46.712
id:5,  sw:1,  hw:46847,  Gx:-0.015150,  Gy:+0.000000,  Gz:-0.015150,  Ax:+117.599998,  Ay:+114.000000,  Az:+989.200012,  Sun 2021-10-10 18:21:32.712
id:5,  sw:2,  hw:46848,  Gx:+0.015150,  Gy:+0.060600,  Gz:-0.015150,  Ax:+118.400002,  Ay:+114.400002,  Az:+988.799988,  Sun 2021-10-10 18:21:33.712
id:5,  sw:3,  hw:46849,  Gx:-0.030300,  Gy:+0.000000,  Gz:-0.015150,  Ax:+118.400002,  Ay:+114.000000,  Az:+988.400024,  Sun 2021-10-10 18:21:34.712
id:5,  sw:4,  hw:46850,  Gx:-0.015150,  Gy:-0.015150,  Gz:-0.015150,  Ax:+118.400002,  Ay:+114.400002,  Az:+989.200012,  Sun 2021-10-10 18:21:35.712

The 'n' parameter can increase the sample transmission interval. This run with same master sync interval makes node ID 5 transmit samples every three seconds (on every third sync message from master):
*Note that --sync does not get --save for master.

sudo ./co-master --sync 1000 -i 5 --tpdo sync 3 -x 5
Master SYNC producer: 1000 msec

id:5,  sw:0,  hw:46852,  Gx:-0.030300,  Gy:-0.015150,  Gz:-0.015150,  Ax:+118.000000,  Ay:+114.400002,  Az:+991.200012,  Wed 1969-12-31 23:32:29.711
id:5,  sw:1,  hw:46853,  Gx:-0.030300,  Gy:-0.030300,  Gz:+0.000000,  Ax:+117.599998,  Ay:+113.599998,  Az:+989.600037,  Sun 2021-10-10 18:27:01.711
id:5,  sw:2,  hw:46854,  Gx:+0.015150,  Gy:+0.060600,  Gz:+0.000000,  Ax:+118.800003,  Ay:+114.400002,  Az:+989.600037,  Sun 2021-10-10 18:27:04.711
id:5,  sw:3,  hw:46855,  Gx:-0.015150,  Gy:+0.015150,  Gz:-0.015150,  Ax:+118.400002,  Ay:+114.800003,  Az:+988.799988,  Sun 2021-10-10 18:27:07.711
id:5,  sw:4,  hw:46856,  Gx:+0.015150,  Gy:+0.045450,  Gz:+0.000000,  Ax:+118.000000,  Ay:+114.400002,  Az:+989.600037,  Sun 2021-10-10 18:27:10.711


D). SYNC (SLAVE PRODUCER AND SLAVES CONSUMERS)
The slave node can be a sync producer for all nodes including itself. The slave node can be in a sample mode being a sync producer for other nodes.

This run makes node ID 1 be the sync producer while this node is in sample transmission mode by default. The node ID 5 is left in sync transmission from previous run (TPDO on every sync) that was saved (--save). The endless (-x -1) sample output is used here because the sample rate of the node ID 1 is 125 sps (default) and with -x 5 the node 1 would stop sync in 40 ms. So, '-x -1' lets the sync producer, node ID 1, to send sync messages to other node (5) till co-master terminated by ctrl+C. The --log creates sample log files you can compare the size and settings of both nodes.

sudo ./co-master -i 5 1 --sync 1000 -x -1 --log
Node 1: SYNC producer: 1000 msec

id:5,  sw:0,  hw:47099,  Gx:+0.075750,  Gy:+0.166650,  Gz:+0.000000,  Ax:+118.400002,  Ay:+114.000000,  Az:+987.600037,  Thu 1970-01-01 00:31:33.619
id:5,  sw:1,  hw:47100,  Gx:+0.121200,  Gy:+0.196950,  Gz:+0.000000,  Ax:+118.400002,  Ay:+113.599998,  Az:+985.200012,  Sun 2021-10-10 19:26:52.619
id:5,  sw:2,  hw:47101,  Gx:+0.090900,  Gy:+0.196950,  Gz:-0.030300,  Ax:+118.800003,  Ay:+114.800003,  Az:+988.799988,  Sun 2021-10-10 19:26:53.619
id:5,  sw:3,  hw:47102,  Gx:+0.181800,  Gy:+0.303000,  Gz:-0.015150,  Ax:+119.599998,  Ay:+114.000000,  Az:+989.200012,  Sun 2021-10-10 19:26:54.619
id:5,  sw:4,  hw:47103,  Gx:-0.166650,  Gy:-0.196950,  Gz:+0.030300,  Ax:+118.000000,  Ay:+115.599998,  Az:+989.600037,  Sun 2021-10-10 19:26:55.619
...
ctrl+C to exit

This run makes node ID 1 a sync producer (--sync) and put it in sync consumer mode (--tpdo sync). So, node 1 sends sync messages to the network and nodes 1 and 5 receive sync messages:
sudo ./co-master -i 5 1 --tpdo sync --sync 1000 -x 5
Node 1: SYNC producer: 1000 msec

id:5,  sw:0,  hw:48577,  Gx:+0.030300,  Gy:+0.075750,  Gz:+0.000000,  Ax:+118.000000,  Ay:+113.200005,  Az:+991.600037,  Thu 1970-01-01 00:57:52.773
id:5,  sw:1,  hw:48578,  Gx:+0.000000,  Gy:+0.030300,  Gz:-0.015150,  Ax:+118.000000,  Ay:+115.200005,  Az:+991.200012,  Sun 2021-10-10 19:51:37.773
id:5,  sw:2,  hw:48579,  Gx:-0.045450,  Gy:-0.045450,  Gz:+0.000000,  Ax:+118.000000,  Ay:+114.800003,  Az:+988.000000,  Sun 2021-10-10 19:51:38.773
id:5,  sw:3,  hw:48580,  Gx:+0.151500,  Gy:+0.378750,  Gz:+0.030300,  Ax:+117.599998,  Ay:+112.800003,  Az:+990.000000,  Sun 2021-10-10 19:51:39.773
id:5,  sw:4,  hw:48581,  Gx:-0.030300,  Gy:-0.015150,  Gz:+0.000000,  Ax:+117.599998,  Ay:+114.400002,  Az:+990.799988,  Sun 2021-10-10 19:51:40.773

Every node can be set and saved with different settings by a separate co-master run. Then all nodes can be run at same time with the saved settings (sample rates, filter, tpdo...). The --log is useful in such cases.


8. MASTER AND SLAVES HEARTBEAT
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The slave sends its heartbeat at every time slave's heartbeat interval. The master expects to receive slaves' heartbeat before the master time out interval.
So, to avoid errors the master's heartbeat interval should not be shorter than slaves' heartbeat interval.

For example, this run produces no error for heartbeats:
sudo ./co-master -i 5 -h 1000 -H 1500 -x 2000


But this one produces error on terminal and into a special .msg file:
sudo ./co-master -i 5 -h 1000 -H 500 -x 2000

This outputs you can see on your terminal:
Node: 5, [NMT state 0] [error code K] failed to boot: Heartbeat event during start error control service. No heartbeat message received from CANopen device during start error control service.
Node 5: The Reset Occurred.

If you miss it, then check if a file with .msg extension created:
G552 ID5 2021-10-11 17:26:25.708.msg

The content of the file:
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 
Node: 5, Heartbeat time out occurred: 1 


9. DUMP
~~~~~~~
The DUMP may have an optional parameter for some devices.*
The --dump gets output of object's settings to console or/and log file before sampling or without it.

sudo ./co-master -i 9 --dump --log -x 3
Index: 1000h, Sub: 00h, U32=00020194h :: Device Type
Index: 1001h, Sub: 00h,  U8=      00h :: Error Register
Index: 1002h, Sub: 00h, U32=00000000h :: Manufacturer Status Register
Index: 1005h, Sub: 00h, U32=00000080h :: SYNC COB-ID
Index: 1006h, Sub: 00h, U32=00002710h :: Communication Cycle Period
Index: 1008h, Sub: 00h,  VS=    G552  :: Manufacturer Device Name
Index: 1009h, Sub: 00h,  VS=    PC10  :: Manufacturer Hardware Version
Index: 100ah, Sub: 00h,  VS=    1.00  :: Manufacturer Software Version
Index: 1010h, Sub: 01h, U32=00000001h :: Save All Parameters
Index: 1011h, Sub: 01h, U32=00000001h :: Restore All Default Parameters
Index: 1012h, Sub: 00h, U32=80000100h :: TIME COB-ID
Index: 1017h, Sub: 00h, U16=    0000h :: Producer Heartbeat Time
Index: 1019h, Sub: 00h,  U8=      00h :: Synchronous Counter Overflow Value
Index: 1200h, Sub: 01h, U32=00000609h :: RSDO COB-ID
Index: 1200h, Sub: 02h, U32=00000589h :: TSDO COB-ID
Index: 1800h, Sub: 01h, U32=40000189h :: TPDO1 COB-ID
Index: 1801h, Sub: 01h, U32=40000289h :: TPDO2 COB-ID
Index: 1802h, Sub: 01h, U32=40000389h :: TPDO3 COB-ID
Index: 1803h, Sub: 01h, U32=40000489h :: TPDO4 COB-ID
Index: 1800h, Sub: 02h,  U8=      feh :: TPDO1 Transmission Type
Index: 1801h, Sub: 02h,  U8=      feh :: TPDO2 Transmission Type
Index: 1802h, Sub: 02h,  U8=      feh :: TPDO3 Transmission Type
Index: 1803h, Sub: 02h,  U8=      feh :: TPDO4 Transmission Type
Index: 1a00h, Sub: 01h, U32=21000010h :: TPDO1 Mapping 1 = Tc
Index: 1a00h, Sub: 02h, U32=71300110h :: TPDO1 Mapping 2 = Gx
Index: 1a00h, Sub: 03h, U32=71300210h :: TPDO1 Mapping 3 = Gy
Index: 1a00h, Sub: 04h, U32=71300310h :: TPDO1 Mapping 4 = Gz
Index: 1a01h, Sub: 01h, U32=21000010h :: TPDO2 Mapping 1 = Tc
Index: 1a01h, Sub: 02h, U32=71300410h :: TPDO2 Mapping 2 = Ax
Index: 1a01h, Sub: 03h, U32=71300510h :: TPDO2 Mapping 3 = Ay
Index: 1a01h, Sub: 04h, U32=71300610h :: TPDO2 Mapping 4 = Az
Index: 1a02h, Sub: 01h, U32=21000010h :: TPDO3 Mapping 1 = Tc
Index: 1a02h, Sub: 02h, U32=71300810h :: TPDO3 Mapping 2 = Temperature
Index: 1a02h, Sub: 03h, U32=71300910h :: TPDO3 Mapping 3 = RTD
Index: 1a02h, Sub: 04h, U32=20220110h :: TPDO3 Mapping 4 = STS
Index: 1a03h, Sub: 01h, U32=21000010h :: TPDO4 Mapping 1 = Tc
Index: 1a03h, Sub: 02h, U32=21010220h :: TPDO4 Mapping 2 = Ms
Index: 1a03h, Sub: 03h, U32=21010110h :: TPDO4 Mapping 3 = Dy
Index: 1f80h, Sub: 00h, U32=0000000ch :: NMT Startup Mode
Index: 2000h, Sub: 01h,  U8=      09h :: CAN Node-ID
Index: 2000h, Sub: 02h,  U8=      02h :: CAN Bitrate
Index: 2001h, Sub: 00h,  U8=      0ah :: Sensor Sample Rate
Index: 2005h, Sub: 00h,  U8=      20h :: Apply parameters
Index: 2020h, Sub: 01h,  U8=      01h :: Attitude control
Index: 2020h, Sub: 02h,  U8=      00h :: Attitude axis conversion
Index: 2020h, Sub: 03h,  U8=      00h :: Attitude motion profile
Index: 2100h, Sub: 00h, U16=    0138h :: Trigger Counter
Index: 2101h, Sub: 01h, U16=    35e6h :: Timestamp Day
Index: 2101h, Sub: 02h, U32=0ce87340h :: Timestamp Millisecond
Index: 6110h, Sub: 01h, U16=    28a1h :: AI Sensor Type 1
Index: 6110h, Sub: 02h, U16=    28a1h :: AI Sensor Type 2
Index: 6110h, Sub: 03h, U16=    28a1h :: AI Sensor Type 3
Index: 6110h, Sub: 04h, U16=    2905h :: AI Sensor Type 4
Index: 6110h, Sub: 05h, U16=    2905h :: AI Sensor Type 5
Index: 6110h, Sub: 06h, U16=    2905h :: AI Sensor Type 6
Index: 6110h, Sub: 07h, U16=    0064h :: AI Sensor Type 7
Index: 6110h, Sub: 08h, U16=    28a1h :: AI Sensor Type 8
Index: 6110h, Sub: 09h, U16=    28a1h :: AI Sensor Type 9
Index: 6110h, Sub: 0ah, U16=    28a1h :: AI Sensor Type 10
Index: 6131h, Sub: 01h, U32=00410300h :: AI Physical Unit PV 1
Index: 6131h, Sub: 02h, U32=00410300h :: AI Physical Unit PV 2
Index: 6131h, Sub: 03h, U32=00410300h :: AI Physical Unit PV 3
Index: 6131h, Sub: 04h, U32=fdf10000h :: AI Physical Unit PV 4
Index: 6131h, Sub: 05h, U32=fdf10000h :: AI Physical Unit PV 5
Index: 6131h, Sub: 06h, U32=fdf10000h :: AI Physical Unit PV 6
Index: 6131h, Sub: 07h, U32=002d0000h :: AI Physical Unit PV 7
Index: 6131h, Sub: 08h, U32=00000000h :: AI Physical Unit PV 8
Index: 6131h, Sub: 09h, U32=00000000h :: AI Physical Unit PV 9
Index: 6131h, Sub: 0ah, U32=00000000h :: AI Physical Unit PV 10
Index: 61a0h, Sub: 01h,  U8=      02h :: AI Filter Type 1
Index: 61a0h, Sub: 02h,  U8=      02h :: AI Filter Type 2
Index: 61a0h, Sub: 03h,  U8=      02h :: AI Filter Type 3
Index: 61a0h, Sub: 04h,  U8=      02h :: AI Filter Type 4
Index: 61a0h, Sub: 05h,  U8=      02h :: AI Filter Type 5
Index: 61a0h, Sub: 06h,  U8=      02h :: AI Filter Type 6
Index: 61a0h, Sub: 07h,  U8=      02h :: AI Filter Type 7
Index: 61a0h, Sub: 08h,  U8=      02h :: AI Filter Type 8
Index: 61a0h, Sub: 09h,  U8=      02h :: AI Filter Type 9
Index: 61a0h, Sub: 0ah,  U8=      02h :: AI Filter Type 10
Index: 61a1h, Sub: 01h,  U8=      08h :: AI Filter Setting Constant 1
Index: 61a1h, Sub: 02h,  U8=      08h :: AI Filter Setting Constant 2
Index: 61a1h, Sub: 03h,  U8=      08h :: AI Filter Setting Constant 3
Index: 61a1h, Sub: 04h,  U8=      08h :: AI Filter Setting Constant 4
Index: 61a1h, Sub: 05h,  U8=      08h :: AI Filter Setting Constant 5
Index: 61a1h, Sub: 06h,  U8=      08h :: AI Filter Setting Constant 6
Index: 61a1h, Sub: 07h,  U8=      08h :: AI Filter Setting Constant 7
Index: 61a1h, Sub: 08h,  U8=      08h :: AI Filter Setting Constant 8
Index: 61a1h, Sub: 09h,  U8=      08h :: AI Filter Setting Constant 9
Index: 61a1h, Sub: 0ah,  U8=      08h :: AI Filter Setting Constant 10
Index: 7130h, Sub: 01h, I16=    ffffh :: AI Input PV 1 (Gx)
Index: 7130h, Sub: 02h, I16=    ffffh :: AI Input PV 2 (Gy)
Index: 7130h, Sub: 03h, I16=    0000h :: AI Input PV 3 (Gz)
Index: 7130h, Sub: 04h, I16=    07b5h :: AI Input PV 4 (Ax)
Index: 7130h, Sub: 05h, I16=    feb3h :: AI Input PV 5 (Ay)
Index: 7130h, Sub: 06h, I16=    05e1h :: AI Input PV 6 (Az)
Index: 7130h, Sub: 07h, I16=    07c7h :: AI Input PV 7 (Temp)
Index: 7130h, Sub: 08h, I16=    fbb5h :: AI Input PV 8 (ANG1)
Index: 7130h, Sub: 09h, I16=    e296h :: AI Input PV 9 (ANG2)
Index: 7130h, Sub: 0ah, I16=    fa7ah :: AI Input PV 10 (-)

id:9,  sw:0,  hw:326,  Gx:+0.000000,  Gy:-0.045450,  Gz:-0.030300,  Ax:+789.200012,  Ay:-132.800003,  Az:+602.000000,  a1:-0.134155,  a2:-52.665649,  st:0x0000,  Thu 1970-01-01 01:51:27.165
id:9,  sw:1,  hw:327,  Gx:+0.015150,  Gy:+0.000000,  Gz:+0.000000,  Ax:+788.799988,  Ay:-132.800003,  Az:+602.400024,  a1:-0.134155,  a2:-52.665649,  st:0x0000,  Sun 2021-10-10 20:45:35.175
id:9,  sw:2,  hw:328,  Gx:+0.000000,  Gy:+0.015150,  Gz:-0.030300,  Ax:+788.799988,  Ay:-134.000000,  Az:+601.600037,  a1:-0.134155,  a2:-52.665649,  st:0x0000,  Sun 2021-10-10 20:45:35.185

_________________________________________________________________________________________________________________
*For such devices as M-A552AC1 that has User Defined Filter (UDF) there an optional parameter to DUMP the current UDF values in the working RAM area to console:

sudo ./co-master -i 1 --dump udf
Node: 1: UDF number of tap: 64

00000000h 00000001h 00000002h 00000003h 
00000004h 00000005h 00000006h 00000007h 
00000008h 00000009h 00000010h 00000011h 
00000012h 00000013h 00000014h 00000015h 
00000016h 00000017h 00000018h 00000019h 
00000020h 00000021h 00000022h 00000023h 
00000024h 00000025h 00000026h 00000027h 
00000028h 00000029h 00000030h 00000031h 
00000032h 00000033h 00000034h 00000035h 
00000036h 00000037h 00000038h 00000039h 
00000040h 00000041h 00000042h 00000043h 
00000044h 00000045h 00000046h 00000047h 
00000048h 00000049h 00000050h 00000051h 
00000052h 00000053h 00000054h 00000055h 
00000056h 00000057h 00000058h 00000059h 
00000060h 00000061h 00000062h 00000063h 


10. USER DEFINED FILTER
~~~~~~~~~~~~~~~~~~~~~~~
Some devices such as M-A552AC1 has UDF function that allows users to load FIR coefficients from file or non-volatilen memory.

Examples:

1. Load UDF from file fir2.txt to RAM and save to non-volatile memory:
sudo ./co-master -i 1 -f U512 fir2.txt save
Node: 1, UDF SAVE takes up to 27 sec...
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 
Node: 1, UDF command successfully completed


2. Load (after reset or powercycle) UDF from non-volatile memory to RAM, and output to console:
sudo ./co-master -i 1 -f u64 load --dump udf
Node: 1, UDF LOAD takes up to 15 sec...
1 2 
Node: 1, UDF command successfully completed
Node: 1: UDF number of tap: 64

004a4459h 004c9652h 0053281dh 005df137h 
006cddc1h 007fcea6h 009699e7h 00b10af8h 
00cee34ch 00efdaefh 0113a13fh 0139ddbfh 
016230ffh 018c3596h 01b7812fh 01e3a5a6h 
0210322bh 023cb472h 0268b9e7h 0293d0dfh 
02bd89ceh 02e57872h 030b34fbh 032e5d21h 
034e9530h 036b8904h 0384ece9h 039a7e6ah 
03ac050bh 03b952dch 03c244fbh 03c6c3f0h 
03c6c3f0h 03c244fbh 03b952dch 03ac050bh 
039a7e6ah 0384ece9h 036b8904h 034e9530h 
032e5d21h 030b34fbh 02e57872h 02bd89ceh 
0293d0dfh 0268b9e7h 023cb472h 0210322bh 
01e3a5a6h 01b7812fh 018c3596h 016230ffh 
0139ddbfh 0113a13fh 00efdaefh 00cee34ch 
00b10af8h 009699e7h 007fcea6h 006cddc1h 
005df137h 0053281dh 004c9652h 004a4459h 


3. Load UDF from file, save to non-volatile memory, apply and run:
sudo ./co-master -i 1 -f u512 win_coeff512_int_fc10.txt save --apply -x 10
Node: 1, UDF SAVE takes up to 27 sec...
1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 
Node: 1, UDF command successfully completed

id:1,  sw:0,  hw:30299,  Ax:+0.01284379,  Ay:+0.00208873,  Az:+1.00282931,  Sat 1983-12-31 23:03:09.872,  ℃:+26.663998
id:1,  sw:1,  hw:30300,  Ax:+0.01284158,  Ay:+0.00208825,  Az:+1.00283408,  Sat 1983-12-31 23:03:09.888,  ℃:+26.663998
id:1,  sw:2,  hw:30301,  Ax:+0.01284003,  Ay:+0.00208777,  Az:+1.00284028,  Sat 1983-12-31 23:03:09.904,  ℃:+26.663998
id:1,  sw:3,  hw:30302,  Ax:+0.01283848,  Ay:+0.00208753,  Az:+1.00284708,  Sat 1983-12-31 23:03:09.920,  ℃:+26.663998
id:1,  sw:4,  hw:30303,  Ax:+0.01283735,  Ay:+0.00208706,  Az:+1.00285482,  Sat 1983-12-31 23:03:09.936,  ℃:+26.663998
id:1,  sw:5,  hw:30304,  Ax:+0.01283628,  Ay:+0.00208682,  Az:+1.00286293,  Sat 1983-12-31 23:03:09.952,  ℃:+26.663998
id:1,  sw:6,  hw:30305,  Ax:+0.01283520,  Ay:+0.00208652,  Az:+1.00287127,  Sat 1983-12-31 23:03:09.968,  ℃:+26.663998
id:1,  sw:7,  hw:30306,  Ax:+0.01283431,  Ay:+0.00208640,  Az:+1.00287938,  Sat 1983-12-31 23:03:09.984,  ℃:+26.663998
id:1,  sw:8,  hw:30307,  Ax:+0.01283348,  Ay:+0.00208658,  Az:+1.00288701,  Sat 1983-12-31 23:03:10.000,  ℃:+26.663998
id:1,  sw:9,  hw:30308,  Ax:+0.01283258,  Ay:+0.00208688,  Az:+1.00289404,  Sat 1983-12-31 23:03:10.016,  ℃:+26.663998



11. APPLY PARAMETERS
~~~~~~~~~~~~~~~~~~~~
This command is for devices having object 2005h. The command applies changes and makes new parameters active.
The command might be used with a parameter or without. In case of using it without parameter the changes are applied for existing setting type.
The details vary for differrent devices and can be found in its specification.

Examples for M-A552AC1:

1. Apply new filter setting for unchanged sensor type:
sudo ./co-master -i 1 --apply -x 5 -f k64
Node: 1, Apply Parameters takes 1 sec...

id:1,  sw:0,  hw:12095,  Ax:+0.01514542,  Ay:+0.01601416,  Az:+1.00198555,  Mon 1984-01-02 05:43:44.048,  ℃:+26.732250
id:1,  sw:1,  hw:12096,  Ax:+0.01542789,  Ay:+0.01627445,  Az:+1.00310898,  Mon 1984-01-02 05:43:44.080,  ℃:+26.732250
id:1,  sw:2,  hw:12097,  Ax:+0.01539516,  Ay:+0.01610428,  Az:+1.00337350,  Mon 1984-01-02 05:43:44.112,  ℃:+26.732250
id:1,  sw:3,  hw:12098,  Ax:+0.01529133,  Ay:+0.01593953,  Az:+1.00289226,  Mon 1984-01-02 05:43:44.144,  ℃:+26.732250
id:1,  sw:4,  hw:12099,  Ax:+0.01543707,  Ay:+0.01619786,  Az:+1.00309908,  Mon 1984-01-02 05:43:44.176,  ℃:+26.732250

2. Now change the sensor type to Tilt Angle sensor:
sudo ./co-master -i 1 --apply 21 -x 5
Node: 1, Apply Parameters takes 1 sec...

id:1,  sw:0,  hw:15095,  Ix:+0.014694028,  Iy:+0.015160605,  Iz:+1.047197580,  Mon 1984-01-02 07:05:04.864,  ℃:+26.717083
id:1,  sw:1,  hw:15096,  Ix:+0.014868151,  Iy:+0.015114347,  Iz:+1.047197580,  Mon 1984-01-02 07:05:04.896,  ℃:+26.717083
id:1,  sw:2,  hw:15097,  Ix:+0.015216103,  Iy:+0.015452463,  Iz:+1.047197580,  Mon 1984-01-02 07:05:04.928,  ℃:+26.717083
id:1,  sw:3,  hw:15098,  Ix:+0.015293360,  Iy:+0.015583312,  Iz:+1.047197580,  Mon 1984-01-02 07:05:04.960,  ℃:+26.717083
id:1,  sw:4,  hw:15099,  Ix:+0.015115956,  Iy:+0.015483044,  Iz:+1.047197580,  Mon 1984-01-02 07:05:04.992,  ℃:+26.717083

3. Now change the sensor type back to Accelerometer:
sudo ./co-master -i 1 --apply 11 -x 5
Node: 1, Apply Parameters takes 1 sec...

id:1,  sw:0,  hw:16595,  Ax:+0.01455879,  Ay:+0.01589489,  Az:+1.00133216,  Mon 1984-01-02 07:16:08.480,  ℃:+26.720875
id:1,  sw:1,  hw:16596,  Ax:+0.01471555,  Ay:+0.01609200,  Az:+1.00172484,  Mon 1984-01-02 07:16:08.512,  ℃:+26.720875
id:1,  sw:2,  hw:16597,  Ax:+0.01490992,  Ay:+0.01590198,  Az:+1.00237036,  Mon 1984-01-02 07:16:08.544,  ℃:+26.720875
id:1,  sw:3,  hw:16598,  Ax:+0.01506925,  Ay:+0.01558161,  Az:+1.00234509,  Mon 1984-01-02 07:16:08.576,  ℃:+26.720875
id:1,  sw:4,  hw:16599,  Ax:+0.01537937,  Ay:+0.01573348,  Az:+1.00194025,  Mon 1984-01-02 07:16:08.608,  ℃:+26.720875


12. EXTRA
~~~~~~~~~
Let's load all default settings for Node ID 5 but keep its current ID (5) and canbitrate (500k).
It can be done by with one co-master run:

sudo ./co-master -i 5 --load --newid 5 --canbitrate 500k --save --reset
Restore All Factory Default Settings requested (--load)
Node ID change requested (--newid)
Can Bitrate requested (--canbitrate)
Node 5: Loading OD with factory default values
Node 5: ID changing to 5
Node 5: Changing CAN Bitrate. The bitrate of the can network must be changed as well.
Node 5: Saving OD settings to non-volatile memory
Node 5: Resetting...










