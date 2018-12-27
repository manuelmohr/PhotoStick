ARDUINO ?= ~/Programs/arduino-1.8.8/arduino
PORT ?= /dev/ttyACM0

all: Pixelstick.ino
	$(ARDUINO) --verify $<

# -Wl,-Map=output.map
debug: Pixelstick.ino
	$(ARDUINO) --verbose-build --preserve-temp-files --verify $<

upload: Pixelstick.ino
	$(ARDUINO) --upload $< --port $(PORT)
