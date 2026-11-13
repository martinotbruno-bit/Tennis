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
BANK3 = extras assets_court assets_persp3
BANK2 = assets_tz2
BANK4 = manual assets_tz4
BANK6 = assets_nude assets_gm2 assets_tz6
BANK7 = intro teaser assets_tz7
BANK8 = podium menuart assets_menu
BANK9 = credits assets_tz9
BANK10 = music_data assets_tz10 assets_vs
BANK11 = assets_persp
BANK12 = assets_courts12 assets_tz12 assets_menu12
BANK13 = assets_swap shots cer
BANK14 = assets_gm gmintro
CSRC = $(FIXED) $(BANK0) $(BANK1) $(BANK2) $(BANK3) $(BANK4) $(BANK5) $(BANK6) $(BANK7) $(BANK8) $(BANK9) $(BANK10) $(BANK11) $(BANK12) $(BANK13) $(BANK14)
OBJS = $(BUILD)/crt0.o $(BUILD)/sprp.o $(BUILD)/poses.o $(BUILD)/slip.o $(BUILD)/dpcm.o $(addprefix $(BUILD)/,$(addsuffix .o,$(CSRC)))

all: $(ROM)

assets/tiles.chr src/assets_data.c src/assets_court.c src/assets_nude.c src/assets_persp.c src/assets_persp3.c src/assets_courts12.c src/assets_swap.c: tools/gen_assets.py
src/assets_gm.c src/assets_gm2.c: tools/gen_gm.py
	python3 tools/gen_gm.py
	python3 tools/gen_assets.py

src/assets_menu.c src/assets_menu12.c: tools/gen_menu.py assets/tiles.chr
	python3 tools/gen_menu.py

src/assets_vs.c src/assets_vs.h src/assets_tz.h src/assets_tz2.c src/assets_tz4.c src/assets_tz9.c src/assets_tz10.c src/assets_tz12.c src/assets_tz6.c src/assets_tz7.c: tools/gen_teaser.py
	python3 tools/gen_teaser.py

src/music_data.c: tools/gen_music.py tools/music_src.c
	python3 tools/gen_music.py

$(BUILD):
	mkdir -p $(BUILD)

# segments par banque
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK0))): SEGS = --code-name BANK0 --rodata-name RODATA0
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK1))): SEGS = --code-name BANK1 --rodata-name RODATA1 --bss-name WBSS
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK2))): SEGS = --code-name BANK2 --rodata-name RODATA2
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK3))): SEGS = --code-name BANK3 --rodata-name RODATA3
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK4))): SEGS = --code-name BANK4 --rodata-name RODATA4
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK5))): SEGS = --code-name BANK5 --rodata-name RODATA5 --bss-name WBSS
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK6))): SEGS = --code-name BANK6 --rodata-name RODATA6
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK7))): SEGS = --code-name BANK7 --rodata-name RODATA7 --bss-name WBSS
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK8))): SEGS = --code-name BANK8 --rodata-name RODATA8 --bss-name WBSS
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK9))): SEGS = --code-name BANK9 --rodata-name RODATA9 --bss-name WBSS
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK10))): SEGS = --code-name BANK10 --rodata-name RODATA10
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK11))): SEGS = --code-name BANK11 --rodata-name RODATA11
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK12))): SEGS = --code-name BANK12 --rodata-name RODATA12
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK13))): SEGS = --code-name BANK13 --rodata-name RODATA13
$(addprefix $(BUILD)/,$(addsuffix .s,$(BANK14))): SEGS = --code-name BANK14 --rodata-name RODATA14

$(BUILD)/crt0.o: src/crt0.s assets/tiles.chr | $(BUILD)
	$(CA65) -t nes -g -o $@ src/crt0.s

$(BUILD)/sprp.o: src/sprp.s | $(BUILD)
	$(CA65) -t nes -g -o $@ src/sprp.s

$(BUILD)/poses.o: src/poses.s | $(BUILD)
	$(CA65) -t nes -g -o $@ src/poses.s
$(BUILD)/slip.o: src/slip.s | $(BUILD)
	$(CA65) -t nes -g -o $@ src/slip.s

$(BUILD)/dpcm.o: src/dpcm.s | $(BUILD)
	$(CA65) -t nes -g -o $@ src/dpcm.s

$(BUILD)/%.s: src/%.c src/*.h src/assets_data.c src/assets_court.c src/assets_nude.c src/assets_persp.c src/assets_persp3.c src/assets_courts12.c src/assets_swap.c src/assets_gm.c src/assets_gm2.c src/assets_tz.h src/assets_vs.h | $(BUILD)
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
	cd tests && npm install --silent --no-audit --no-fund && node run_tests.mjs ../$(ROM)

# contrôle rapide d'une modification localisée : sections dont le nom contient
# l'un des mots de T (ex. make test-only T=bande-annonce,plongeon)
test-only: $(ROM)
	cd tests && node run_tests.mjs ../$(ROM) --only=$(T)

clean:
	rm -rf $(BUILD)

.PHONY: all test test-only clean
