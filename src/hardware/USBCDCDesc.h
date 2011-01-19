
/* Copyright (c) 2010, Peter Barrett  
**  
** Permission to use, copy, modify, and/or distribute this software for  
** any purpose with or without fee is hereby granted, provided that the  
** above copyright notice and this permission notice appear in all copies.  
**  
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
** SOFTWARE.  
*/

#define USB_VID 0x239A	// Adafruit vid
#define USB_PID 0x2001	// sorry dave sprach zarathustra
///#define USB_VID		0x16C0 	// Vendor ID
//#define USB_PID		0x03F2	// Product ID - need a real one at some point

#define USB_DEVICE	0x0100	// Device ID

#define CDC_ACM_ENDPOINT 1
#define CDC_RX_ENDPOINT 2
#define CDC_TX_ENDPOINT 3

#define EP_TYPE_CONTROL				0x00
#define EP_TYPE_BULK_IN				0x81
#define EP_TYPE_BULK_OUT			0x80
#define EP_TYPE_INTERRUPT_IN		0xC1
#define EP_TYPE_INTERRUPT_OUT		0xC0
#define EP_TYPE_ISOCHRONOUS_IN		0x41
#define EP_TYPE_ISOCHRONOUS_OUT		0x40

#define GET_STATUS			0
#define CLEAR_FEATURE		1
#define SET_FEATURE			3
#define SET_ADDRESS			5
#define GET_DESCRIPTOR		6
#define GET_CONFIGURATION	8
#define SET_CONFIGURATION	9
#define GET_INTERFACE		10
#define SET_INTERFACE		11

#define CDC_SET_LINE_CODING		0x20
#define CDC_GET_LINE_CODING		0x21
#define CDC_SET_CONTROL_LINE_STATE	0x22

//	Descriptors

#define USB_DEVICE_DESC_SIZE 18
#define USB_CONFIGUARTION_DESC_SIZE 9
#define USB_INTERFACE_DESC_SIZE 9
#define USB_ENDPOINT_DESC_SIZE 7

#define USB_DEVICE_DESCRIPTOR_TYPE             1
#define USB_CONFIGURATION_DESCRIPTOR_TYPE      2
#define USB_STRING_DESCRIPTOR_TYPE             3
#define USB_INTERFACE_DESCRIPTOR_TYPE          4
#define USB_ENDPOINT_DESCRIPTOR_TYPE           5

#define USB_DEVICE_CLASS_COMMUNICATIONS        0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE       0x03
#define USB_DEVICE_CLASS_STORAGE               0x08
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC       0xFF

#define USB_CONFIG_POWERED_MASK                0x40
#define USB_CONFIG_BUS_POWERED                 0x80
#define USB_CONFIG_SELF_POWERED                0xC0
#define USB_CONFIG_REMOTE_WAKEUP               0x20

// bMaxPower in Configuration Descriptor
#define USB_CONFIG_POWER_MA(mA)                ((mA)/2)

// bEndpointAddress in Endpoint Descriptor
#define USB_ENDPOINT_DIRECTION_MASK            0x80
#define USB_ENDPOINT_OUT(addr)                 ((addr) | 0x00)
#define USB_ENDPOINT_IN(addr)                  ((addr) | 0x80)

#define USB_ENDPOINT_TYPE_MASK                 0x03
#define USB_ENDPOINT_TYPE_CONTROL              0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS          0x01
#define USB_ENDPOINT_TYPE_BULK                 0x02
#define USB_ENDPOINT_TYPE_INTERRUPT            0x03

#define TOBYTES(x) ((x) & 0xFF),(((x) >> 8) & 0xFF)

#define USB_CDC_CIF_NUM     0
#define USB_CDC_DIF_NUM     1
#define USB_CDC_BUFSIZE		64

#define CDC_V1_10                               0x0110
#define CDC_COMMUNICATION_INTERFACE_CLASS       0x02

#define CDC_CALL_MANAGEMENT                     0x01
#define CDC_ABSTRACT_CONTROL_MODEL              0x02
#define CDC_HEADER                              0x00
#define CDC_ABSTRACT_CONTROL_MANAGEMENT         0x02
#define CDC_UNION                               0x06
#define CDC_CS_INTERFACE                        0x24
#define CDC_CS_ENDPOINT                         0x25
#define CDC_DATA_INTERFACE_CLASS                0x0A

#if 0

struct DESC_STR {
	uint8_t bLength;
	uint8_t bDescriptorType;
	int16_t wString[];
};

extern const struct DESC_STR DESC_LANGUAGE PROGMEM;
const struct DESC_STR DESC_LANGUAGE = {
	4,
	3,
	{0x0409}
};

#ifdef STR_MANUFACTURER
#define IMANUFACTURER 1
static struct DESC_STR PROGMEM DESC_MANUFACTURER = {sizeof(STR_MANUFACTURER), 3, STR_MANUFACTURER };
#else
#define IMANUFACTURER 0
#endif

#ifdef STR_PRODUCT
#define IPRODUCT 2
static struct DESC_STR PROGMEM DESC_PRODUCT = { sizeof(STR_PRODUCT), 3, STR_PRODUCT };
#else
#define IPRODUCT 0
#endif

#ifdef STR_SERIAL_NUMBER
#define ISERIAL_NUMBER 3
static struct DESC_STR PROGMEM DESC_SERIAL_NUMBER = { sizeof(STR_SERIAL_NUMBER), 3, STR_SERIAL_NUMBER };
#else
#define ISERIAL_NUMBER 0
#endif

#else

extern const u16 DESC_LANGUAGE[] PROGMEM;
const u16 DESC_LANGUAGE[] = {
	(3<<8) | (2+2),
	0x0409	// English
};

extern const u16 DESC_PRODUCT[] PROGMEM;
const u16 DESC_PRODUCT[] = {
	(3<<8) | (2+2*10),
	'M','i','c','r','o','t','o','u','c','h'
};

#define IMANUFACTURER	0
#define IPRODUCT		2
#define ISERIAL_NUMBER	0

#endif


// USB Standard Device Descriptor
extern const uint8_t USB_DeviceDescriptor[] PROGMEM;
const uint8_t USB_DeviceDescriptor[] = {
  USB_DEVICE_DESC_SIZE,				// bLength
  USB_DEVICE_DESCRIPTOR_TYPE,		// bDescriptorType
  TOBYTES(0x0200),					// bcdUSB
  USB_DEVICE_CLASS_COMMUNICATIONS,	// bDeviceClass CDC
  0x00,								// bDeviceSubClass
  0x00,								// bDeviceProtocol
  16,								// bMaxPacketSize0 endpoint zero is 16 bytes
  TOBYTES(USB_VID),					// idVendor
  TOBYTES(USB_PID),					// idProduct
  TOBYTES(0x100),					// bcdDevice
  IMANUFACTURER,					// iManufacturer string index
  IPRODUCT,							// iProduct string index
  ISERIAL_NUMBER,					// iSerialNumber string index
  0x01								// bNumConfigurations
};

extern const uint8_t USB_ConfigDescriptor[] PROGMEM;
const uint8_t USB_ConfigDescriptor[] = {
// Configuration 1
	USB_CONFIGUARTION_DESC_SIZE,		// bLength
	USB_CONFIGURATION_DESCRIPTOR_TYPE,	// bDescriptorType
	TOBYTES(								// wTotalLength
		USB_CONFIGUARTION_DESC_SIZE +
		USB_INTERFACE_DESC_SIZE     +		// communication interface
		0x0013                      +		// CDC functions
		USB_ENDPOINT_DESC_SIZE      +		// interrupt endpoint
		USB_INTERFACE_DESC_SIZE   +			// data interface
		USB_ENDPOINT_DESC_SIZE*2			// bulk endpoints
	),
	0x02,								// bNumInterfaces
	0x01,								// bConfigurationValue: 0x01 is used to select this configuration
	0x00,								// iConfiguration: no string to describe this configuration
	USB_CONFIG_BUS_POWERED,				// bmAttributes
	USB_CONFIG_POWER_MA(100),			// bMaxPower, device power consumption is 100 mA

//	Interface 0, Alternate Setting 0, Communication class interface descriptor
	USB_INTERFACE_DESC_SIZE,           // bLength
	USB_INTERFACE_DESCRIPTOR_TYPE,     // bDescriptorType
	USB_CDC_CIF_NUM,                   // bInterfaceNumber: Number of Interface
	0x00,                              // bAlternateSetting: Alternate setting
	0x01,                              // bNumEndpoints: One endpoint used
	CDC_COMMUNICATION_INTERFACE_CLASS, // bInterfaceClass: Communication Interface Class
	CDC_ABSTRACT_CONTROL_MODEL,        // bInterfaceSubClass: Abstract Control Model
	0x00,                              // bInterfaceProtocol: no protocol used
	0x00,                              // iInterface:
		//Header Functional Descriptor
		0x05,                              // bLength: Endpoint Descriptor size
		CDC_CS_INTERFACE,                  // bDescriptorType: CS_INTERFACE
		CDC_HEADER,                        // bDescriptorSubtype: Header Func Desc
		TOBYTES(CDC_V1_10), // 1.10       // bcdCDC
		//Call Management Functional Descriptor
		0x05,                              // bFunctionLength
		CDC_CS_INTERFACE,                  // bDescriptorType: CS_INTERFACE
		CDC_CALL_MANAGEMENT,               // bDescriptorSubtype: Call Management Func Desc
		0x01,                              // bmCapabilities: device handles call management
		0x01,                              // bDataInterface: CDC data IF ID
		//Abstract Control Management Functional Descriptor
		0x04,                              // bFunctionLength
		CDC_CS_INTERFACE,                  // bDescriptorType: CS_INTERFACE
		CDC_ABSTRACT_CONTROL_MANAGEMENT,   // bDescriptorSubtype: Abstract Control Management desc
		0x02,                              // bmCapabilities: SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported
		//Union Functional Descriptor
		0x05,                              // bFunctionLength
		CDC_CS_INTERFACE,                  // bDescriptorType: CS_INTERFACE
		CDC_UNION,                         // bDescriptorSubtype: Union func desc
		USB_CDC_CIF_NUM,                   // bMasterInterface: Communication class interface is master
		USB_CDC_DIF_NUM,                   // bSlaveInterface0: Data class interface is slave 0
		//Endpoint 2 Descriptor            // event notification (optional)
		USB_ENDPOINT_DESC_SIZE,            // bLength
		USB_ENDPOINT_DESCRIPTOR_TYPE,      // bDescriptorType
		USB_ENDPOINT_IN(1),                // bEndpointAddress
		USB_ENDPOINT_TYPE_INTERRUPT,       // bmAttributes
		TOBYTES(0x0010),                     // wMaxPacketSize
		0x40,							// bInterval

//	Interface 1, Alternate Setting 0, Data class interface descriptor
	USB_INTERFACE_DESC_SIZE,           // bLength
	USB_INTERFACE_DESCRIPTOR_TYPE,     // bDescriptorType
	USB_CDC_DIF_NUM,                   // bInterfaceNumber: Number of Interface
	0x00,                              // bAlternateSetting: no alternate setting
	0x02,                              // bNumEndpoints: two endpoints used
	CDC_DATA_INTERFACE_CLASS,          // bInterfaceClass: Data Interface Class
	0x00,                              // bInterfaceSubClass: no subclass available
	0x00,                              // bInterfaceProtocol: no protocol used
	0x00,                              // iInterface:

		// Endpoint, EP3 Bulk Out
		USB_ENDPOINT_DESC_SIZE,            // bLength
		USB_ENDPOINT_DESCRIPTOR_TYPE,      // bDescriptorType
		USB_ENDPOINT_OUT(2),               // bEndpointAddress
		USB_ENDPOINT_TYPE_BULK,            // bmAttributes
		TOBYTES(USB_CDC_BUFSIZE),            // wMaxPacketSize
		0x00,                              // bInterval: ignore for Bulk transfer
		// Endpoint, EP4 Bulk In
		USB_ENDPOINT_DESC_SIZE,            // bLength
		USB_ENDPOINT_DESCRIPTOR_TYPE,      // bDescriptorType
		USB_ENDPOINT_IN(3),                // bEndpointAddress
		USB_ENDPOINT_TYPE_BULK,            // bmAttributes
		TOBYTES(USB_CDC_BUFSIZE),            // wMaxPacketSize
		0x00,                              // bInterval: ignore for Bulk transfer
		0                                  // bLength
};
