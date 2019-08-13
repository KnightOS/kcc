peeph-upstream.def:
	curl https://svn.code.sf.net/p/sdcc/code/trunk/sdcc/src/z80/peeph-z80.def > $@
	
peeph.rul: peeph-upstream.def
	sed -e 's/\(%*[[:digit:]]\+\) *(\(i[xy]\))/(\2 + \1)/g' $< > peeph.tmp
	#remove jr-related optimizations
	sed peeph.tmp -i -e 's@// Ensure that all rules above see only jp.*@FILE_END@'
	awk -f ../SDCCpeeph.awk peeph.tmp > $@
	sed -i $@ -e 's@.*FILE_END.*@/*@'
	echo "*/" >>$@
	rm peeph.tmp
