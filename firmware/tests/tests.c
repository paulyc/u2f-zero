/*
 * tests.c
 *
 *  Created on: Feb 14, 2016
 *      Author: Conor
 */
#include <SI_EFM8UB1_Register_Enums.h>
#include <stdio.h>
#include <stdint.h>
#include "app.h"
#include "bsp.h"
#include "i2c.h"
#include "atecc508a.h"
#include "eeprom.h"
#include "tests.h"


#ifdef ENABLE_TESTS


static void PRINT(const char * s)
{
	u2f_prints(s);
	u2f_prints("\r\n");
}


#ifdef TEST_SHA
static int test_sha()
{
	uint8_t buf[40];
	uint8_t len;
	char pw[] = "2pckJ4IkT3PwdGMwuCygPpxD6+lObNGORiLGPQxM4ef4YoNvx9/k0xskZl84rCd3TllCvitepe+B";
	do{
		atecc_send(ATECC_CMD_SHA,
				ATECC_SHA_START,
				0,NULL,0);
	}while((len = atecc_recv(buf,sizeof(buf))) < 0);
	dump_hex(buf,len);

	do{
		atecc_send(ATECC_CMD_SHA,
				ATECC_SHA_UPDATE,
				64,pw,64);
	} while ((len = atecc_recv(buf,sizeof(buf))) < 0);
	dump_hex(buf,len);

	do{
		atecc_send(ATECC_CMD_SHA,
				ATECC_SHA_END,
				sizeof(pw)-65,pw+64,sizeof(pw)-65);
	}while((len = atecc_recv(buf,sizeof(buf))) < 0);
	dump_hex(buf,len);

	// sha256 sum should be bcddd71b48f8a31d1374ad51c2e4138a871cb7f1eb3f2bdab49bc9bc60afc3a5
	return (SMB.crc == 0x9768) ? 0 : -1;
}
#else
#define test_sha(x)
#endif

#ifdef TEST_ATECC_EEPROM

static void slot_dump(void* slot)
{
	struct atecc_slot_config* a = (struct atecc_slot_config*) slot;
	flush_messages();
	u2f_print("    readkey %bx\r\n", a->readkey);
	if (a->nomac) u2f_print("    nomac\r\n");
	flush_messages();
	if (a->limiteduse) u2f_print("    limiteduse\r\n");
	if (a->encread) u2f_print("    encread\r\n");
	flush_messages();
	if (a->secret) u2f_print("    secret\r\n");
	u2f_print("    writekey %bx\r\n", a->writekey);
	flush_messages();
	u2f_print("    writeconfig %bx\r\n", a->writeconfig);
	flush_messages();
}

static void key_dump(void* slot)
{
	struct atecc_key_config* a = (struct atecc_slot_config*) slot;
	flush_messages();

	if (a->private) u2f_print("    private\r\n");
	if (a->pubinfo) u2f_print("    pubinfo\r\n");
	flush_messages();
	u2f_print("    keytype %bx\r\n", a->keytype);
	if (a->lockable) u2f_print("    lockable\r\n");
	flush_messages();
	if (a->reqrandom) u2f_print("    reqrandom\r\n");
	if (a->reqauth) u2f_print("    reqauth\r\n");
	flush_messages();
	u2f_print("    authkey %bx\r\n", a->authkey);
	if (a->intrusiondisable) u2f_print("    intrusiondisable\r\n");
	flush_messages();
	if (a->rfu) u2f_print("    rfu\r\n");
	u2f_print("    x509id %bx\r\n", a->x509id);
	flush_messages();
}

static int test_atecc_eeprom()
{
	uint8_t buf[7];
	uint16_t c1,c2,c3,c4;

	struct atecc_response res;
	struct atecc_slot_config slotconfig;
	struct atecc_key_config keyconfig;

	memset(&slotconfig, 0, sizeof(struct atecc_slot_config));
	memset(&keyconfig, 0, sizeof(struct atecc_key_config));

	slotconfig.secret = 1;
	slotconfig.writeconfig = 0xA;
	slotconfig.readkey = 0x3;


	if (atecc_write_eeprom(ATECC_EEPROM_SLOT(0), ATECC_EEPROM_SLOT_OFFSET(0), &slotconfig, ATECC_EEPROM_SLOT_SIZE) != 0)
	{
		return -1;
	}

	slotconfig.writeconfig = 0x3;

	if (atecc_write_eeprom(ATECC_EEPROM_SLOT(1), ATECC_EEPROM_SLOT_OFFSET(1), &slotconfig, ATECC_EEPROM_SLOT_SIZE) != 0)
	{
		return -1;
	}

	keyconfig.private = 1;
	keyconfig.pubinfo = 1;
	keyconfig.keytype = 0x4;
	keyconfig.lockable = 1;

	if (atecc_write_eeprom(ATECC_EEPROM_KEY(0), ATECC_EEPROM_KEY_OFFSET(0), &keyconfig, ATECC_EEPROM_KEY_SIZE) != 0)
	{
		return -1;
	}

	keyconfig.keytype = 0x3;

	if (atecc_write_eeprom(ATECC_EEPROM_KEY(1), ATECC_EEPROM_KEY_OFFSET(1), &keyconfig, ATECC_EEPROM_KEY_SIZE) != 0)
	{
		return -1;
	}


	atecc_send_recv(ATECC_CMD_READ,
			ATECC_RW_CONFIG, 5,NULL,0,
			buf,sizeof(buf), &res);

	u2f_print("-- slot 0 --\r\n");
	dump_hex(res.buf,2);
	slot_dump(res.buf);

	u2f_print("-- slot 1 --\r\n");
	dump_hex(res.buf+2,2);
	slot_dump(res.buf+2);

	if (*(uint16_t*)(res.buf ) != 0x83a0 || *(uint16_t*)(res.buf + 2) != 0x8330)
	{
		return -1;
	}

	atecc_send_recv(ATECC_CMD_READ,
			ATECC_RW_CONFIG, 24,NULL,0,
			buf,sizeof(buf), &res);

	u2f_print("-- key 0 --\r\n");
	dump_hex(res.buf,2);
	key_dump(res.buf);

	u2f_print("-- key 1 --\r\n");
	dump_hex(res.buf+2,2);
	key_dump(res.buf+2);

	if (*(uint16_t*)(res.buf ) != 0x3300 || *(uint16_t*)(res.buf+2) != 0x2f00)
	{
		return -1;
	}

	return 0;
}
#else
#define test_atecc_eeprom(x)
#endif

#ifdef TEST_KEY_SIGNING
int test_key_signing()
{
	struct atecc_response res;
	uint8_t buf[72];
	char msg[] = "wow a signature!\n";

	atecc_send_recv(ATECC_CMD_SHA,
			ATECC_SHA_START, 0, NULL, 0,
			buf, sizeof(buf), &res);

	atecc_send_recv(ATECC_CMD_SHA,
			ATECC_SHA_END, sizeof(msg)-1, msg, sizeof(msg)-1,
			buf, sizeof(buf), &res);

	dump_hex(res.buf, res.len);

	u2f_print("sig:\r\n");

	atecc_send_recv(ATECC_CMD_SIGN,
			ATECC_SIGN_EXTERNAL, 0, NULL, 0,
			buf, sizeof(buf), &res);
	dump_hex(res.buf, res.len);

	// lazy/bad check but eh
	return res.len > 8 ? 0 : -1;
}
#else
#define test_key_signing(x)
#endif

#ifdef TEST_EFM8UB1_EEPROM

void dump_eeprom()
{
	// 0xF800 - 0xFB7F
	uint16_t i = 0xF800;
	uint8_t eep;
	for (; i <= 0xF800 + 300; i++)
	{
		eeprom_read(i,&eep,1);
		u2f_putb(eep);
	}
}

int8_t test_efm8ub1_eeprom()
{
	uint16_t crc = 0;
	uint8_t secbyte;
	int8_t i;
	char k[] = "\x55\x66\x77\x88";
	char buf[4];

	eeprom_read(0xFBFF,&secbyte,1);

	u2f_printb("security_byte: ",1,secbyte);

	if (secbyte == 0xff)
	{
		eeprom_erase(0xFBC0);
		i = -32;
		eeprom_write(0xFBFF, &i, 1);
		u2f_prints("eeprom_write\r\n");
	}

	eeprom_write(KEYHANDLES_START + 0, k, 4);
	eeprom_write(KEYHANDLES_START + 4, k, 4);
	eeprom_write(KEYHANDLES_START + 8, k, 4);
	eeprom_write(KEYHANDLES_START + 12, k, 4);

	eeprom_read(KEYHANDLES_START + 0,buf,4);
	for(i=0; i < 4; i++) crc = feed_crc(crc, buf[i]);
	dump_hex(buf,4);
	dump_hex(k,4);
	eeprom_read(KEYHANDLES_START + 4,buf,4);
	for(i=0; i < 4; i++) crc = feed_crc(crc, buf[i]);
	dump_hex(buf,4);
	dump_hex(k,4);
	eeprom_read(KEYHANDLES_START + 8,buf,4);
	for(i=0; i < 4; i++) crc = feed_crc(crc, buf[i]);
	dump_hex(buf,4);
	dump_hex(k,4);
	eeprom_read(KEYHANDLES_START + 12,buf,4);
	for(i=0; i < 4; i++) crc = feed_crc(crc, buf[i]);
	dump_hex(buf,4);
	dump_hex(k,4);

	u2f_printx("crc: ", 1, crc);

	if (crc == 0xd1e8)
		return 0;
	return -1;
}

#else
#define test_efm8ub1_eeprom(x)
#endif


void run_tests()
{
	int rc;

#ifdef SHA_TEST
	PRINT("--- STARTING SHA TEST ---\r\n");
	rc = test_sha();
	if (rc == 0)
		PRINT("--- SHA TEST SUCCESS ---\r\n");
	else
		PRINT("--- SHA TEST FAILED %d ---\r\n",rc);
#endif

#ifdef TEST_ATECC_EEPROM
	PRINT("--- STARTING ATECC EEPROM TEST ---\r\n");
	rc = test_atecc_eeprom();
	if (rc == 0)
		PRINT("--- EEPROM TEST SUCCESS ---\r\n");
	else
		PRINT("--- EEPROM TEST FAILED %d ---\r\n",rc);
#endif

#ifdef TEST_KEY_SIGNING
	PRINT("--- STARTING KEY SIGNING TEST ---\r\n");
	rc = test_key_signing();
	if (rc == 0)
		PRINT("--- KEY SIGNING SUCCESS ---\r\n");
	else
		PRINT("--- KEY SIGNING FAILED %d ---\r\n",rc);
#endif

#ifdef TEST_EFM8UB1_EEPROM
	PRINT("--- STARTING EFM8UB1 EEPROM TEST ---\r\n");
	rc = test_efm8ub1_eeprom();
	if (rc == 0)
		PRINT("--- EFM8UB1 EEPROM SUCCESS ---\r\n");
	else
		PRINT("--- EFM8UB1 EEPROM FAILED ---\r\n");
#endif


}



#endif
