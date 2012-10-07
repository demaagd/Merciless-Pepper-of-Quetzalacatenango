Overview
--------

Ever wanted a private/internal URL shortening service without all of the overhead of the framework-de-jour, then you're in luck - urlshortd was written to be a fast, simple, modular solution to passing around long URLs.  

It is written in fairly straight C, with as few uncommon dependancies as I could get away with.  

Compiling
---------

urlshortd is compiled via cmake (http://www.cmake.org/) and was initially written under Debian GNU/Linux.  

Simply check out the source code, cd <checkoutdir>/Release, and run ./b 

This will create the binary and database backends for you.  This is all that is really needed.  The templates are nice (but it has stubs that will work in many cases), but not required. Copy these files to where you wish to run them from, and execute.  For example:

./urlshortd -d sqlite:/tmp/db -n 10 -p 10000 -t ../templates/

Will tell it to use the SQLite back end (the only functional backend, MySQL and PostgreSQL are only stubs, would also like to do LevelDB as well), 10 threads, listening on port 10000 and use templates from ../templates (this is running straight from the Release dir uing the parent dir templates).  Woosh, Bob's your uncle.  

To Do
-----
- Database backends: MySQL, PostgreSQL, LevelDB, at least take a stab at them
- DB specific config help
- Some error case handling needs cleanup
- Change port spec to just let mongoose handle it (it looks to be able to handle more than a straight port-as-string


Copyright
---------

(C) Dave DeMaagd, demaagd@gmail.com or demaagd@spinynorm.org, 2012

For more information, see the LICENSE file.  

Includes code from mongoose, https://github.com/valenok/mongoose, used per terms listed in https://github.com/valenok/mongoose/blob/master/LICENSE as of 20121004 (see LICENSE-mongoose)

