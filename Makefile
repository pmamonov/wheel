MCU=atmega32
CS=1.8432
TSVAL=`echo '65536 - $(CS)*100000/64' | bc`

CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS= -g -mmcu=$(MCU) -Wall -Wstrict-prototypes -Os -mcall-prologues \
 -DEEPROM_LEN=1024 \
 -DF_CPU=`echo '$(CS)*1000000' | bc`L \
 -DLCD_PORT_NAME=A \
 -DLCD_TYPE=161 \
 -DTIM_START_VAL=$(TSVAL)
# -DTIM_START_VAL=0xffff

all: wheel.hex

wheel.hex : wheel.out
	$(OBJCOPY) -R .eeprom -O ihex wheel.out wheel.hex 

wheel.out : wheel.o lcd.o time.o
	$(CC) $(CFLAGS) -o wheel.out -Wl,-Map,wheel.map wheel.o lcd.o time.o

wheel.o : wheel.c
	$(CC) $(CFLAGS) -c wheel.c

lcd.o : ../lcd/lcd.c
	$(CC) $(CFLAGS) -o lcd.o -c ../lcd/lcd.c

time.o : ../time/time.c
	$(CC) $(CFLAGS) -o time.o -c ../time/time.c

load: wheel.hex
	avrdude -c usbasp -p m32 -U flash:w:wheel.hex:i

clean:
	rm -f *.o *.map *.out *.hex

#timestamp:
#	avrdude -c dapa -p m16 -U lfuse:r:lfuse.bin:r && date +%s > timestamp
#
#ttest: timestamp
#	(cat timestamp && date +%s) | awk -F: 'NR==1 {t0=$$0} NR==2 {printf("\n%d:%d:%d\n", int(($$0-t0)/3600), int((($$0-t0)%3600)/60), ($$0-t0)%60); }'
