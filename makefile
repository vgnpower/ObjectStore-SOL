# ---------------------------------------------------------------------------
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2 as 
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#  As a special exception, you may use this file as part of a free software
#  library without restriction.  Specifically, if other files instantiate
#  templates or use macros or inline functions from this file, or you compile
#  this file and link it with other files to produce an executable, this
#  file does not by itself cause the resulting executable to be covered by
#  the GNU General Public License.  This exception does not however
#  invalidate any other reasons why the executable file might be covered by
#  the GNU General Public License.
#
# ---------------------------------------------------------------------------

CC			=  gcc
AR          =  ar
CFLAGS	    += -std=c99 -Wall
ARFLAGS     =  rvs
INCLUDES	= -I.
LDFLAGS 	= -L.
OPTFLAGS	= -g -O3 
LIBS        = -pthread -lm

# aggiungere qui altri targets
TARGETS		= objstore_server	 \
			  objstore_client

.PHONY: all clean cleanall test
.SUFFIXES: .c .h

%: %.c
	$(CC) $(CCFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

all		: $(TARGETS)

libUtils.a: utils.o utils.h
	$(AR) $(ARFLAGS) $@ $<

libAccess.a: access.o access.h libUtils.a
	$(AR) $(ARFLAGS) $@ $<

objstore_client: objstore_client.o libAccess.a  libUtils.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

objstore_server: objstore_server.o libUtils.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -R -f *.o *.a *.log data .tmp
test		:
	> testout.log
	bash looptest.sh
	bash testsum.sh