#
#  Make for omron power supply test program


MODULE		= app

CFLAGS		= -Wall -pedantic -g

IFLAGS		= /usr/local/include 
LIBS		= /usr/local/lib 

OBJECTS		= main.o 

.c.o:
		cc $(CFLAGS) -c -I$(IFLAGS) $<

all:		$(MODULE)

$(MODULE):	$(OBJECTS)
		cc $(CFLAGS) -o $(MODULE) $(OBJECTS) -L $(LIBS) 

clean:
		/bin/rm -f $(MODULE) *.core *.o

depend:
