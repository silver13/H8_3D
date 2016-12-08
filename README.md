# Eachine H8 mini 3D firmware

*board warning: some boards use an unsupported cpu. Check the cpu markings for St logo or stm32F031
The unsupported cpu can't be connected to or erased*

###Hardware information
The eachine H8 3D uses a STM32F031 processor with 16K flash, MPU 6052 gyro and XN297 radio.
It also has a 5V step-up onboard, followed by a 2.8V linear reg. Because of this, care should be taken not too overdischarge the battery, as the quad will not reset until the battery will become very low.

For this reason, the firmware currently uses the "#define LVC_PREVENT_RESET" to reduce the throttle when the battery is too low.

Do not change the motor pins as setting them incorrectly will break the board.


###Installation and Support
*Flashing instructions same as the CG023 in the link below*

Currently this port is covered by the CG023 thread on rcgroups.
http://www.rcgroups.com/forums/showthread.php?t=2634611#post34381034


###Radio protocol:
Stock H8 3d protocol, the rate switch cycles between 2 level modes (high / low rates) and 1 acro ( high rates)

For Deviation Tx / module please use Bayang protocol, as it has better accuracy, and also has more on/off channels.


###Accelerometer calibration:
Move the pitch stick down 3 times within about 2 seconds. Needs to be done on a level surface. Saved so it only needs to be done once. You may need to use high rates in order to reach the treshold. High rates only right now.


###Differences from H8 version:
 * the quadcopter rate ( in deg/sec) is no longer multiplied by 2, so it's the actual rate with devo.
 * acro only version can be compiled by enabling respective setting in config.h

###Motor cut
The board includes a step-up regulator and to prevent battery damage the motors will stop working when a voltage treshhold is reached.

###Linux/gcc version
The gcc version compiles close to 16k, and may need turning off features in order to make it fit. Read install.md for flashing information.

###Wiki
http://sirdomsen.diskstation.me/dokuwiki/doku.php?id=start

###Board images
Check the boards are identical with the images below

**the JJRC H22 board is not the same!**

<a href="/img/IMAG0520res.jpg" target="_blank"><img src="/img/IMAG0520res.jpg" alt="Loading" width="240" height="240" border="10" /></a>

<a href="/img/IMAG0522res.jpg" target="_blank"><img src="/img/IMAG0522res.jpg" alt="Loading" width="240" height="240" border="10" /></a>

### Purchase
http://www.banggood.com/Eachine-H8-3D-Mini-CF-Mode-2_4G-4CH-6Axis-RC-Quadcopter-RTF-p-990494.html
You can purchase one from banggood.
###History:

####21.08.16
* update and added stock protocol

####07.08.16
* initial code posted

###Old CG023 History:

####03.08.16
* added bluetooth beacon functionality

####05.07.16
* software i2c optimizations (speed/size)

####01.07.16
* added hw i2c, it can work at full speed in some cases where softi2c needs a slowdown
* pins PB6 and PB7 by default for hw i2c

####30.06.16
* removed cpu speed dependancies in pwm and systimer routines
* added overclock option to 64Mhz from 48

####27.06.16
* added serial support (tx) on SWCLK pin
* added osd output using LTM protocol
* changed delay() so it's actually close to what it's supposed to be
 
####21.05.16
* added esc driver
* pwm frequency now works to 185Hz
* default protocol changed to H8mini

####09.05.16
* added extra pwm pins from H8 port( A4,A6,A7,B0,B1 )
* i2c speed setting from D1 port
* GPIOF pins can now be used

####29.04.16
* added motor curves
* exp optimization merged from user stawel

####21.04.16
* added CX-10 (blue) protocol

####19.04.16
* adc input fix
* added invsqrt to imu (from quake) 

####19.04.16
* merged gcc support by Paweł Stawicki
* merged H7 protocol support by Paweł Stawicki

####13.04.16
* fixed bug setting motor to PA11

####03.04.16
* added automatic flips
* flash saving optimization 

####01.04.16
* flash saving optimizations
* headless fixed in both modes
* pwm frequency can be set now

####29.03.16
* moved pin setting to hardware.h
* level pid limit change ( LEVEL_MAX_RATE now acts as hardware limit)

####26.03.16
* added pwm defines
* i2c speed improvement

####23.03.16
* some optimizations, etc

####20.03.16:
* dual mode added
* added alternate led "battery low 2" ( 3.3V )

####20.03.16:
* CG023 stock tx protocol added



