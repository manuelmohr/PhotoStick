ARDUINO ?= ~/Programs/arduino-1.8.12/arduino
PORT ?= /dev/ttyACM0

all: Photostick.ino
	$(ARDUINO) --verify $<

# -Wl,-Map=output.map
debug: Photostick.ino
	$(ARDUINO) --verbose-build --preserve-temp-files --verify $<

upload: Photostick.ino
	$(ARDUINO) --upload $< --port $(PORT)
