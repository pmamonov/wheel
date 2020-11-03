## Prerequisites

```
$ sudo apt install gcc-avr avr-libc binutils-avr avrdude
```

## Download the project

```
$ git clone -b ref https://github.com/pmamonov/wheel
```

## Build & upload

```
$ cd wheel/
$ make load && make fuse
```
