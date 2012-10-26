#!/bin/bash
xgettext -k_ -d urlshortd -s -o urlshortd.pot *c *h
for l in `cat LINGUAS`; do 
	msginit --no-wrap --locale=$l --input=urlshortd.pot --output-file=po/$l.po
done

