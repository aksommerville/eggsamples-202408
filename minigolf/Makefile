# Yes, this is the whole build process!
# This can only work if the game is written in Javascript, since we're not calling out to any compilers.

all:
.SILENT:

ifeq (,$(EGG_SDK))
  EGG_SDK:=../../egg
endif
ifeq (,$(EGG_ARCH))
  EGG_ARCH:=linux
endif

SRCFILES:=$(shell find src -type f)
ROM:=out/minigolf.egg

# Here's the important part:
$(ROM):$(SRCFILES);echo "  $@" ; mkdir -p out ; $(EGG_SDK)/out/tool/eggrom -c -o$@ src

all:$(ROM)
run:$(ROM);$(EGG_SDK)/out/$(EGG_ARCH)/egg $(ROM)
clean:;rm -rf out

