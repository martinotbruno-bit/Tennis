# Royal Martinot — construction de la ROM NES (MMC1, 128 Ko) et des tests
CC65 ?= cc65
CA65 ?= ca65
LD65 ?= ld65
CFLAGS = -t nes -Oirs --static-locals -g -I src
BUILD = build
ROM = $(BUILD)/royal_martinot.nes
# banque fixe ($C000)
FIXED = main rules gfx audio assets_data gamefix
# banques commutables ($8000)
BANK0 = game
BANK1 = screens
BANK5 = comp stats password
BANK3 = extras assets_court
BANK4 = manual
BANK6 = assets_nude
BANK7 = intro
BANK8 = podium
BANK9 = credits
CSRC = $(FIXED) $(BANK0) $(BANK1) $(BANK3) $(BANK4) $(BANK5) $(BANK6) $(BANK7) $(BANK8) $(BANK9)
OBJS = $(BUILD)/crt0.o $(BUILD)/sprp.o $(addprefix $(BUILD)/,$(addsuffix .o,$(CSRC)))

all: $(ROM)

assets/tiles.chr src/assets_data.c src/assets_court.c src/assets_nude.c: tools/gen_assets.py
	python3 tools/gen_assets.py

$(BUILD):
	mkdir -p $(BUILD)

# segments par banque
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK0))): SEGS = --code-name BANK0 --rodata-name RODATA0
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK1))): SEGS = --code-name BANK1 --rodata-name RODATA1 --bss-name WBSS
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK3))): SEGS = --code-name BANK3 --rodata-name RODATA3
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK4))): SEGS = --code-name BANK4 --rodata-name RODATA4
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK5))): SEGS = --code-name BANK5 --rodata-name RODATA5 --bss-name WBSS
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK6))): SEGS = --code-name BANK6 --rodata-name RODATA6
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK7))): SEGS = --code-name BANK7 --rodata-name RODATA7 --bss-name WBSS
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK8))): SEGS = --code-name BANK8 --rodata-name RODATA8 --bss-name WBSS
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK9))): SEGS = --code-name BANK9 --rodata-name RODATA9 --bss-name WBSS

$(BUILD)/crt0.o: src/crt0.s assets/tiles.chr | $(BUILD)
	$(CA65) -t nes -g -o $@ src/crt0.s

$(BUILD)/sprp.o: src/sprp.s | $(BUILD)
	$(CA65) -t nes -g -o $@ src/sprp.s

$(BUILD)/%.s: src/%.c src/*.h src/assets_data.c src/assets_court.c src/assets_nude.c | $(BUILD)
	$(CC65) $(CFLAGS) $(SEGS) -o $@ $<

$(BUILD)/%.o: $(BUILD)/%.s
	$(CA65) -t nes -g -o $@ $<

$(ROM): $(OBJS) cfg/mmc1.cfg
	$(LD65) -C cfg/mmc1.cfg -o $@ -m $(BUILD)/royal_martinot.map -Ln $(BUILD)/royal_martinot.lbl --dbgfile $(BUILD)/royal_martinot.dbg $(OBJS) nes.lib
	@python3 tools/rom_report.py $(BUILD)/royal_martinot.map $@

# tests unitaires natifs (règles, codes) + tests sur émulateur
test: $(ROM)
	python3 tools/check_texts.py
	python3 tools/contrast.py 25
	gcc -Wall -Wextra -o $(BUILD)/rules_test tests/rules_test.c src/rules.c && $(BUILD)/rules_test
	gcc -Wall -Wextra -O2 -o $(BUILD)/password_test tests/password_test.c src/password.c && $(BUILD)/password_test
	cd tests && npm install --silent --no-audit --no-fund && node emu_test.mjs ../$(ROM)

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
