clean:
# The directories are still in CVS, so don't delete them
#       rm -rf $(MANUAL).html $(TSS).html
	rm -rf $(MANUAL).html/* $(TSS).html/* $(CDB).html
	rm -rf $(MANUAL).txt $(TSS).txt $(CDB).txt \
		*.pdf *.tex *.aux *.dvi *.idx *.ilg *.out\
		*.ind *.log *.toc *~ \#* *.ps */*.css */*.pl *.gif core *.glo
	rm -rf sdcc-doc sdcc-doc.tar.bz2

# Deleting all files created by configuring or building the program
# -----------------------------------------------------------------
distclean: clean

# Like clean but some files may still exist
# -----------------------------------------
mostlyclean: clean

# Deleting everything that can reconstructed by this Makefile. It deletes
# everything deleted by distclean plus files created by bison, etc.
# -----------------------------------------------------------------------
realclean: distclean
