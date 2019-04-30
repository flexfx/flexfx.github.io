#ifndef INCLUDED_XIO_H
#define INCLUDED_XIO_H

typedef unsigned char byte;
typedef unsigned int  bool;

#define FALSE 0
#define TRUE  1

// Functions and variables to be implemented by the application ...

extern const char* product_name_string;   // Your product name
extern const char* usb_audio_output_name; // Your USB audio output name
extern const char* usb_audio_input_name;  // Your USB audio input name
extern const char* usb_midi_output_name;  // Your USB MIDI output name
extern const char* usb_midi_input_name;   // Your USB MIDI input name

extern const int audio_sample_rate;       // Audio sampling frequency (48K,96K,192K,or 384K)
extern const int usb_output_chan_count;   // 2 USB audio output channels (32 max)
extern const int usb_input_chan_count;    // 2 USB audio input channels (32 max)
extern const int i2s_channel_count;       // 2,4,or8 I2S channels per SDIN/SDOUT wire

extern const int i2s_sync_word[8];        // I2S WCLK words for each slot

extern const char controller_script[];

// The startup task is called before the control and mixer tasks and should be used to initizlize
// control data and variables.

extern void xio_startup( void );

// The control task is called at a rate of 1000 Hz and should be used to implement audio CODEC
// initialization/control, pot and switch sensing via I2C ADC's, handling of properties from USB
// MIDI, and generation of properties to be consumed by the USB MIDI host and by the DSP threads.
// The incoming USB property 'rcv_prop' is valid if its ID is non-zero and an outgoing USB
// property, as a response to the incoming property, will be sent out if's ID is non-zero. DSP
// propertys can be sent to DSP threads (by setting the DSP property ID to zero) at any time.
// It's OK to use floating point calculations here as this thread is not a real-time audio thread.

extern void xio_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] );

// The mixer function is called once per audio sample and is used to route USB, I2S and DSP samples.
// This function should only be used to route samples and for very basic DSP processing - not for
// substantial sample processing since this may starve the I2S audio driver. Do not use floating
// point operations since this is a real-time audio thread - all DSP operations and calculations
// should be performed using fixed-point math.
// NOTE: IIR, FIR, and BiQuad coeff and state data *must* be declared non-static global!

extern void xio_mixer( const int usb_output[32], int usb_input[32],
                       const int i2s_output[32], int i2s_input[32],
                       const int dsp_output[32], int dsp_input[32], const int property[6] );

// Audio Processing Threads. These functions run on tile 1 and are called once for each audio sample
// cycle. They cannot share data with the controller task or the mixer functions above that run on
// tile 0. The number of incoming and outgoing samples in the 'samples' array is set by the constant
// 'dsp_chan_count' defined above. Do not use floating point operations since these are real-time
// audio threads - all DSP operations and calculations should be performed using fixed-point math.
// NOTE: IIR, FIR, and BiQuad coeff and state data *must* be declared non-static global!

extern void xio_intialize( void );

// Process samples from the app_mixer function. Send results to stage 2.
extern void xio_thread1( int samples[32], const int property[6] );
// Process samples from stage 1. Send results to stage 3.
extern void xio_thread2( int samples[32], const int property[6] );
// Process samples from stage 2. Send results to stage 4.
extern void xio_thread3( int samples[32], const int property[6] );
// Process samples from stage 3. Send results to stage 5.
extern void xio_thread4( int samples[32], const int property[6] );
// Process samples from stage 4. Send results to the app_mixer function.
extern void xio_thread5( int samples[32], const int property[6] );

// On-board FLASH read write functions.

void flash_read ( int page, byte data[256] );
void flash_write( int page, const byte data[256] );

unsigned timer_count( void );
void timer_delay( int microseconds );

void midi_send_start( void );      // Send MIDI start command to USB host.
void midi_send_stop( void );       // Send MIDI stop command to USB host.
void midi_send_beat( void );       // Send MIDI beat command to USB host.
void midi_configure( double bpm ); // Enable (bpm>0) or disable (bpm<=0) MIDI beat clock.

// Functions for peripheral control (*** Only use these in the 'xio_control' function ***).
// I2C ans SPI share the same pins (SPI SCLK and I2C SCL, SPI MOSI and I2C SDA).
// For multiple SPI slave devices use an I2S bus expander to implement multiple SPI CSEL signals.
// Serial log output uses 1 start bit, 2 stop bits, and 1Mbaud rate.

void i2c_init ( int speed );  // Set bit rate (bps), set SCL/SDA to high
void i2c_start( void );       // Assert an I2C start condition.
void i2c_cont ( void );       // Continue with a restart.
byte i2c_write( byte value ); // Write 8-bit data value.
byte i2c_read ( void );       // Read 8-bit data value.
void i2c_ack  ( byte ack );   // Assert the ACK/NACK bit after a read.
void i2c_stop ( void );       // Assert an I2C stop condition.

void spi_init ( int speed );  // Set the SPI bit rate (bps), this SPI bus is *not* full-duplex.
void spi_write( byte value ); // Write one byte (SPI SCLK is I2C SCL, SPI MOSI is I2C SDA).
byte spi_read ( void );       // Read one byte (SPI SCLK is I2C SCL, SPI MISO is I2C SDA).

void log_chr( char val );                  // Print single text character.
void log_str( const char* text );          // Print null-terminated text string.
void log_hex( byte val );                  // Print 1-byte ASCII/HEX value.
void log_bin( const byte* data, int len ); // Print binary as ASCII/HEX.

#endif // XIO_H
