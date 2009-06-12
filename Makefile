CC=			gcc
CXX=		g++
CFLAGS=		-g -Wall -O2 #-m64 #-arch ppc
CXXFLAGS=	$(CFLAGS)
### For ncurses: please apply `-D_NO_CURSES' if you do not have ncurses installed
DFLAGS=		-D_IOLIB=2 -D_FILE_OFFSET_BITS=64 #-D_NO_CURSES
OBJS=		bam.o bam_import.o bam_pileup.o bam_lpileup.o bam_sort.o bam_index.o \
			razf.o bgzf.o faidx.o bam_tview.o bam_maqcns.o bam_aux.o bam_plcmd.o \
			bam_mate.o bam_rmdup.o glf.o bam_stat.o kstring.o bam_md.o sam.o sam_view.o \
			bam_rmdupse.o
PROG=		razip bgzip samtools
INCLUDES=	-Izlib
LIBS=		-lm -Lzlib -lz
SUBDIRS=	zlib . misc

.SUFFIXES:.c .o

.c.o:
		$(CC) -c $(CFLAGS) $(DFLAGS) $(INCLUDES) $< -o $@

all-recur lib-recur clean-recur cleanlocal-recur install-recur:
		@target=`echo $@ | sed s/-recur//`; \
		wdir=`pwd`; \
		list='$(SUBDIRS)'; for subdir in $$list; do \
			cd $$subdir; \
			$(MAKE) -f Makefile CC="$(CC)" CXX="$(CXX)" DFLAGS="$(DFLAGS)" CFLAGS="$(CFLAGS)" \
				INCLUDES="$(INCLUDES)" LIBS="$(LIBS)" $$target || exit 1; \
			cd $$wdir; \
		done;

all:$(PROG)

lib:libbam.a

libbam.a:$(OBJS)
		$(AR) -cru $@ $(OBJS)

### For ncurses: comment out `-lcurses' if you do not have ncurses installed
samtools:lib bamtk.o
		$(CC) $(CFLAGS) -o $@ bamtk.o $(LIBS) -L. -lbam -lcurses

razip:razip.o razf.o
		$(CC) $(CFLAGS) -o $@ razf.o razip.o $(LIBS)

bgzip:bgzip.o bgzf.o
		$(CC) $(CFLAGS) -o $@ bgzf.o bgzip.o $(LIBS)

razip.o:razf.h
bam.o:bam.h razf.h bam_endian.h kstring.h
sam.o:sam.h bam.h
bam_import.o:bam.h kseq.h khash.h razf.h
bam_pileup.o:bam.h razf.h ksort.h
bam_plcmd.o:bam.h faidx.h bam_maqcns.h glf.h
bam_index.o:bam.h khash.h ksort.h razf.h bam_endian.h
bam_lpileup.o:bam.h ksort.h
bam_tview.o:bam.h faidx.h bam_maqcns.h
bam_maqcns.o:bam.h ksort.h bam_maqcns.h
bam_sort.o:bam.h ksort.h razf.h
bam_md.o:bam.h faidx.h
razf.o:razf.h
glf.o:glf.h

faidx.o:faidx.h razf.h khash.h
faidx_main.o:faidx.h razf.h

cleanlocal:
		rm -fr gmon.out *.o a.out *.dSYM $(PROG) *~ *.a

clean:cleanlocal-recur
