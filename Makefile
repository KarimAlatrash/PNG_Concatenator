# Makefile, ECE252  
# Karim Alatrash, 2021/10/04

CC = gcc 
CFLAGS = -Wall -std=gnu99 -g # "curl-config --cflags" output is empty  
LD = gcc
LDFLAGS = -std=c99 -g
LDLIBS = -lcurl -lz -pthread # "curl-config --libs" output 

SRCS   = paster.c crc.c curl_util.c png_util.c zutil.c l_list.c
OBJS1  = paster.o crc.o curl_util.o png_util.o zutil.o l_list.o
TARGETS= paster.out

all: ${TARGETS}

paster.out: $(OBJS1) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) *.png
