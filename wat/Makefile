all:
.SILENT:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

ifeq (,$(EGG_SDK))
  EGG_SDK:=../../egg
endif
ifeq (,$(EGG_ARCH))
  EGG_ARCH:=linux
endif
ifeq (,$(WABT_SDK))
  WABT_SDK:=../../thirdparty/wabt
endif

clean:;rm -rf mid out

DATAFILES:=$(shell find data -type f)
WATFILES:=$(shell find src -name '*.wat')

WASM1:=mid/wasm/1
ROM:=out/wat.egg
all:$(ROM)
$(ROM):$(WASM1) $(DATAFILES);$(PRECMD) $(EGG_SDK)/out/tool/eggrom -c -o$@ mid/wasm data
$(WASM1):$(WATFILES);$(PRECMD) $(WABT_SDK)/build/wat2wasm -o $@ $^

run:$(ROM);$(EGG_SDK)/out/$(EGG_ARCH)/egg $(ROM)

serve:$(ROM);make --no-print-directory -C$(EGG_SDK) && $(EGG_SDK)/out/tool/server --port=8080 --htdocs=$(EGG_SDK)/src/web --rom=$(ROM)
# Add "--listen-remote" to serve on external-facing interfaces, eg to test on your phone. Use with caution.
