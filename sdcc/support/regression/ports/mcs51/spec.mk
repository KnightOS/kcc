# Port specification for the mcs51 port running with uCsim
#
# model small

# path to uCsim
S51 = $(SDCC_DIR)/sim/ucsim/s51.src/s51

SDCCFLAGS +=--lesspedantic -DREENTRANT=reentrant --stack-after-data

OBJEXT = .rel
EXEEXT = .ihx

EXTRAS = fwk/lib/testfwk$(OBJEXT) $(PORTS_DIR)/$(PORT)/support$(OBJEXT)

# Rule to link into .ihx
%$(EXEEXT): %$(OBJEXT) $(EXTRAS)
	$(SDCC) $(SDCCFLAGS) $(EXTRAS) $<
	mv fwk/lib/testfwk.ihx $@
	mv fwk/lib/testfwk.map $(@:.ihx=.map)

%$(OBJEXT): %.c
	$(SDCC) $(SDCCFLAGS) -c $<

# run simulator with 10 seconds timeout
%.out: %$(EXEEXT) fwk/lib/timeout
	mkdir -p `dirname $@`
	-fwk/lib/timeout 10 $(S51) -t32 -S in=/dev/null,out=$@ $< < $(PORTS_DIR)/mcs51/uCsim.cmd >/dev/null \
	  || echo -e --- FAIL: \"timeout, simulation killed\" in $(<:.ihx=.c)"\n"--- Summary: 1/1/1: timeout >> $@
	-grep -n FAIL $@ /dev/null || true

fwk/lib/timeout: fwk/lib/timeout.c

_clean:
	rm -f fwk/lib/timeout fwk/lib/timeout.exe $(PORTS_DIR)/$(PORT)/*.rel $(PORTS_DIR)/$(PORT)/*.rst \
	      $(PORTS_DIR)/$(PORT)/*.lst $(PORTS_DIR)/$(PORT)/*.sym $(PORTS_DIR)/$(PORT)/*.asm temp.lnk

