#!/bin/bash
xgettext -k_ -d urlshortd -s -o urlshortd.pot *c *h
for l in `cat LINGUAS`; do 
	msginit --locale=$l --input=urlshortd.pot --output-file=$l.po
done

