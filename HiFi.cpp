/*
  HiFi.cpp

  This library implements a driver for the SSC peripheral on the Arduino
  DUE board (ARM Cortex-M3 based).  This can be used to interface with external
  high quality ADCs, DACs, or CODECs instead of the limited bit-depth 
  converters that are on the SAM3X.
  
  Currently, only I2S mode is supported, although the SSC peripheral can 
  support left-justified, right justified and TDM (>2 channels) modes.  
  These modes could be added, but were left out for simplicity.  Most codecs
  support I2S mode and it doesn't really have many options which reduces
  overall complexity.  This seems like a good compromise for the Arduino 
  platform.   
  
  You can configure the library to transmit, receive, or both, but only in slave
  mode.  The SAM3X doesn't appear to  have the capability of driving an MCLK
  signal that is synchronous with the TK/RK and TF/RF signals, which is needed
  by many of the audio converters.  Additionally, it doesn't seem to be able to
  generate clocks that are multiples of standard audio sampling rates (44.1k, 
  48k, etc.).  It's simpler to use an external clock master and just set the 
  SSC to slave to that.  That allows the playback/recording of wave files that
  can be generated/rendered on other systems easily.
  
  For transmit and receive, there are 2 clock signals needed - frame clock and 
  bit clock.  As mentioned above, these are inputs to the SSC.  If both 
  transmit and receive are desired, one side can be synchronized to the other 
  to reduce the overall pin count (i.e. don't need separate receive and 
  transmit clocks).  Although the SSC does support independent transmit and 
  receive rates if needed (e.g. separate input and output sample rates).
  
  To use this library, place the files in a folder called 'HiFi' under the 
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

#include "HiFi.h"

// Make sure that data is first pin in the list.
const PinDescription SSCTXPins[]=
{
  { PIOA, PIO_PA16B_TD,  ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT, PIN_ATTR_DIGITAL,  NO_ADC, NO_ADC, NOT_ON_PWM,  NOT_ON_TIMER },  // A0
  { PIOA, PIO_PA15B_TF,  ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT, PIN_ATTR_DIGITAL,  NO_ADC, NO_ADC, NOT_ON_PWM,  NOT_ON_TIMER },  // PIN 24
  { PIOA, PIO_PA14B_TK,  ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT, PIN_ATTR_DIGITAL,  NO_ADC, NO_ADC, NOT_ON_PWM,  NOT_ON_TIMER },  // PIN 23
};  

// Make sure that data is first pin in the list.
const PinDescription SSCRXPins[]=
{
  { PIOB, PIO_PB18A_RD,  ID_PIOB, PIO_PERIPH_A, PIO_DEFAULT, PIN_ATTR_DIGITAL,  NO_ADC, NO_ADC, NOT_ON_PWM,  NOT_ON_TIMER },  // A9
  { PIOB, PIO_PB17A_RF,  ID_PIOB, PIO_PERIPH_A, PIO_DEFAULT, PIN_ATTR_DIGITAL,  NO_ADC, NO_ADC, NOT_ON_PWM,  NOT_ON_TIMER },  // A8
  { PIOB, PIO_PB19A_RK,  ID_PIOB, PIO_PERIPH_A, PIO_DEFAULT, PIN_ATTR_DIGITAL,  NO_ADC, NO_ADC, NOT_ON_PWM,  NOT_ON_TIMER },  // A10
};

void HiFiClass::begin(void)
{
  // Enable module
  pmc_enable_periph_clk(ID_SSC);
  ssc_reset(SSC);
  
  _dataOutAddr = (uint32_t *)ssc_get_tx_access(SSC);
  _dataInAddr = (uint32_t *)ssc_get_rx_access(SSC);
  
  // Enable SSC interrupt line from the core
  NVIC_DisableIRQ(SSC_IRQn);
  NVIC_ClearPendingIRQ(SSC_IRQn);
  NVIC_SetPriority(SSC_IRQn, 0);  // most arduino interrupts are set to priority 0.
  NVIC_EnableIRQ(SSC_IRQn);
}

void HiFiClass::configureTx( HiFiAudioMode_t audioMode,
                HiFiClockMode_t clkMode,
                uint8_t bitsPerChannel )
{
  clock_opt_t tx_clk_option;
  data_frame_opt_t tx_data_frame_option;
  
  memset((uint8_t *)&tx_clk_option, 0, sizeof(clock_opt_t));
  memset((uint8_t *)&tx_data_frame_option, 0, sizeof(data_frame_opt_t));
  
  ///////////////////////////////////////////////////////////////////////////
  /// Transmitter IO Pin configuration
  ///////////////////////////////////////////////////////////////////////////
  uint8_t endpin = sizeof(SSCTXPins)/sizeof(SSCTXPins[0]);
  if (clkMode == HIFI_CLK_MODE_USE_TK_RK_CLK)
  {
    endpin = 1;
  }
  
  for (int i=0; i < endpin; i++)
  {
    PIO_Configure(SSCTXPins[i].pPort,
      SSCTXPins[i].ulPinType,
      SSCTXPins[i].ulPin,
      SSCTXPins[i].ulPinConfiguration);
  }
  
  // Note: there is a function in the Atmel ssc driver for configuration of
  // the peripheral in I2S mode, but it is incomplete and buggy.  This library
  // will configure the SSC directly which will also shed some light on the
  // various configuration parameters should a user need something slightly
  // different.

  ///////////////////////////////////////////////////////////////////////////
  /// Transmitter clock mode configuration
  ///////////////////////////////////////////////////////////////////////////
  switch (clkMode)
  {      
    case HIFI_CLK_MODE_USE_EXT_CLKS:
      // Use clocks on the TK/TF pins
      // Despite what this looks like, it actually means use the TK clock
      // pin.  There is both an error in the documentation and the macro
      // definition supplied by Atmel in the ssc.h file.  The document author
      // just cut and paste and the Atmel engineer decided to keep the
      // macro consistent with the data sheet (or didn't pay attention...).
      // I imagine that this will get fixed at some point, so this may need
      // to be revisited.
      tx_clk_option.ul_cks = SSC_TCMR_CKS_RK;
      
      if (audioMode == HIFI_AUDIO_MODE_MONO_RIGHT)
      {
        // high level on the frame clock is right channel in
        // I2S.  If we're only using the right channel, then
        // we can set start condition for right-only
        tx_clk_option.ul_start_sel = SSC_TCMR_START_RF_RISING;  
      }
      else
      {
        // stereo or mono-left will start in the left channel slot
        tx_clk_option.ul_start_sel = SSC_TCMR_START_RF_FALLING;
      }
      break;
    
    case HIFI_CLK_MODE_USE_TK_RK_CLK:
      // Despite what this looks like, it actually means use the RK clock
      // pin.  There is both an error in the documentation and the macro
      // definition supplied by Atmel in the ssc.h file.  The document author
      // just cut and paste and the Atmel engineer decided to keep the
      // macro consistent with the data sheet (or didn't pay attention...).
      // I imagine that this will get fixed at some point, so this may need
      // to be revisited. See the external clock case above.
      tx_clk_option.ul_cks = SSC_TCMR_CKS_TK;
    
      // Use the receiver's configuration as the start trigger for
      // transmit.  The receiver must be configured and running in
      // order for this to work.
      tx_clk_option.ul_start_sel = SSC_TCMR_START_RECEIVE;
      break;
  }
  
  // No output clocks
  tx_clk_option.ul_ckg = SSC_TCMR_CKG_NONE;
  tx_clk_option.ul_period = 0;  // we're not master -- set to 0
  tx_clk_option.ul_cko = SSC_TCMR_CKO_NONE;
  tx_clk_option.ul_cki = 0;
  // I2S has a one bit delay on the data.
  tx_clk_option.ul_sttdly = 1;
  
  ///////////////////////////////////////////////////////////////////////////
  /// Transmitter frame mode configuration.
  ///////////////////////////////////////////////////////////////////////////
  tx_data_frame_option.ul_datlen = bitsPerChannel - 1;
  tx_data_frame_option.ul_msbf = SSC_TFMR_MSBF;

  // number of channels
  if (audioMode == HIFI_AUDIO_MODE_STEREO) 
  {
    tx_data_frame_option.ul_datnb = 1;
  } 
  else 
  {
    tx_data_frame_option.ul_datnb = 0;
  }

  // No frame clock output
  tx_data_frame_option.ul_fsos = SSC_TFMR_FSOS_NONE;
  
  ///////////////////////////////////////////////////////////////////////////
  /// Load configuration and enable TX interrupt
  ///////////////////////////////////////////////////////////////////////////
  ssc_set_transmitter(SSC, &tx_clk_option, &tx_data_frame_option);
  ssc_enable_interrupt(SSC, SSC_IER_TXRDY);  
}

void HiFiClass::enableTx(bool enable)
{
  if (enable)
  {
    ssc_enable_tx(SSC);
  }
  else
  {
    ssc_disable_tx(SSC);
  }
}

void HiFiClass::onTxReady(void(*function)(HiFiChannelID_t)) {
  onTxReadyCallback = function;
}

void HiFiClass::configureRx( HiFiAudioMode_t audioMode,
                HiFiClockMode_t clkMode,
                uint8_t bitsPerChannel )
{
  clock_opt_t rx_clk_option;
  data_frame_opt_t rx_data_frame_option;
  
  memset((uint8_t *)&rx_clk_option, 0, sizeof(clock_opt_t));
  memset((uint8_t *)&rx_data_frame_option, 0, sizeof(data_frame_opt_t));
  
  ///////////////////////////////////////////////////////////////////////////
  /// Receiver IO Pin configuration
  ///////////////////////////////////////////////////////////////////////////
  uint8_t endpin = sizeof(SSCRXPins)/sizeof(SSCRXPins[0]);
  if (clkMode == HIFI_CLK_MODE_USE_TK_RK_CLK)
  {
    endpin = 1;
  }
  
  for (int i=0; i < (sizeof(SSCRXPins)/sizeof(SSCRXPins[0])); i++)
  {
    PIO_Configure(SSCRXPins[i].pPort,
            SSCRXPins[i].ulPinType,
            SSCRXPins[i].ulPin,
            SSCRXPins[i].ulPinConfiguration);
  }
  
  // Note: there is a function in the Atmel ssc driver for configuration of
  // the peripheral in I2S mode, but it is incomplete and buggy.  This library
  // will configure the SSC directly which will also shed some light on the
  // various configuration parameters should a user need something slightly
  // different.

  ///////////////////////////////////////////////////////////////////////////
  /// Receiver clock mode configuration
  ///////////////////////////////////////////////////////////////////////////
  switch (clkMode)
  {
    case HIFI_CLK_MODE_USE_EXT_CLKS:
      // Use clocks on the RK/RF pins
      rx_clk_option.ul_cks = SSC_RCMR_CKS_RK;
  
      if (audioMode == HIFI_AUDIO_MODE_MONO_RIGHT)
      {
        // high level on the frame clock is right channel in
        // I2S.  If we're only using the right channel, then
        // we can set start condition for right-only
        rx_clk_option.ul_start_sel = SSC_RCMR_START_RF_RISING;
      }
      else
      {
        // stereo or mono-left will start in the left channel slot
        rx_clk_option.ul_start_sel = SSC_RCMR_START_RF_FALLING;
      }
      break;
  
    case HIFI_CLK_MODE_USE_TK_RK_CLK:
      // Use the clock selected by the transmitter config 
      // (i.e. sync receiver to transmitter).
      rx_clk_option.ul_cks = SSC_RCMR_CKS_TK;
  
      // Use the transmitter's configuration as the start trigger
      // The transmitter must be configured and running in
      // order for this to work.
      rx_clk_option.ul_start_sel = SSC_RCMR_START_TRANSMIT;
      break;
  }

  // No output clocks
  rx_clk_option.ul_ckg = SSC_RCMR_CKG_NONE;
  rx_clk_option.ul_period = 0;  // we're not master -- set to 0
  rx_clk_option.ul_cko = SSC_RCMR_CKO_NONE;
  // I2S latches data on the rising edge of the clock.
  rx_clk_option.ul_cki = SSC_RCMR_CKI;
  // I2S has a one bit delay on the data.
  rx_clk_option.ul_sttdly = 1;
  
  ///////////////////////////////////////////////////////////////////////////
  /// Receiver frame mode configuration.
  ///////////////////////////////////////////////////////////////////////////
  rx_data_frame_option.ul_datlen = bitsPerChannel - 1;
  rx_data_frame_option.ul_msbf = SSC_RFMR_MSBF;

  // number of channels
  if (audioMode == HIFI_AUDIO_MODE_STEREO) 
  {
    rx_data_frame_option.ul_datnb = 1;
  } 
  else 
  {
    rx_data_frame_option.ul_datnb = 0;
  }

  // No frame clock output
  rx_data_frame_option.ul_fsos = SSC_TFMR_FSOS_NONE;
  
  ///////////////////////////////////////////////////////////////////////////
  /// Load configuration and enable RX interrupt
  ///////////////////////////////////////////////////////////////////////////
  ssc_set_receiver(SSC, &rx_clk_option, &rx_data_frame_option);
  ssc_enable_interrupt(SSC, SSC_IER_RXRDY);
}  

void HiFiClass::enableRx(bool enable)
{
  if (enable)
  {
    ssc_enable_rx(SSC);
  }
  else
  {
    ssc_disable_rx(SSC);
  }
}

void HiFiClass::onRxReady(void(*function)(HiFiChannelID_t)) {
  onRxReadyCallback = function;
}

void HiFiClass::onService(void)
{
  HiFiChannelID_t channel;
  
  // read and save status -- some bits are cleared on a read 
  uint32_t status = ssc_get_status(SSC);

  if (ssc_is_tx_ready(SSC) == SSC_RC_YES)
  {
    if (HiFi.onTxReadyCallback)
    {
      // The TXSYN event is triggered based on what the start 
      // condition was set to during configuration.  This
      // is usually the left channel, except in the case of the 
      // mono right setup, in which case it's the right.  This
      // may need to change if support for other formats 
      // (e.g. TDM) is added. 
      if (status & SSC_IER_TXSYN)
      {
        channel = HIFI_CHANNEL_ID_1;
      }
      else
      {
        channel = HIFI_CHANNEL_ID_2;
      }
      HiFi.onTxReadyCallback(channel);
    }
  }
  
  if (ssc_is_rx_ready(SSC) == SSC_RC_YES)
  {
    if (HiFi.onRxReadyCallback)
    {
      // The RXSYN event is triggered based on what the start
      // condition was set to during configuration.  This
      // is usually the left channel, except in the case of the
      // mono right setup, in which case it's the right.  This
      // may need to change if support for other formats
      // (e.g. TDM) is added.
      if (status & SSC_IER_RXSYN)
      {
        channel = HIFI_CHANNEL_ID_1;
      }
      else
      {
        channel = HIFI_CHANNEL_ID_2;
      }
      HiFi.onRxReadyCallback(channel);
    }
  }
}

/**
 * \brief Synchronous Serial Controller Handler.
 *
 */
void SSC_Handler(void)
{
  HiFi.onService();
}

// Create our object
HiFiClass HiFi = HiFiClass();

