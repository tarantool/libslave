[![Build Status](https://travis-ci.org/tarantool/libslave.svg?branch=master)](https://travis-ci.org/tarantool/libslave)

ABOUT
===================================================================

This is a library that allows any arbitrary C++ application to connect
to a MySQL replication master and read/parse the replication binary
logs.

In effect, any application can now act like a MySQL replication slave,
without having to compile or link with any MySQL server code.

One important use-case for this library is for receiving changes in
the master database in real-time, without having the store the
master's data on the client server.

Features
-------------------------------------------------------------------
* Statistics of rps and time of every event in every table.
* Support for mysql options:
  * binlog_checksum=(NONE,CRC32)
  * binlog_row_image=(full,minimal)
  * GTID or log name and position positioning
* Column filter - you can receive only desired subset of fields from
a table in callback.
* Distinguish between absense of field and NULL field.
* Optional use `boost::variant` instead of `boost::any` for field
value storing.
* Store field values in `vector` by indexes instead of `std::map`
by names. Must be used in conjunction with column filter.

USAGE
===================================================================

Build requirements
-------------------------------------------------------------------

For building the library, you will need:

 * g++ with C++11 support.

 * The standard mysql C client libraries (libmysqlclient):
   * for 5.6-5.7 versions you will need place **hash.h** from mysql repo
     into your mysql include directory.

 * The headers of the boost libraries (http://www.boost.org).
   At the minimum, you will need at least the any.hpp.
   If boost_unit_test_framework is found, tests will be built.

 * You (likely) will need to review and edit the contents of `Logging.h`
   and `SlaveStats.h`
   These headers contain the compile-time configuration of the logging
   and monitoring subsystems.
   The provided defaults are sub-standard and an example only.

Ubuntu: cmake clang libboost-all-dev git libmysqlclient-dev

https://packages.ubuntu.com/xenial/amd64/libmysqlclient-dev/filelist
https://packages.ubuntu.com/bionic/amd64/libmysqlclient-dev/filelist
https://packages.ubuntu.com/focal/amd64/libmysqlclient-dev/filelist

Usage requirements
-------------------------------------------------------------------
 * Requires >= MySQL 5.1.23 and <= MySQL 5.7.12. Tested with some of the 5.1, 5.5, 5.6, 5.7
   versions of MySQL servers.
 * Requires rights `REPLICATION SLAVE` and `REPLICATION CLIENT`, and `SELECT` for tables being used.

Compiling
-------------------------------------------------------------------

Create directory "build" in source tree, step into it and run
"cmake .. -DCMAKE_BUILD_TYPE=Release".
Then, if configure step is complete, run "make".

Review and edit `Logging.h` and `SlaveStats.h` to interface the library to
your app's logging and monitoring subsystems.

You can type "make test" inside of "build" directory to run tests. You
will need a working mysql server for it. Settings of mysql connection
can be adjusted in `test/data/mysql.conf`. Type `ctest -V` if something
went wrong and you need see test output.

Using the library
-------------------------------------------------------------------

Please see the examples in 'test/'.

You can find the programmer's API documentation on our github wiki
pages, see https://github.com/vozbu/libslave/wiki/API.

Please read this article about real project libslave usage: http://habrahabr.ru/company/mailru/blog/219015/


CREDITS
===================================================================

(c) 2011, ZAO "Begun"

https://github.com/Begun/libslave

(c) 2016-2020, "Mail.Ru Group" LLC

https://github.com/vozbu/libslave

This library is licensed under the GNU LGPL. Please see the file LICENSE.
