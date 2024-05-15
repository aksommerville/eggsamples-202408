all:
.SILENT:
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

# Wasm compiler and linker are required.
# I recommend llvm (ie clang and wasm-ld). WASI-SDK should work too.
CCWARN:=-Wno-comment -Wno-incompatible-library-redeclaration -Wno-parentheses -Werror
CC_WASM:=clang --target=wasm32 -c -O3 -MMD -nostdlib -I$(EGG_SDK)/src -Isrc $(CCWARN)
LD_WASM_EXE:=$(firstword $(shell which wasm-ld) $(shell which wasm-ld-11))
LD_WASM:=$(LD_WASM_EXE) --no-entry -z stack-size=4194304 --no-gc-sections --allow-undefined --export-table \
  --export=egg_client_init --export=egg_client_quit --export=egg_client_update --export=egg_client_render

SRCFILES:=$(shell find src -type f)
CFILES:=$(filter %.c,$(SRCFILES))
DATAFILES:=$(filter src/data/%,$(SRCFILES))

# See lightson for a more capable data-build system, where you can insert custom preprocessors.
# Here in shmup, we're keeping it dead simple. No data-related rules at all.

OFILES_WASM:=$(patsubst src/%.c,$(MIDDIR)/wasm/%.o,$(CFILES))

-include $(OFILES_WASM:.o=.d) $(OFILES_NATIVE:.o=.d)
$(MIDDIR)/wasm/%.o:src/%.c|$(DATAHEADER);$(PRECMD) $(CC_WASM) -o$@ $<

WASM1:=$(MIDDIR)/data/wasm/1
$(WASM1):$(OFILES_WASM);$(PRECMD) $(LD_WASM) -o$@ $^
ROM:=$(OUTDIR)/shmup.egg
$(ROM):$(WASM1) $(DATAFILES);$(PRECMD) $(EGGDEV) pack -o$@ src/data $(WASM1)
all:$(ROM)

run:run-rom
run-rom:$(ROM);$(EGG) $(ROM)
serve:$(ROM);$(EGGDEV) serve $(ROM) --htdocs=$(EGG_SDK)/src/www
clean:;rm -rf $(MIDDIR) $(OUTDIR)

#-----------------------------------------------------------------
# Everything below is optional.
# Eliminate these top three lines to build nothing but the ROM.

HTML_BUNDLED:=$(OUTDIR)/shmup.html
EXE_BUNDLED:=$(OUTDIR)/shmup-fake.exe
EXE_NATIVE:=$(OUTDIR)/shmup-true.exe

CC_NATIVE:=gcc -c -MMD -O3 -I$(EGG_SDK)/src -I$(MIDDIR) -Isrc -Wimplicit -Werror -DUSE_REAL_STDLIB=1
AR_NATIVE:=ar rc

ifneq (,$(strip $(EXE_BUNDLED)))
  $(EXE_BUNDLED):$(ROM);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM)
  all:$(EXE_BUNDLED)
  run-bundled:$(EXE_BUNDLED);$(EXE_BUNDLED)
endif

ifneq (,$(strip $(HTML_BUNDLED)))
  $(HTML_BUNDLED):$(ROM);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM)
  all:$(HTML_BUNDLED)
endif

ifneq (,$(strip $(EXE_NATIVE)))
  OFILES_NATIVE:=$(patsubst src/%.c,$(MIDDIR)/native/%.o,$(CFILES))
  $(MIDDIR)/native/%.o:src/%.c|$(DATAHEADER);$(PRECMD) $(CC_NATIVE) -o$@ $<
  LIB_NATIVE:=$(MIDDIR)/shmup.a
  $(LIB_NATIVE):$(OFILES_NATIVE);$(PRECMD) $(AR_NATIVE) $@ $^
  $(EXE_NATIVE):$(ROM) $(LIB_NATIVE);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM) --code=$(LIB_NATIVE)
  all:$(EXE_NATIVE)
  run-native:$(EXE_NATIVE);$(EXE_NATIVE)
endif

