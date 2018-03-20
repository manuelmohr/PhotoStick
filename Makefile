ARDUINO ?= ~/Programs/arduino-1.8.5/arduino
PORT ?= /dev/ttyACM0

all: Pixelstick.ino
	$(ARDUINO) --verify $<

upload:
	$(ARDUINO) --upload $< --port $(PORT)
