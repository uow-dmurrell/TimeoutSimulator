lib_LTLIBRARIES = libflowmanager.la
include_HEADERS = libflowmanager.h tcp_reorder.h helpers.h

libflowmanager_la_SOURCES = libflowmanager.cc flowid.cc tcp_reorder.c helpers.cc claffy.cc rfctimeout.cc netramet.cc mbet.cc dots.cc pgat.cc hash.cc packetmatcher.cc flowexpiry.cc test.cc


INCLUDES = @ADD_INCLS@
libflowmanager_la_LIBADD = @ADD_LIBS@ @LTLIBOBJS@
libflowmanager_la_LDFLAGS = -version-info 2:0:0 @ADD_LDFLAGS@

