/*
  This example uses the HiFi library to connect with a Cirrus CS4271 audio
  codec.  The codec has been configured to generate all the clocks and the 
  Arduino will sync to them.  The codec is using I2S mode to communicate
  with the SSC peripheral.
  
  The sketch takes any data is receives from the codec and sends it right
  back out. 
  
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

void codecTxReadyInterrupt(HiFiChannelID_t);
void codecRxReadyInterrupt(HiFiChannelID_t);

static uint32_t ldat = 0;
static uint32_t rdat = 0;

void setup() {
 
  // set codec into reset
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
 
  HiFi.begin();
 
  // Configure transmitter for 2 channels, external TK/TF clocks, 32 bit per
  // channel (data less than 32-bit is left justified in the 32 bit word, but
  // codec config needs 32 clocks per channel).
  HiFi.configureTx(HIFI_AUDIO_MODE_STEREO, HIFI_CLK_MODE_USE_EXT_CLKS, 32);
 
  // Same config as above, except sync the receiver to transmitter (RK/RF
  // clock signals not needed)
  HiFi.configureRx(HIFI_AUDIO_MODE_STEREO, HIFI_CLK_MODE_USE_TK_RK_CLK, 32);

  // Since we've decided to sync the receiver to the transmitter, we could 
  // handle both reading and writing in a single interrupt (receive or 
  // transmit).  This example uses both just for demonstration purposes.
  HiFi.onTxReady(codecTxReadyInterrupt);
  HiFi.onRxReady(codecRxReadyInterrupt);
 
  // release codec from reset
  digitalWrite(7, HIGH);
 
  // Enable both receiver and transmitter.
  HiFi.enableRx(true);
  HiFi.enableTx(true);
}

void loop() {
}

void codecTxReadyInterrupt(HiFiChannelID_t channel)
{
  if (channel == HIFI_CHANNEL_ID_1)
  {
    // Left channel
    HiFi.write(ldat);  
  }
  else
  {
    // Right channel
    HiFi.write(rdat);
  }
}

void codecRxReadyInterrupt(HiFiChannelID_t channel)
{
  if (channel == HIFI_CHANNEL_ID_1)
  {
    // Left channel
    ldat = HiFi.read();
  }
  else
  {
    // Right channel
    rdat = HiFi.read();
  }
}

