# Deleting all files created by building the program
# --------------------------------------------------
clean:
	rm -f *core *[%~] *.[oa] *.output
	rm -f .[a-z]*~
	rm -f $(PRJDIR)/bin/sdcc sdcc
	for port in $(PORTS) ; do\
	  $(MAKE) -C $$port -f clean.mk clean ;\
	done


# Deleting all files created by configuring or building the program
# -----------------------------------------------------------------
distclean: clean
	rm -f Makefile *.dep


# Like clean but some files may still exist
# -----------------------------------------
mostlyclean: clean


# Deleting everything that can reconstructed by this Makefile. It deletes
# everything deleted by distclean plus files created by bison, etc.
# -----------------------------------------------------------------------
realclean: distclean
	rm -f SDCCy.c
	rm -f SDCCy.h
	rm -f SDCClex.c
