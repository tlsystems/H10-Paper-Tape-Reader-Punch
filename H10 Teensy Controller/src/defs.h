// Teensy 4.1 pin definitions for H10 Controller project

// H10 Controls
#define PunchStart      40	// Starts H10 Punch cycle
#define PunchReady      41  // H10 punch is ready to cycle
#define ReaderReady     42	// H10 read data is available
#define ReaderStart     43  // Start H10 Read cycle
#define ReadDataLoad    44	// Load H10 Reader data
#define PunchDataLatch  45	// Latch H10 Punch data

// SPI Bus
#define MOSI            11	// SPI MasterOutSlaveIn
#define MISO            12	// SPI MasterInSlaveOut
#define SCK             13	// SPI Clock

// RS-232 Port
#define RS232_Port Serial8
#define RS232_CTS 25
#define RS232_RTS 26
#define RS232_RX  27
#define RS232_TX  28

// FTDI Cable Port
#define FTID_PORT Serial7
#define FTDI_CTS 18
#define FTDI_RTS 19
#define FTDI_TX  20
#define FTDI_RX  21

// Debug
#define PB1     22
#define LED1    23
#define LED2    24

// Onboard status LED
#define STATUS_LED LED_BUILTIN






