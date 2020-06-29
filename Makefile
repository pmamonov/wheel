MCU=atmega32
CS=1.8432
TSVAL=`echo '65536 - $(CS)*100000/64' | bc`

OBJ=wheel.o lcd.o time.o

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

wheel.out: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

load: wheel.hex
	avrdude -c usbasp -p m32 -U flash:w:wheel.hex:i

clean:
	rm -f $(OBJ) *.out *.hex
