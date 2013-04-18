/*
  This example implements a very simple sine wave generator using
  the HiFi library to output audio to a codec or DAC.  The HiFi
  library uses the SSC peripheral in I2S mode to communicated with
  external converters.

  This sketch is free software; you can redistribute it and/or
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

#include <HiFi.h>

extern const int16_t sine[512];

uint16_t sineTblPtr;
int16_t sign;

void setup() {
 
  sineTblPtr = 0;
  sign = 1;
 
  // set codec into reset
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
 
  HiFi.begin();
 
  // Configure transmitter for 2 channels, external TK/TF clocks, 32 bit per
  // channel (data less than 32-bit is left justified in the 32 bit word, but
  // codec config needs 32 clocks per channel).
  HiFi.configureTx(HIFI_AUDIO_MODE_STEREO, HIFI_CLK_MODE_USE_EXT_CLKS, 32);
 
  // Register to be notified when the transmit register is ready for more data
  HiFi.onTxReady(codecTxReadyInterrupt);
  
  // release codec from reset
  digitalWrite(7, HIGH);
 
  // Enable both receiver and transmitter.
  HiFi.enableTx(true);
}

void loop() {
}

void codecTxReadyInterrupt(HiFiChannelID_t channel)
{
  HiFi.write((sign * sine[sineTblPtr])<<16);

  if (channel ==  HIFI_CHANNEL_ID_2)
  {
    if (sineTblPtr == ((sizeof(sine)/sizeof(sine[0])) - 1))
    {
      sign *= -1;
      sineTblPtr=0;
     }
     else
     {
      sineTblPtr++;
     }
  }
}
