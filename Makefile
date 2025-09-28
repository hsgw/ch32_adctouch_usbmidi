all : flash

TARGET:=adctouch_usbmidi


TARGET_MCU?=CH32V003
ADDITIONAL_C_FILES+=../rv003usb/rv003usb/rv003usb.S ../rv003usb/rv003usb/rv003usb.c
EXTRA_CFLAGS:=-I../rv003usb/lib -I../rv003usb/rv003usb

include ../ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean


