MCU=atmega32
HZ=1843200

OBJ=wheel.o lcd.o time.o menu.o

CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS= -g -mmcu=$(MCU) -Wall -Wstrict-prototypes -Os -mcall-prologues \
 -DF_CPU=$(HZ)ul \
 -DLCD_PORT_NAME=A \
 -DLCD_NCHARS=8 \
 -DEEPROM_LEN=1024

AVRDUDE = avrdude -c usbasp -p m32 -B 10

all: wheel.hex

wheel.hex : wheel.out
	$(OBJCOPY) -R .eeprom -O ihex wheel.out wheel.hex 

wheel.out: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

load: wheel.hex
	$(AVRDUDE) -U flash:w:wheel.hex:i

fuse:
	$(AVRDUDE) -U lfuse:w:0xfd:m

clean:
	rm -f $(OBJ) *.out *.hex
