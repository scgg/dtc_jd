#  +----------------------------------------------------------------------+
#  | Makefile for TPHPLIB                                                 |
#  +----------------------------------------------------------------------+
#  | Copyright (c) 2004-2008 Tencent Inc. All Rights Reserved.            |
#  +----------------------------------------------------------------------+
#  | Authors: The Club Dev Team, ISRD, Tencent Inc.                       |
#  |          fishchen <fishchen@tencent.com>                             |
#  +----------------------------------------------------------------------+

PHP_VER     = 5.4.0
PHP_MAIN_VER= php5.4


BIN_PATH  = ./
INCLUDE   = ./include
LIB       = ../../c_api
HOM_PHP   = $(INCLUDE)/php
LIBC_VER  = $(shell ls /lib/libc-*.so | cut -c11-13)

INC_SYS   = -I/usr/include 
LNK_SYS   = -rdynamic

INC_PHP   = -I$(INCLUDE)/ttcapi -I$(HOM_PHP) -I$(HOM_PHP)/main -I$(HOM_PHP)/TSRM -I$(HOM_PHP)/Zend
LNK_PHP   =

INC_TTC   = -I$(INCLUDE)/ttcpi
LNK_TTC   = -L$(LIB)/ -ldtc -Wl,-rpath=$(LIB)/

INC = $(INC_SYS) $(INC_PHP)  
INC+= $(INC_TTC)

LIB_SYS   = $(LNK_SYS)    $(LNK_PHP)
LIB_TTC   = $(LNK_TTC)    $(LIB_SYS)

CC    = g++
CFLAG = -fpic -DCOMPILE_DL=1 -Wall
LFLAG = -shared

HPP = tphplib.h
OOB = 

OBJ_TTC   = tphp_ttc.o tphp_ttc_server.o tphp_ttc_result.o tphp_ttc_request.o
TAR_TTC   = tphp_dtc.so.${PHP_MAIN_VER}


all: $(TAR_TTC)


$(TAR_TTC): $(OBJ_TTC)
	$(CC) $(LFLAG) -g -o $(TAR_TTC) $(OBJ_TTC) $(INC) $(LIB_TTC)
	@echo $@


%.o: %.cpp
	$(CC) $(CFLAG) -g -c $*.cpp $(INC)
	@echo $@

clean: 
	rm -f *.o
	rm -f *.so*
	@#rm -rf $(TAR)


