# Port specification for the xa51 port running with uCsim

# path to uCsim
SXA_A = $(SDCC_DIR)/sim/ucsim/xa.src/sxa
SXA_B = $(SDCC_DIR)/bin/sxa

SXA = $(shell if [ -f $(SXA_A) ]; then echo $(SXA_A); else echo $(SXA_B); fi)

SDCCFLAGS +=-mxa51 --lesspedantic -DREENTRANT=

OBJEXT = .rel
EXEEXT = .hex

EXTRAS = $(PORTS_DIR)/$(PORT)/testfwk$(OBJEXT) $(PORTS_DIR)/$(PORT)/support$(OBJEXT)

# Rule to link into .hex
%$(EXEEXT): %$(OBJEXT) $(EXTRAS)
	$(SDCC) $(SDCCFLAGS) $(EXTRAS) $< -o $@
	mv fwk/lib/testfwk.hex $@
	mv fwk/lib/testfwk.map $(@:.hex=.map)

%$(OBJEXT): %.c
	$(SDCC) $(SDCCFLAGS) -c $< -o $@

$(PORTS_DIR)/$(PORT)/%$(OBJEXT): fwk/lib/%.c
	$(SDCC) $(SDCCFLAGS) -c $< -o $@

# run simulator with 1 second timeout
%.out: %$(EXEEXT) fwk/lib/timeout
	mkdir -p `dirname $@`
	-fwk/lib/timeout 1 $(SXA) -S in=/dev/null,out=$@ $< < $(PORTS_DIR)/xa51/uCsim.cmd >/dev/null || \
          echo -e --- FAIL: \"timeout, simulation killed\" in $(<:.ihx=.c)"\n"--- Summary: 1/1/1: timeout >> $@
	-grep -n FAIL $@ /dev/null || true

fwk/lib/timeout: fwk/lib/timeout.c

_clean:
	rm -f fwk/lib/timeout fwk/lib/timeout.exe $(PORTS_DIR)/$(PORT)/*.rel $(PORTS_DIR)/$(PORT)/*.rst \
	      $(PORTS_DIR)/$(PORT)/*.lst $(PORTS_DIR)/$(PORT)/*.sym $(PORTS_DIR)/$(PORT)/*.xa $(PORTS_DIR)/$(PORT)/*.lnk


