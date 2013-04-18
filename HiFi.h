/*
  Parallel.h

  This library implements a limited functionality parallel port on the Arduino
  DUE board (ARM Cortex-M3 based).  
  
  The DUE board pins out the data bus on the extended digital headers along 
  with the control signals NC1 and NWR.  Some of the address signals are 
  connected to the PWM pins (A0-A5), but a full address bus is unavailable.  
  There is also conflict between the SS1 pin for SPI and the NRD signal used 
  for the parallel bus.  In short the hardware wasn't designed for use with
  external parallel memories.  
  
  The library does allow connection to some of the lower resolution LCD
  controllers that used index addressing and can speed up read/write times 
  considerably.
  
  To use this library, place the files in a folder called 'Parallel' under the 
  libraries directory in your sketches folder.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef HIFI_H
#define HIFI_H

#include "Arduino.h"
#include "ssc.h"

typedef enum
{
  HIFI_AUDIO_MODE_MONO_LEFT,
  HIFI_AUDIO_MODE_MONO_RIGHT,
  HIFI_AUDIO_MODE_STEREO
} HiFiAudioMode_t;

typedef enum
{
  HIFI_CLK_MODE_USE_EXT_CLKS,
  HIFI_CLK_MODE_USE_TK_RK_CLK
} HiFiClockMode_t;

typedef enum
{
  // in MONO modes, channel 1 will be the only one used.  
  // In STEREO modes, channel 1 is left, channel 2 is right
  HIFI_CHANNEL_ID_1,
  HIFI_CHANNEL_ID_2
} HiFiChannelID_t;

class HiFiClass {
public:
  HiFiClass() { };  
  void begin();
    
  void configureTx(HiFiAudioMode_t busMode,
          HiFiClockMode_t clkMode,
          uint8_t bitsPerChannel );
  void enableTx(bool enable);
  void onTxReady(void(*)(HiFiChannelID_t));
  
  void configureRx(HiFiAudioMode_t busMode,
          HiFiClockMode_t clkMode,
          uint8_t bitsPerChannel );
  void enableRx(bool enable);
  void onRxReady(void(*)(HiFiChannelID_t));
  
  void write(uint32_t value)
  {
    *(_dataOutAddr) = value;
  }
  
  uint32_t read()
  {
    return *(_dataInAddr);
  }
  
  // Interrupt handler function
  void onService(void);

private:
  uint32_t *_dataOutAddr;
  uint32_t *_dataInAddr;
  
  // Callback user functions
  void (*onTxReadyCallback)(HiFiChannelID_t channel);
  void (*onRxReadyCallback)(HiFiChannelID_t channel);
};

extern HiFiClass HiFi;

#endif
