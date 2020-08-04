#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "err.h"

#include "lcd.h"
#include "time.h"
#include "menu.h"

#define HZ	10
#define TIM_START_VAL (0x10000ul - F_CPU / 64 / HZ)

#define LCD_BS	(16 + 1)

#define KEY_LEFT	0xe
#define KEY_RIGHT	0xb
#define KEY_UP		0xd
#define KEY_DOWN	0x7

#define ROM_SZ		1024

struct dh {
	uint32_t magic;
#define DATA_MAGIC	0xda7ada7a
	uint32_t time;
	uint32_t period;
	uint16_t dsz;
	uint16_t lenp;
#define DATA_NLEN	8
	uint16_t len[DATA_NLEN];
} datah;

volatile unsigned long	jiffies;
unsigned long time;
struct tm tm;

volatile unsigned long sw0;
volatile unsigned char sw1;

unsigned long tdata;
unsigned long count_prev;

ISR(TIMER1_OVF_vect)
{
	TCNT1 = TIM_START_VAL;
	jiffies++;
}

ISR(INT0_vect)
{
	if (sw1)
		sw0++;
	sw1 = 0;
}

ISR(INT1_vect)
{
	sw1 = 1;
}

unsigned long wheel_get(void)
{
	unsigned long t;

	do {
		t = sw0;
	} while(t != sw0);

	return t;
}

void wheel_reset(void)
{
	cli();
	sw0 = 0;
	sei();
}

unsigned long clock(void)
{
	unsigned long t;

	do {
		t = jiffies;
	} while (t != jiffies);

	return t;
}


void data_load(struct dh *dh)
{
	eeprom_read_block(dh, (void *)0, sizeof(*dh));
}

int data_init(struct dh *dh, unsigned long period)
{
	int lenp = 0;

	if (!dh)
		return EINVAL;

	if (dh->magic == DATA_MAGIC)
		lenp = (dh->lenp + 1) % DATA_NLEN;

	memset(dh, 0, sizeof(*dh));

	dh->magic = DATA_MAGIC;
	dh->lenp = lenp;
	dh->period = period;
	dh->time = time;

	/* Calculate required datah size */
#define WHEEL_HZ	1ul
	if (period * WHEEL_HZ < 0x100)
		dh->dsz = 1;
	else if (period * WHEEL_HZ < 0x10000)
		dh->dsz = 2;
	else
		dh->dsz = 4;

	eeprom_write_block(dh, (void *)0, sizeof(*dh));

	return 0;
}

int data_append(struct dh *dh, unsigned long d)
{
	uint16_t *plen, _len;
	int loff, off;

	if (!dh)
		return -EINVAL;

	plen = &dh->len[dh->lenp];
	loff = (void *)plen - (void *)dh;
	off = sizeof(*dh) + dh->dsz * (*plen);
	*plen += 1;

	if (off + dh->dsz > ROM_SZ)
		return -ENOSPC;

	eeprom_write_block(&d, (void *)off, dh->dsz); /* little endian */
	eeprom_write_block(plen, (void *)loff, sizeof(*plen));
	eeprom_read_block(&_len, (void *)loff, sizeof(*plen));

	if (_len != *plen)
		return -EIO;

	return 0;
}

unsigned long data_get(struct dh *dh, int i)
{
	int off;
	unsigned long x = 0;

	if (!dh || dh->magic != DATA_MAGIC || i >= dh->len[dh->lenp])
		return -EINVAL;

	off = sizeof(*dh) + dh->dsz * i;
	eeprom_read_block(&x, (void *)off, dh->dsz); /* little endian */

	return x;
}

void _render(char *s)
{
	lcd_Clear();
	lcd_Print(s);
}

struct menu_it	mit_clock, mit_clock_day,
		mit_clock_mon, mit_clock_yr,
		mit_clock_hr, mit_clock_min;

struct menu_it	mit_wheel, mit_period, mit_start;

struct menu_it	mit_data, mit_data_start,
		mit_data_per, mit_data_np, mit_data_view;

struct menu_it *clock_apply(struct menu_it *m)
{
	date2stamp(&tm, &time);
	return &mit_clock;
}

void clock_r(struct menu_it *m)
{
	char s[LCD_BS];

	stamp2date(time, &tm);
	snprintf(s, LCD_BS, "%4d-%02d-%02d %02d%c%02d",
		 tm.year, tm.mon, tm.day,
		 tm.hour, time % 2 ? ':' : ' ', tm.min);
	_render(s);
}

struct menu_it *clock_yr_inc(struct menu_it *m)
{
	if (tm.year < 2100)
		tm.year++;
	return m;
}

struct menu_it *clock_yr_dec(struct menu_it *m)
{
	if (tm.year > 2000)
		tm.year--;
	return m;
}

void clock_yr_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "YEAR: %d", tm.year);
	_render(s);
}

void clock_mon_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "MONTH: %d", tm.mon);
	_render(s);
}

struct menu_it *clock_mon_inc(struct menu_it *m)
{
	if (tm.mon < 12)
		tm.mon++;
	return m;
}

struct menu_it *clock_mon_dec(struct menu_it *m)
{
	if (tm.mon > 1)
		tm.mon--;
	return m;
}

void clock_day_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "DAY: %d", tm.day);
	_render(s);
}

struct menu_it *clock_day_inc(struct menu_it *m)
{
	if (tm.day < 31)
		tm.day++;
	return m;
}

struct menu_it *clock_day_dec(struct menu_it *m)
{
	if (tm.day > 1)
		tm.day--;
	return m;
}

void clock_hr_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "HOUR: %d", tm.hour);
	_render(s);
}

struct menu_it *clock_hr_inc(struct menu_it *m)
{
	if (tm.hour < 23)
		tm.hour++;
	return m;
}

struct menu_it *clock_hr_dec(struct menu_it *m)
{
	if (tm.hour > 0)
		tm.hour--;
	return m;
}

void clock_min_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "MINUTE: %d", tm.min);
	_render(s);
}

struct menu_it *clock_min_inc(struct menu_it *m)
{
	if (tm.min < 60)
		tm.min++;
	return m;
}

struct menu_it *clock_min_dec(struct menu_it *m)
{
	if (tm.min > 1)
		tm.min--;
	return m;
}

struct menu_it mit_clock = {
	.render = clock_r,
	.lrud = {&mit_data, &mit_wheel, 0, &mit_clock_yr},
};

struct menu_it mit_clock_yr = {
	.render = clock_yr_r,
	.x = 0b1110,
	.lrud = {clock_yr_dec, clock_yr_inc, clock_apply, &mit_clock_mon},
};

struct menu_it mit_clock_mon = {
	.render = clock_mon_r,
	.x = 0b1100,
	.lrud = {clock_mon_dec, clock_mon_inc, &mit_clock_yr, &mit_clock_day},
};

struct menu_it mit_clock_day = {
	.render = clock_day_r,
	.x = 0b1100,
	.lrud = {clock_day_dec, clock_day_inc, &mit_clock_mon, &mit_clock_hr},
};

struct menu_it mit_clock_hr = {
	.render = clock_hr_r,
	.x = 0b1100,
	.lrud = {clock_hr_dec, clock_hr_inc, &mit_clock_day, &mit_clock_min},
};

struct menu_it mit_clock_min = {
	.render = clock_min_r,
	.x = 0b1101,
	.lrud = {clock_min_dec, clock_min_inc, &mit_clock_hr, clock_apply},
};

void wheel_r(struct menu_it *m)
{
	char s[LCD_BS];
	unsigned long c = wheel_get();

	snprintf(s, LCD_BS, "PT %d/%d: %ld",
		 tdata ? datah.len[datah.lenp] + 1 : -1,
		 tdata ? (EEPROM_LEN - sizeof(datah)) / datah.dsz : -1,
		 c - count_prev);
	_render(s);
}

struct menu_it mit_wheel = {
	.render = wheel_r,
	.lrud = {&mit_clock, 0, 0, &mit_period},
};

unsigned long data_period = 1;

void period_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "PERIOD: %ld min", data_period);
	_render(s);
}

struct menu_it *period_inc(struct menu_it *m)
{
	data_period++;

	return m;
}

struct menu_it *period_dec(struct menu_it *m)
{
	if (data_period > 1)
		data_period--;

	return m;
}

struct menu_it mit_period = {
	.render = period_r,
	.x = 0b1100,
	.lrud = {period_dec, period_inc, &mit_wheel, &mit_start},
};

void start_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "DELETE OLD DATA?");
	_render(s);
}

struct menu_it *do_start(struct menu_it *m)
{
	data_init(&datah, data_period * 60);
	wheel_reset();
	count_prev = 0;
	tdata = clock() + datah.period * HZ;

	return &mit_wheel;
}

struct menu_it mit_start = {
	.render = start_r,
	.x = 0b0001,
	.lrud = {0, 0, &mit_period, do_start},
};

void data_r(struct menu_it *m)
{
	char s[LCD_BS];
	
	snprintf(s, LCD_BS,
		 datah.magic == DATA_MAGIC ? "VIEW DATA" : "NO DATA");
	_render(s);
}

unsigned dvi;

struct menu_it *data_enter(struct menu_it *m)
{
	dvi = 0;
	return datah.magic == DATA_MAGIC ?  &mit_data_start : m;
}


struct menu_it mit_data = {
	.render = data_r,
	.x = 0b0001,
	.lrud = {0, &mit_clock, 0, data_enter},
};

void data_start_r(struct menu_it *m)
{
	char s[LCD_BS];
	struct tm stm;

	stamp2date(datah.time, &stm);
	snprintf(s, LCD_BS, "%4d-%02d-%02d %02d:%02d",
		 stm.year, stm.mon, stm.day,
		 stm.hour, stm.min);
	_render(s);
}

struct menu_it mit_data_start = {
	.render = data_start_r,
	.lrud = {0, 0, &mit_data, &mit_data_per},
};

void data_per_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "PERIOD: %ld min", datah.period / 60); 
	_render(s);
}

struct menu_it mit_data_per = {
	.render = data_per_r,
	.lrud = {0, 0, &mit_data_start, &mit_data_np},
};

void data_np_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "NP: %d x %dB /%d",
		 datah.len[datah.lenp], datah.dsz, datah.lenp);
	_render(s);
}

struct menu_it mit_data_np = {
	.render = data_np_r,
	.lrud = {0, 0, &mit_data_per, &mit_data_view},
};

void data_view_r(struct menu_it *m)
{
	char s[LCD_BS];

	snprintf(s, LCD_BS, "PT %d: %ld", dvi + 1, data_get(&datah, dvi));
	_render(s);
}

struct menu_it *dv_prev(struct menu_it *m)
{
	if (dvi)
		dvi--;

	return m;
}

struct menu_it *dv_next(struct menu_it *m)
{
	if (dvi + 1 < datah.len[datah.lenp])
		dvi++;

	return m;
}

struct menu_it mit_data_view = {
	.render = data_view_r,
	.x = 0b1100,
	.lrud = {dv_prev, dv_next, &mit_data_np, 0},
};

int main(void)
{
	struct menu_it *menu = &mit_clock;
	unsigned long tkbd = 0, tlcd = 0, tup = 0;

	GICR |= (1 << INT0) | (1 << INT1); // enable int0, int1
	MCUCR |= 0xa; // xxxx1010: int0,1 on falling edge

	TIMSK |= (1 << TOIE1); // enable int on timer overflow
	TCNT1 = TIM_START_VAL;
	TCCR1B |= (1 << CS11) | (1 << CS10); // prescaler 1/64

	sei();

	/* enable pull-ups */
	PORTB = 0xf;
	PORTD = (1 << 2) | (1 << 3);

	lcd_Init();
	lcd_Print("0123456789abcdef");

	data_load(&datah);

	while (clock() < 2 * HZ)
		;

	while (1) {
		unsigned long t = clock();

		if (tdata && t >= tdata) {
			unsigned long c = wheel_get();

			tdata += datah.period * HZ;
			data_append(&datah, c - count_prev);
			count_prev = c;
		}

		if (t >= tup + HZ) {
			int n = (t - tup) / HZ;
			tup += n * HZ;
			time += n;
		}

		if (t >= tlcd &&
		    (menu == &mit_clock ||
		     menu == &mit_wheel)) {
			tlcd = t + HZ / 2;
			menu->render(menu);
		}

		if (t >= tkbd) {
			tkbd = t + HZ / 5;
			switch (PINB & 0xf) {
			case KEY_LEFT:
				menu = menu_proc(menu, MENU_LEFT);
				break;
			case KEY_RIGHT:
				menu = menu_proc(menu, MENU_RIGHT);
				break;
			case KEY_UP:
				menu = menu_proc(menu, MENU_UP);
				break;
			case KEY_DOWN:
				menu = menu_proc(menu, MENU_DOWN);
				break;
			}
		}
	}
}
