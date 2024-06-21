all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

MIDDIR:=mid
OUTDIR:=out

ifndef EGG_SDK
  EGG_SDK:=../../egg
endif
ifndef EGG_ARCH
  EGG_ARCH:=$(firstword $(filter linux macos mswin,$(notdir $(wildcard $(EGG_SDK)/out/*))))
endif
EGGDEV:=$(EGG_SDK)/out/tools/eggdev
EGG:=$(EGG_SDK)/out/$(EGG_ARCH)/egg

PROJNAME:=arrautza

# List final outputs. ROM is required; the other three, you can blank if you don't want them.
# EXE_NATIVE requires CC_NATIVE and AR_NATIVE set.
# EXE_BUILDER is only necessary during build, but I call it a "final output" anyway.
ROM:=$(OUTDIR)/$(PROJNAME).egg
EXE_BUNDLED:=$(OUTDIR)/$(PROJNAME)-fake.exe
HTML_BUNDLED:=$(OUTDIR)/$(PROJNAME).html
EXE_NATIVE:=$(OUTDIR)/$(PROJNAME)-true.exe
EXE_BUILDER:=$(OUTDIR)/builder

# Which thing to run by default? run-rom, run-bundled, run-native
run:run-rom

# Wasm compiler and linker are required.
# I recommend llvm (ie clang and wasm-ld). WASI-SDK should work too.
# If you want to build exclusively true-native, you could skip the Wasm, but I'm not coding for that here.
# Native compiler and linker are also required; we use them for 'builder'.
CCWARN:=-Wno-comment -Wno-incompatible-library-redeclaration -Wno-parentheses -Werror
CC_WASM:=clang --target=wasm32 -c -O3 -MMD -nostdlib -I$(EGG_SDK)/src -I$(MIDDIR) -Isrc $(CCWARN)
LD_WASM_EXE:=$(firstword $(shell which wasm-ld) $(shell which wasm-ld-11))
LD_WASM:=$(LD_WASM_EXE) --no-entry -z stack-size=4194304 --no-gc-sections --allow-undefined --export-table \
  --export=egg_client_init --export=egg_client_quit --export=egg_client_update --export=egg_client_render
CC_NATIVE:=gcc -c -MMD -O3 -I$(EGG_SDK)/src -I$(MIDDIR) -Isrc -Wimplicit -Werror -DUSE_REAL_STDLIB=1
AR_NATIVE:=ar rc
LD_NATIVE:=gcc
LDPOST_NATIVE:=

# Rules to generate data files.
# It's normal to have just the one `cp` (in which case a lot of the "DATA" stuff below is redundant).
DATAHEADER:=$(MIDDIR)/resid.h
$(MIDDIR)/data/%:src/data/%;$(PRECMD) cp $< $@
$(MIDDIR)/data/map/%:src/data/map/% $(EXE_BUILDER) $(DATAHEADER);$(PRECMD) $(EXE_BUILDER) -o$@ $< -tmap -h$(DATAHEADER)

# Can add `--external` to serve on external interfaces, eg to test on your phone. Don't leave that on.
serve:$(ROM);$(EGGDEV) serve $(ROM) --htdocs=$(EGG_SDK)/src/www

edit:$(ROM);$(EGGDEV) serve $(ROM) --htdocs=$(EGG_SDK)/src/editor --runtime=$(EGG_SDK)/src/www --override=src/editor --data=src/data

#-----------------------------------------
# Everything below this point should just work, all configuration is above.

SRCFILES:=$(shell find -L src -type f)
CFILES:=$(filter %.c,$(SRCFILES))
DATAFILES:=$(filter src/data/%,$(SRCFILES))

DATAFILES_MID:=$(patsubst src/data/%,$(MIDDIR)/data/%,$(DATAFILES))
DATAFLAG:=$(MIDDIR)/dataflag
$(shell $(EGGDEV) tocflag -o$(DATAFLAG) src/data --types=src/restypes)
$(DATAHEADER):$(DATAFLAG) $(wildcard src/data/string/* src/data/sound/*) etc/tool/genhdr.js \
  ;$(PRECMD) $(EGGDEV) toc src/data --named-only --types=src/restypes | node etc/tool/genhdr.js -o$@ --types=src/restypes

CFILES_BUILDER:=$(filter src/builder/%,$(CFILES))
CFILES_GAME:=$(filter-out src/builder/%,$(CFILES))
OFILES_WASM:=$(patsubst src/%.c,$(MIDDIR)/wasm/%.o,$(CFILES_GAME))
OFILES_NATIVE:=$(patsubst src/%.c,$(MIDDIR)/native/%.o,$(filter-out src/stdlib/%,$(CFILES_GAME)))
OFILES_BUILDER:=$(patsubst src/builder/%.c,$(MIDDIR)/builder/%.o,$(CFILES_BUILDER))

-include $(OFILES_WASM:.o=.d) $(OFILES_NATIVE:.o=.d) $(OFILES_BUILDER:.o=.d)
$(MIDDIR)/wasm/%.o:src/%.c|$(DATAHEADER);$(PRECMD) $(CC_WASM) -o$@ $<
$(MIDDIR)/native/%.o:src/%.c|$(DATAHEADER);$(PRECMD) $(CC_NATIVE) -o$@ $<
$(MIDDIR)/builder/%.o:src/builder/%.c;$(PRECMD) $(CC_NATIVE) -o$@ $<

WASM1:=$(MIDDIR)/data/wasm/1
$(WASM1):$(OFILES_WASM);$(PRECMD) $(LD_WASM) -o$@ $^
$(ROM):$(WASM1) $(DATAFILES_MID);$(PRECMD) $(EGGDEV) pack -o$@ $(MIDDIR)/data --types=src/restypes
all:$(ROM)
run-rom:$(ROM);$(EGG) $(ROM)

ifneq (,$(strip $(EXE_BUNDLED)))
  $(EXE_BUNDLED):$(ROM);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM)
  all:$(EXE_BUNDLED)
  run-bundled:$(EXE_BUNDLED);$(EXE_BUNDLED)
endif

ifneq (,$(strip $(HTML_BUNDLED)))
  $(HTML_BUNDLED):$(ROM);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM)
  all:$(HTML_BUNDLED)
  # No "run" or "serve" for this one. You can open it directly in a web browser.
endif

ifneq (,$(strip $(EXE_NATIVE)))
  LIB_NATIVE:=$(MIDDIR)/$(PROJNAME).a
  $(LIB_NATIVE):$(OFILES_NATIVE);$(PRECMD) $(AR_NATIVE) $@ $^
  $(EXE_NATIVE):$(ROM) $(LIB_NATIVE);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM) --code=$(LIB_NATIVE)
  all:$(EXE_NATIVE)
  run-native:$(EXE_NATIVE);$(EXE_NATIVE)
endif

$(EXE_BUILDER):$(OFILES_BUILDER);$(PRECMD) $(LD_NATIVE) -o$@ $(OFILES_BUILDER) $(LDPOST_NATIVE)
all:$(EXE_BUILDER)

clean:;rm -rf $(MIDDIR) $(OUTDIR)
