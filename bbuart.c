#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "bbuart.h"

static int port, ddr;

static uint32_t crc32(const unsigned char *buffer, uint32_t crc, int len)
{
	while (len--) {
		int i = 8;

		crc = crc ^ *buffer++;
		while (i--) {
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc = crc >> 1;
		}
	}

	return crc;
}

void bbuart_init(void)
{
	port = BBUART_PORT & BBUART_TX;
	ddr = BBUART_DDR & BBUART_TX;

	BBUART_PORT |= BBUART_TX;
	BBUART_DDR |= BBUART_TX;
}

void bbuart_deinit(void)
{
	BBUART_PORT = (BBUART_PORT & ~BBUART_TX) | port;
	BBUART_DDR = (BBUART_DDR & ~BBUART_TX) | ddr;
}

int bbuart_putchar(unsigned char c)
{
	int i, x = (1 << 9) | (c << 1);

	for (i = 0; i < 10; i++) {
		if (x & 1)
			BBUART_PORT |= BBUART_TX;
		else
			BBUART_PORT &= ~BBUART_TX;
		x >>= 1;
		_delay_us(1000000ul / BBUART_BAUDRATE - 6); /* FIXME */
	}
	return c;
}

int bbuart_puts(const char *s)
{
	while (*s)
		bbuart_putchar(*s++);
	return 0;
}

int bbuart_write(const unsigned char *s, int len, uint32_t *crc)
{
	if (crc)
		*crc = crc32(s, *crc, len);
	while (len--)
		bbuart_putchar(*s++);
	return 0;
}
