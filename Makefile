# eggsamples Makefile
# Replace EGG_SDK, and EGG_ARCH if needed per your build environment.
# Output Egg files will appear in each game's "out" directory.
# Usage:
#   make             # Build everything.
#   make clean       # Remove all outputs.
#   make run-GAME    # Build and launch one game.
#   make build-GAME  # Build one game.

all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;
force:

export EGG_SDK:=$(realpath ../egg)
export EGG_ARCH:=$(firstword $(filter linux mswin macos,$(notdir $(wildcard $(EGG_SDK)/out/*))))

#------------------------------------------------
# Everything in the repo's root directory (where this Makefile lives) must be named exactly once here.

GAMES_MAKE:=lightson shmup hardboiled arrautza gravedigger
GAMES_IGNORE:=Makefile README.md playable

#------------------------------------------------

GAMES_ACTUAL:=$(sort $(wildcard *))
GAMES_NAMED:=$(sort $(GAMES_MAKE) $(GAMES_IGNORE))
ifneq ($(GAMES_ACTUAL),$(GAMES_NAMED))
  GAMES_MISSING:=$(strip $(filter-out $(GAMES_ACTUAL),$(GAMES_NAMED)))
  ifneq (,$(GAMES_MISSING))
    $(error Some games are named in Makefile but not found: $(GAMES_MISSING))
  endif
  GAMES_UNKNOWN:=$(strip $(filter-out $(GAMES_NAMED),$(GAMES_ACTUAL)))
  ifneq (,$(GAMES_UNKNOWN))
    $(error Some games exist but are not listed in Makefile: $(GAMES_UNKNOWN))
  endif
endif

all:$(addprefix build-,$(GAMES_MAKE))
build-%:force;make --no-print-directory -C$*
clean:;$(foreach G,$(GAMES_MAKE),make --no-print-directory -C$G clean ; )
$(addprefix run-,$(GAMES_MAKE)):;make --no-print-directory -C$(subst run-,,$@) run

# Copy HTML builds into playable -- these will be committed like input files.
playables:all;cp $(foreach G,$(GAMES_MAKE),$G/out/$G.html) playable
