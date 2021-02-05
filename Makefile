-include config.mk

all: Photostick.ino
	$(ARDUINO) --verify $<

# -Wl,-Map=output.map
debug: Photostick.ino
	$(ARDUINO) --verbose-build --preserve-temp-files --verify $<

upload: Photostick.ino
	$(ARDUINO) --upload $< --port $(PORT)
