#!/bin/sh

set -exu

MYSQL_USER="tarantool"
MYSQL_PASSWORD="tarantool"
MYSQL_DB="tarantool_mysql_test"
MYSQL_TABLE="test"

DB_FILLER_OPT="-h 127.0.0.1 -P 3306 -u $MYSQL_USER -p $MYSQL_PASSWORD -d $MYSQL_DB -s $MYSQL_TABLE -l $MYSQL_TABLE -c 10 -b 64"
TEST_CLIENT_OPT="-h 127.0.0.1 -P 3306 -u $MYSQL_USER -p $MYSQL_PASSWORD -d $MYSQL_DB -m"

./build/test/db_filler $DB_FILLER_OPT
# TODO: learn how to run test (GH-27)
# ./build/test/test_client $TEST_CLIENT_OPT
