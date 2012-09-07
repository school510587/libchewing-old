INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/internal \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/porting_layer/include

CC = $(CC_FOR_BUILD)
AM_CFLAGS = $(CFLAGS_FOR_BUILD)

noinst_PROGRAMS = sort_word sort_dic maketree

sort_word_SOURCES = sort_word.c \
		    phoneless_util.c

sort_word_LDADD = \
        $(top_builddir)/src/common/libcommon.la \
        $(top_builddir)/src/porting_layer/src/libporting_layer.la

sort_dic_SOURCES = sort_dic.c

sort_dic_LDADD = \
        -lm \
        $(top_builddir)/src/common/libcommon.la \
        $(top_builddir)/src/porting_layer/src/libporting_layer.la

maketree_SOURCES = maketree.c

