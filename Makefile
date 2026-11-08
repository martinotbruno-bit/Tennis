# Pixel Open — construction de la ROM NES et des tests
CC65 ?= cc65
CA65 ?= ca65
LD65 ?= ld65
CFLAGS = -t nes -Oirs --static-locals -I src
BUILD = build
ROM = $(BUILD)/pixel_open.nes
CSRC = main game rules gfx audio password assets_data
OBJS = $(BUILD)/crt0.o $(addprefix $(BUILD)/,$(addsuffix .o,$(CSRC)))

all: $(ROM)

assets/tiles.chr src/assets_data.c: tools/gen_assets.py
	python3 tools/gen_assets.py

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/crt0.o: src/crt0.s assets/tiles.chr | $(BUILD)
	$(CA65) -t nes -o $@ src/crt0.s

$(BUILD)/%.s: src/%.c src/*.h | $(BUILD)
	$(CC65) $(CFLAGS) -o $@ $<

$(BUILD)/assets_data.s: src/assets_data.c | $(BUILD)
	$(CC65) $(CFLAGS) -o $@ $<

$(BUILD)/%.o: $(BUILD)/%.s
	$(CA65) -t nes -o $@ $<

$(ROM): $(OBJS) cfg/nrom.cfg
	$(LD65) -C cfg/nrom.cfg -o $@ -m $(BUILD)/pixel_open.map -Ln $(BUILD)/pixel_open.lbl --dbgfile $(BUILD)/pixel_open.dbg $(OBJS) nes.lib
	@python3 tools/rom_report.py $(BUILD)/pixel_open.map $@

# tests unitaires natifs (règles, codes) + tests sur émulateur
test: $(ROM)
	gcc -Wall -Wextra -o $(BUILD)/rules_test tests/rules_test.c src/rules.c && $(BUILD)/rules_test
	gcc -Wall -Wextra -o $(BUILD)/password_test tests/password_test.c src/password.c && $(BUILD)/password_test
	cd tests && npm install --silent --no-audit --no-fund && node emu_test.js ../$(ROM)

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
