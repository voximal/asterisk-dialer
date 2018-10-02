#
# GnuDialer - Complete, free predictive dialer
#
# Complete, free predictive dialer for contact centers.
#
# Copyright (C) 2005, GnuDialer Project
#
# Heath Schultz <heath1444@yahoo.com>
# Richard Lyman <richard@dynx.net>
#
# This program is free software, distributed under the terms of
# the GNU General Public License.
#

SHELL=/bin/sh

# Feel free to comment out the "guessed values" and enter the paths explicitly
# if you have troubles getting them from ./mkhelper or mysql_config.

RUNAS := `ps wwwaux | grep asterisk  | sed -e 's/ /\n/' | sed -e 's/ /\n/' | head -n 1`

MYSQLVER := `mysql -V | cut -c25-27`        

PREFIX=
URLPREFIX=

PACKAGE=voximal
YEAR := $(shell date +%Y)
COPYRIGHT:= ${PACKAGE}_${YEAR}

LMYSQLCLIENT=-lmysqlclient

CC=g++ -Wall -D PREFIX=\"${PREFIX}\" -D DOCROOT=\"${DOCROOT}\" -D URLPREFIX=\"${URLPREFIX}\" -D COPYRIGHT=\"${COPYRIGHT}\" 

all: dialer
	@echo Package :${PACKAGE} / ${COPYRIGHT} Dist ${DIST} 
	@echo
	@echo "Done! Now type \"make install\" to install."
	@echo
                                        
dialer: dialer.cpp *.h
	$(CC) dialer.cpp -g -o dialer `mysql_config --include` `mysql_config --libs` 
  
clean:
	rm dialer

install: theinstall

theinstall:
	cp dialer ${UB}
	mkdir -p ${PREFIX}/var/log/asterisk/dialer
	chmod a+rw ${PREFIX}/var/log/asterisk/dialer
	@echo
	@echo "Done!"
	@echo

uninstall: 
	rm ${UB}dialer         
