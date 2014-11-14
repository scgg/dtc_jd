#this makefile is just for auto-build system's use

all:
	cd src; make C=0

clean:
	cd src; make C=0 clean
