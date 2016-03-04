#include <SI_EFM8UB1_Register_Enums.h>
#include <stdarg.h>

#include "efm8_usb.h"
#include "usb_0.h"
#include "atecc508a.h"
#include "InitDevice.h"
#include "descriptors.h"
#include "eeprom.h"
#include "idle.h"
#include "bsp.h"
#include "app.h"
#include "i2c.h"
#include "u2f_hid.h"
#include "u2f.h"
#include "tests.h"

data struct APP_DATA appdata;


#ifdef U2F_PRINT

	FIFO_CREATE(debug,struct debug_msg,  DBG_MSG_COUNT)

#endif

static void init(struct APP_DATA* ap)
{
	memset(ap,0, sizeof(struct APP_DATA));
	debug_fifo_init();
	u2f_hid_init();
	smb_init();
	atecc_idle();
}

static void listen_for_pkt(struct APP_DATA* ap)
{
	USBD_Read(EP1OUT, ap->hidmsgbuf, sizeof(ap->hidmsgbuf), true);
}


void set_app_error(APP_ERROR_CODE ec)
{
	appdata.state = APP_ERROR;
	appdata.error = ec;
}

//uint8_t readByte (uint16_t addr)
//{
//
//	uint8_t code * pread;               // FLASH read pointer
//	uint8_t byte;
//	DISABLE_INTERUPTS();
//   pread = (uint8_t code *) addr;
//
//   byte = *pread;                      // Read the byte
//
//   ENABLE_INTERUPTS();
//
//   return byte;
//}





int8_t test_eeprom()
{

}

#define ms_since(ms,num) (((uint16_t)get_ms() - (ms)) >= num ? (1|(ms=(uint16_t)get_ms())):0)

int16_t main(void) {

	data uint8_t i = 0;
	data uint16_t last_ms = get_ms();
	data uint16_t ms_heart;
	data uint16_t ms_wink;
	data uint8_t winks = 0;


	uint8_t b = 0x66;
	uint16_t  d = 7777;
	uint32_t l = 0x12345678;


	enter_DefaultMode_from_RESET();
	init(&appdata);



	// STDIO library requires TI to print
	SCON0_TI = 1;

	// Enable interrupts
	IE_EA = 1;

	u2f_prints("U2F ZERO\r\n");

	u2f_printb("b:",1,b);
	u2f_printd("d:",1,d);
	u2f_printlx("lx:",1,l);
	run_tests();
	test_eeprom();

	while (1) {

		if (ms_since(ms_heart,500))
		{
			u2f_printl("ms ", 1, get_ms());
			LED_G = !LED_G;
		}


		if ( USBD_GetUsbState() == USBD_STATE_CONFIGURED)
		{
			if (!USBD_EpIsBusy(EP1OUT) && !USBD_EpIsBusy(EP1IN))
			{
				listen_for_pkt(&appdata);
				u2f_prints("read added\r\n");
			}

		}

		switch(appdata.state)
		{
			case APP_NOTHING:
				break;
			case APP_WINK:
				LED_B = 0;
				ms_wink = get_ms();
				appdata.state = _APP_WINK;
				break;
			case _APP_WINK:
				if (ms_since(ms_wink,150))
				{
					LED_B = !LED_B;
					winks++;
				}
				if (winks == 5)
				{
					winks = 0;
					appdata.state = APP_NOTHING;
				}
				break;
			case APP_ERROR:
				u2f_printb("error: ", 1, appdata.error);
				appdata.state = APP_NOTHING;
				break;
		}

		flush_messages();

	}
}


