all:
.SILENT:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

ifeq (,$(EGG_SDK))
  EGG_SDK:=../../egg
endif
ifeq (,$(EGG_ARCH))
  EGG_ARCH:=linux
endif
ifeq (,$(WASI_SDK))
  WASI_SDK:=../../thirdparty/wasi-sdk-16.0
endif

CC:=$(WASI_SDK)/bin/clang -c -MMD -O3 -Isrc -I$(EGG_SDK)/src -Werror -Wimplicit -Wno-comment -Wno-parentheses
LD:=$(WASI_SDK)/bin/clang -Wl,--no-entry -Wl,--export-table -Wl,--export-all -Wl,--import-undefined -Wl,--allow-undefined -Wl,--initial-memory=10485760

clean:;rm -rf mid out

DATAFILES:=$(shell find data -type f)
CFILES:=$(shell find src -name '*.c')

OFILES:=$(patsubst src/%.c,mid/%.o,$(CFILES))
-include $(OFILES:.o=.d)
mid/%.o:src/%.c;$(PRECMD) $(CC) -o$@ $<

WASM1:=mid/wasm/1
$(WASM1):$(OFILES);$(PRECMD) $(LD) -o$@ $(OFILES)

ROM:=out/cfood.egg
all:$(ROM)
$(ROM):$(WASM1) $(DATAFILES);$(PRECMD) $(EGG_SDK)/out/tool/eggrom -c -o$@ mid/wasm data

run:$(ROM);$(EGG_SDK)/out/$(EGG_ARCH)/egg $(ROM)

serve:$(ROM);make --no-print-directory -C$(EGG_SDK) && $(EGG_SDK)/out/tool/server --port=8080 --htdocs=$(EGG_SDK)/src/web --rom=$(ROM)
# Add "--listen-remote" to serve on external-facing interfaces, eg to test on your phone. Use with caution.

#-------------------------------
# Facilities from ts that we ought to parallel here:
#../ts/src/input/JoyTwoState.ts
#../ts/src/input/JoyQuery.ts
#../ts/src/input/JoyLogical.ts
#../ts/src/input/FakePointer.ts
#../ts/src/input/Bus.ts
#../ts/src/input/FakeKeyboard.ts
#../ts/src/utility/TextService.ts
