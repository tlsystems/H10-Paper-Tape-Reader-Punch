

// H10 Controls
#define PunchStart      4   // Starts H10 Punch cycle
#define PunchReady      5   // H10 punch is ready to cycle
#define ReaderReady     6   // H10 read data is available
#define ReaderStart     7   // Start H10 Read cycle

// SPI Bus
#define ReadDataLoad    8    // Load H10 Reader data
#define PunchDataLatch  9    // Latch H10 Punch data
#define SCK             10   // SPI Clock
#define MOSI            11   // SPI MasterOutSlaveIn
#define MISO            12   // SPI MasterInSlaveOut

// Debug
#define PB1     13
#define LED1    14
#define LED2    15

// RS-232 Port
#define RS232_TX  16
#define RS232_RX  17
#define RS232_CTS 18
#define RS232_RTS 19

// internal pico pins
#define POWER_SAVE   23 // Output to enable power save mode
#define VBUS_SENSE   24 // Input to monitor presense of VBUS
#define PICO_LED     25 // Output to control onboard LED
#define VSYS_MONITOR 29 // Analog Input to monotor VSYS






