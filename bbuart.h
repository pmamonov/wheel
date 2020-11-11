#include <stdint.h>

#define BBUART_BAUDRATE	9600
#define BBUART_PORT	PORTD
#define BBUART_DDR	DDRD
#define BBUART_TX	(1 << 3)

void bbuart_init(void);

void bbuart_deinit(void);

int bbuart_putchar(unsigned char c);

int bbuart_puts(const char *s);

int bbuart_write(const unsigned char *s, int len, uint32_t *crc);
