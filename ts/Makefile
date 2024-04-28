all:
.SILENT:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

ifeq (,$(EGG_SDK))
  EGG_SDK:=../../egg
endif
ifeq (,$(EGG_ARCH))
  EGG_ARCH:=linux
endif

clean:;rm -rf mid out

DATAFILES:=$(shell find data -type f)
TSFILES:=$(shell find src -name '*.ts')

INDEXJS:=mid/js/index.js
ROM:=out/ts.egg
all:$(ROM)
$(ROM):$(INDEXJS) $(DATAFILES);$(PRECMD) $(EGG_SDK)/out/tool/eggrom -c -o$@ mid/js data
$(INDEXJS):$(TSFILES);$(PRECMD) tsc || ( rm $@ ; exit 1 )

run:$(ROM);$(EGG_SDK)/out/$(EGG_ARCH)/egg $(ROM)

serve:$(ROM);make --no-print-directory -C$(EGG_SDK) && $(EGG_SDK)/out/tool/server --port=8080 --htdocs=$(EGG_SDK)/src/web --rom=$(ROM) --listen-remote
# Add "--listen-remote" to serve on external-facing interfaces, eg to test on your phone. Use with caution.
