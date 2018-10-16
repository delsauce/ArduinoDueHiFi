ArduinoDueHiFi
==============

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/235294e9ee4c42ce863f7b10c74c79c8)](https://app.codacy.com/app/delsauce/ArduinoDueHiFi?utm_source=github.com&utm_medium=referral&utm_content=delsauce/ArduinoDueHiFi&utm_campaign=Badge_Grade_Settings)

An I2S audio codec driver library for the Arduino Due board.

This library will configure the SSC on the ARM to transmit and/or
receive in I2S mode.  This will allow the Arduino to interface with 
a large number of audio codecs and enable higher quality audio I/O
than what is available with the standard on-chip ADC and DAC.  The 
library can be used to enable CD quality audio signal processing, 
waveform synthesis, audio file (e.g. wav) recording and playback, etc.

The driver currently supports slave mode only as the ARM doesn't
appear to be able to generate the appropriate MCLK signal to drive 
external converters.  It may be desirable to record and playback
files and transfer them to another device (e.g. PC) for further use
so the clocks can be supplied by the converter to get to a standard 
sampling frequency (e.g. 32kHz, 44.1kHz, 48kHz, etc.).

Although the SSC peripheral suppors many modes (Left-justified, I2S,
TDM) only I2S is supported out-of-the box to keep the driver simple
and easier to understand.  Most audio converters support this protocol.

A couple of simple examples are provided that demonstrate usage of the
library.
