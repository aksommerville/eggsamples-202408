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
  EGG_ARCH:=linux
endif
EGGDEV:=$(EGG_SDK)/out/tools/eggdev
EGG:=$(EGG_SDK)/out/$(EGG_ARCH)/egg

# Wasm compiler and linker are required.
# I recommend llvm (ie clang and wasm-ld). WASI-SDK should work too.
CCWARN:=-Wno-comment -Wno-incompatible-library-redeclaration -Wno-parentheses -Werror
CC_WASM:=clang --target=wasm32 -c -O3 -MMD -nostdlib -I$(EGG_SDK)/src -I$(MIDDIR) -Isrc $(CCWARN)
LD_WASM_EXE:=$(firstword $(shell which wasm-ld) $(shell which wasm-ld-11))
LD_WASM:=$(LD_WASM_EXE) --no-entry -z stack-size=4194304 --no-gc-sections --allow-undefined --export-table \
  --export=egg_client_init --export=egg_client_quit --export=egg_client_update --export=egg_client_render

# Native C compiler is optional. Only needed if you want a true native executable in addition to bundled ones.
CC_NATIVE:=gcc -c -MMD -O3 -I$(EGG_SDK)/src -I$(MIDDIR) -Isrc -Wimplicit -Werror -DUSE_REAL_STDLIB=1
AR_NATIVE:=ar rc

SRCFILES:=$(shell find src -type f)
CFILES:=$(filter %.c,$(SRCFILES))
DATAFILES:=$(filter src/data/%,$(SRCFILES))

# Copying data is overkill, but I'm demonstrating it could be used for preprocessing resources.
# Add more specific overrides eg $(MIDDIR)/data/sprite/%:src/data/sprite/%;$(PRECMD) my-sprite-compiler.sh -o$@ $<
DATAFILES_MID:=$(patsubst src/data/%,$(MIDDIR)/data/%,$(DATAFILES))
DATAFLAG:=$(MIDDIR)/dataflag
DATAHEADER:=$(MIDDIR)/resid.h
$(shell $(EGGDEV) tocflag -o$(DATAFLAG) src/data --types=src/restypes)
$(DATAHEADER):$(DATAFLAG) src/data/string/* src/data/sound/* \
  ;$(PRECMD) rm -f $@ ; $(EGGDEV) toc src/data --named-only --types=src/restypes \
  | while read TID QUAL RID NAME ; do echo "#define RID_$${TID}_$${NAME} $${RID}" >>$@ ; done >>$@
$(MIDDIR)/data/%:src/data/%;$(PRECMD) cp $< $@

OFILES_WASM:=$(patsubst src/%.c,$(MIDDIR)/wasm/%.o,$(CFILES))
OFILES_NATIVE:=$(patsubst src/%.c,$(MIDDIR)/native/%.o,$(filter-out src/stdlib/%,$(CFILES)))

-include $(OFILES_WASM:.o=.d) $(OFILES_NATIVE:.o=.d)
$(MIDDIR)/wasm/%.o:src/%.c|$(DATAHEADER);$(PRECMD) $(CC_WASM) -o$@ $<
$(MIDDIR)/native/%.o:src/%.c|$(DATAHEADER);$(PRECMD) $(CC_NATIVE) -o$@ $<

WASM1:=$(MIDDIR)/data/wasm/1
$(WASM1):$(OFILES_WASM);$(PRECMD) $(LD_WASM) -o$@ $^
ROM:=$(OUTDIR)/lightson.egg
$(ROM):$(WASM1) $(DATAFILES_MID);$(PRECMD) $(EGGDEV) pack -o$@ $(MIDDIR)/data --types=src/restypes
all:$(ROM)

EXE_BUNDLED:=$(OUTDIR)/lightson-fake.exe
$(EXE_BUNDLED):$(ROM);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM)
all:$(EXE_BUNDLED)

HTML_BUNDLED:=$(OUTDIR)/lightson.html
$(HTML_BUNDLED):$(ROM);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM)
all:$(HTML_BUNDLED)

ifneq (,$(strip $(CC_NATIVE)))
  LIB_NATIVE:=$(MIDDIR)/lightson.a
  $(LIB_NATIVE):$(OFILES_NATIVE);$(PRECMD) $(AR_NATIVE) $@ $^
  EXE_NATIVE:=$(OUTDIR)/lightson-true.exe
  $(EXE_NATIVE):$(ROM) $(LIB_NATIVE);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM) --code=$(LIB_NATIVE)
  all:$(EXE_NATIVE)
  run-native:$(EXE_NATIVE);$(EXE_NATIVE)
endif

run:run-rom
run-rom:$(ROM);$(EGG) $(ROM)
run-bundled:$(EXE_BUNDLED);$(EXE_BUNDLED)

serve:$(ROM);$(EGGDEV) serve $(ROM) --htdocs=$(EGG_SDK)/src/www
# Can add `--external` to serve on external interfaces, eg to test on your phone. I don't recommend leaving that up long.

clean:;rm -rf $(MIDDIR) $(OUTDIR)
