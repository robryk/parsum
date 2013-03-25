#!/usr/bin/sed -nf

/^$/ { # print on empty line
	x
	s/^\n//
	s/\n/;/g
	p
}

# Remove label
:b
s/^[0-9.]*[^0-9.]//
tb

# Paste 
H

