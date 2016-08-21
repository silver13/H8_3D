/*
The MIT License (MIT)

Copyright (c) 2016 silverx

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include "binary.h"
#include "drv_spi.h"

#include "project.h"
#include "xn297.h"
#include "drv_time.h"
#include <stdio.h>
#include "config.h"
#include "defines.h"

#include "rx_bayang.h"

#include "util.h"

#ifdef RX_H8_3D_PROTOCOL

// compatibility with older version hardware.h
#if ( !defined RADIO_XN297 && !defined RADIO_XN297L)
#define RADIO_XN297
#endif

#define BAYANG_LOWRATE_MULTIPLIER 1.0


// packet period in uS
#define PACKET_PERIOD 3000

// was 250 ( uS )
#define PACKET_OFFSET 500


// how many times to hop ahead if no reception
#define HOPPING_NUMBER 4

// use with caution, other people may be in the area
//#define LOCK_WITHOUT_BIND

#ifdef LOCK_WITHOUT_BIND
#warning LOCK_WITHOUT_BIND enabled
#endif

// do not change below
///////////////////////



#define PAYLOAD_LEN 20

#define RF_CHAN_NUMBER 4


extern float rx[4];
extern char aux[AUXNUMBER];
extern char lastaux[AUXNUMBER];
extern char auxchange[AUXNUMBER];

uint8_t txid[4];

void writeregs ( uint8_t data[] , uint8_t size )
{
spi_cson();
for ( uint8_t i = 0 ; i < size ; i++)
{
	spi_sendbyte( data[i]);
}
spi_csoff();
delay(1000);
}


void rx_init()
{

#ifdef RADIO_XN297
static uint8_t bbcal[6] = { 0x3f , 0x4c , 0x84 , 0x6F , 0x9c , 0x20  };
writeregs( bbcal , sizeof(bbcal) );

// new
static uint8_t rfcal[8] = { 0x3e , 0xc9 , 0x9a , 0xA0 , 0x61 , 0xbb , 0xab , 0x9c };
writeregs( rfcal , sizeof(rfcal) );

static uint8_t demodcal[6] = { 0x39 , 0x0b , 0xdf , 0xc4 , 0xa7 , 0x03};
writeregs( demodcal , sizeof(demodcal) );
#endif

static int rxaddress[5] =  { 0xc4, 0x57, 0x09, 0x65, 0x21};
xn_writerxaddress( rxaddress);

	xn_writereg( EN_AA , 0 );	// aa disabled
	xn_writereg( EN_RXADDR , 1 ); // pipe 0 only
	xn_writereg( RF_SETUP , B00000001);  // lna high current on ( better performance )
	xn_writereg( RX_PW_P0 , PAYLOAD_LEN ); // payload size
	xn_writereg( SETUP_RETR , 0 ); // no retransmissions ( redundant?)
	xn_writereg( SETUP_AW , 3 ); // address size (5 bits)
	xn_command( FLUSH_RX);
  xn_writereg( RF_CH ,  0x06  );  // bind channel

#ifdef RADIO_XN297
  xn_writereg( 0 , B00001111 ); // power up, crc enabled
#endif

#ifdef RADIO_XN297L
  xn_writereg( 0 , B10001111 ); // power up, crc enabled
#endif

#ifdef RADIO_CHECK
void check_radio(void);
 check_radio();
#endif	
}


void check_radio()
{	
	int temp = xn_readreg( 0x0f); // rx address pipe 5	
	// should be 0xc6
	extern void failloop( int);
	if ( temp != 0xc6) failloop(3);
}


static char checkpacket()
{
	spi_cson();
	int status = spi_sendzerorecvbyte();
	spi_csoff();
	if ( status&(1<<MASK_RX_DR) )
	{	 // rx clear bit
		// this is not working well
	 // xn_writereg( STATUS , (1<<MASK_RX_DR) );
		//RX packet received
		//return 1;
	}
	if( (status & B00001110) != B00001110 )
	{
		// rx fifo not empty		
		return 2;	
	}
	
  return 0;
}

int rxdata[PAYLOAD_LEN];
int fail = 0;
int fail2 = 0;
int fail3 = 0;

static int decodepacket( void)
{
	if ( rxdata[0] == 0x13 )
	{
		 
	  for(int i = 0 ; i < 4; i++)
			{
				// if the txid is different we discard the packet
				if ( txid[i] != rxdata[1+i]) return 0;
			}
		// check for bind packets and ignore those too
	  if (  rxdata[5] == 0x00
					&& rxdata[6] == 0x00
					&& rxdata[7] == 0x01 )
					{
						fail++;;
						return 0;
					}
		// check normal header
		if (  rxdata[5] < 4
					&& rxdata[6] == 0x08
					&& rxdata[7] == 0x03 )
					{
						// check sum if it matches
						int sum = 0;
						for (int i=9; i < 19; i++) sum += rxdata[i];
						if ( (uint8_t) sum == rxdata[19] )
						{
						rx[0] = mapf ( rxdata[12], 0x43 , 0xbb , 1.0f , -1.0f );
						rx[1] = mapf ( rxdata[11], 0x43 , 0xbb , -1.0f , 1.0f );

					// yaw
						if ( rxdata[10] <= 0x3c && rxdata[10] >= 0x0) 
							rx[2] = mapf ( rxdata[10], 0 , 0x3c , 0.0f , 1.0f );
						else if ( rxdata[10] <= 0xbc && rxdata[10] >=  0x80 )
							rx[2] = mapf ( rxdata[10], 0x80 , 0xbc , 0.0f , -1.0f );
						else rx[2] = 0;
					// throttle		
						rx[3] = mapf ( rxdata[9], 0 , 255 , 0.0f , 1.0f );
					
#ifndef DISABLE_EXPO
						rx[0] = rcexpo ( rx[0] , EXPO_XY );
						rx[1] = rcexpo ( rx[1] , EXPO_XY ); 
						rx[2] = rcexpo ( rx[2] , EXPO_YAW ); 	
#endif
			

						
						aux[CH_FLIP] = (rxdata[17] & 0x01) ? 1 : 0;		
if ((rxdata[17] & 0x02) ? 1 : 0) 
{
// rate mid
	aux[CH_VID] = 1;
	aux[CH_EXPERT] = 1;
}	
else
if ((rxdata[17] & 0x04) ? 1 : 0 )
{
// rate high
	aux[CH_VID] = 0;
	aux[CH_EXPERT] = 1;
}	
else
{
// rate low
	aux[CH_VID] = 1;
	aux[CH_EXPERT] = 0;
}
						aux[CH_EXPERT] = (rxdata[17] & 0x02) ? 1 : 0;	// rate mid		 // high 0x04 			
						aux[CH_HEADFREE] = (rxdata[17] & 0x10) ? 1 : 0;	// headless
						aux[CH_RTH] = 	(rxdata[17] & 0x20) ? 1 : 0;	// rth
						aux[CH_PIC] = 	(rxdata[17] & 0x08) ? 1 : 0;	// led

						for ( int i = 0 ; i < AUXNUMBER - 2 ; i++)
						{
							auxchange[i] = 0;
							if ( lastaux[i] != aux[i] ) auxchange[i] = 1;
							lastaux[i] = aux[i];
						}
			return 1;	// valid packet	
			}
			fail2++;
			return 0; // sum bad
		} // header incorrect
		fail3++;
		return 0; // 
		}
	return 0; // first byte different
}


  uint8_t rfchannel[4];
	int rxaddress[5];
	int rxmode = 0;
	int chan = 0;

void nextchannel()
{
	chan++;
	if (chan > 3 ) chan = 0;
	xn_writereg(0x25, rfchannel[chan] );
}



unsigned long failsafetime;


int failsafe = 0;

//#define RXDEBUG

#ifdef RXDEBUG	
/*
struct rxdebug
	{
	unsigned long packettime;
	int failcount;
	int packetpersecond;
	int channelcount[4];
	} 
	rxdebug;
*/
struct rxdebug rxdebug;
int packetrx;

unsigned int secondtimer;
#warning "RX debug enabled"
int skipstats[HOPPING_NUMBER + 2];
int afterskip[HOPPING_NUMBER + 2];
#endif

int bind_ch = 6;
unsigned int last_bind_channel_time;

unsigned int skipchannel = 0;
int lastrxchan;
int timingfail = 0;
unsigned int lastrxtime;

void checkrx( void)
{
	int packetreceived =	checkpacket();
	 
unsigned int time1 = gettime(); 
	
	if (   rxmode == RXMODE_BIND && time1 - last_bind_channel_time > 5000)
	{
		// bind channel hopping
		last_bind_channel_time = time1;
		xn_writereg(0x25, bind_ch );
		bind_ch++;	
		if ( bind_ch > 21 ) bind_ch = 6; 
		
	}
	
		if ( packetreceived ) 
		{ 
			if ( rxmode == RXMODE_BIND)
			{	// rx startup , bind mode
				xn_readpayload( rxdata , PAYLOAD_LEN);	
				// normal bind
				if ( (rxdata[0] == 0x13 
					&& rxdata[5] == 0x00
					&& rxdata[6] == 0x00
					&& rxdata[7] == 0x01 )
				#ifdef LOCK_WITHOUT_BIND	
				|| 
				( rxdata[5] < 4
					&& rxdata[6] == 0x08
					&& rxdata[7] == 0x03 )
				#endif
				) 

				{// bind packet
					
					for(int i = 0 ; i < 4; i++)
					{
						txid[i] = rxdata[1+i];
					}

					for(int i=0; i<4; i++)
					{
					rfchannel[i] = 6 + (0x0f*i) + (((txid[i] >> 4) + (txid[i] & 0x0f)) % 0x0f);
					}
					rxmode = RXMODE_NORMAL;				

				  // skip to another channel
					nextchannel();
					
					#ifdef SERIAL_INFO	
					printf( " BIND \n");
					#endif
				}
			}
			else
		    {		// normal mode  
#ifdef RXDEBUG
			    rxdebug.channelcount[chan]++;
			    rxdebug.packettime = gettime() - lastrxtime;
					
					if ( skipchannel&& !timingfail ) afterskip[skipchannel]++;
					if ( timingfail ) afterskip[0]++;

#endif

unsigned long temptime = gettime();
	
			    nextchannel();

			    xn_readpayload(rxdata, PAYLOAD_LEN);
					
			    int pass = decodepacket();

			    if (pass)
			      {
#ifdef RXDEBUG
				      packetrx++;
#endif
							skipchannel = 0;
							timingfail = 0;
							lastrxchan = chan;
							lastrxtime = temptime;
				      failsafetime = temptime;
				      failsafe = 0;
			      }
			    else
			      {
#ifdef RXDEBUG
				      rxdebug.failcount++;
#endif
			      }

		    }		// end normal rx mode

	  }			// end packet received
	
	unsigned long time = gettime();

		
	// sequence period 12000
	if (time - lastrxtime > (HOPPING_NUMBER*PACKET_PERIOD + 1000) && rxmode != RXMODE_BIND)
	  {			
			//  channel with no reception   
		  lastrxtime = time;
			// set channel to last with reception
			if (!timingfail) chan = lastrxchan;
			// advance to next channel
		  nextchannel();
			// set flag to discard packet timing
			timingfail = 1;
	  }
		
	if ( !timingfail && skipchannel < HOPPING_NUMBER+1 && rxmode != RXMODE_BIND)
		{
			unsigned int temp = time - lastrxtime ;

			if ( temp > 1000 && ( temp - (PACKET_OFFSET) )/((int) PACKET_PERIOD) >= (skipchannel + 1) ) 
			{
				nextchannel();
#ifdef RXDEBUG				
				skipstats[skipchannel]++;
#endif				
				skipchannel++;
			}
		}	
	
	if (time - failsafetime > FAILSAFETIME)
	  {	//  failsafe
		  failsafe = 1;
		  rx[0] = 0;
		  rx[1] = 0;
		  rx[2] = 0;
		  rx[3] = 0;
	  }
#ifdef RXDEBUG
	if (gettime() - secondtimer > 1000000)
	  {
		  rxdebug.packetpersecond = packetrx;
		  packetrx = 0;
		  secondtimer = gettime();
	  }
#endif

}

#endif




