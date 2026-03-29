// Teensy 4.1 pin definitions for H10 Controller project

// H10 Controls
#define PunchStart      18	// Starts H10 Punch cycle
#define PunchReady      19  // H10 punch is ready to cycle
#define ReaderReady     20	// H10 read data is available
#define ReaderStart     21  // Start H10 Read cycle
#define ReadDataLoad    22	// Load H10 Reader data
#define PunchDataLatch  23	// Latch H10 Punch data

// SPI Bus
#define MOSI		11	// SPI MasterOutSlaveIn
#define MISO		12	// SPI MasterInSlaveOut
#define SCK			13	// SPI Clock

// RS-232 Port
#define RS232_Port Serial1
#define RS232_RX  	0
#define RS232_TX  	1
#define RS232_CTS  	2
#define RS232_RTS  	3

// Debug
#define LED1    	30
#define LED2    	31
#define PB1     	32

// FTDI Cable Port
#define FTID_PORT Serial8
#define FTDI_RTS	33	// Signal to FTDI CTS to indicate it is OK for FTDI to send data	
#define FTDI_RX  	34	// Receive data from the FTDI TX
#define FTDI_TX  	35	// Sends data to FTDI RX
#define FTDI_CTS 	36	// Signal from FTID RTS to indicate it is OK to send data to FTDI

// Onboard status LED
#define STATUS_LED LED_BUILTIN

#define kMaxCommandSize 146




