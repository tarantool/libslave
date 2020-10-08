#!/bin/sh

set -eux

sudo cp $TRAVIS_BUILD_DIR/test/data/travis.cnf /etc/mysql/conf.d/
sudo service mysql restart
mysql -u root -e "CREATE USER tarantool IDENTIFIED BY 'tarantool';"
mysql -u root -e "GRANT ALL ON *.* TO tarantool;"
mysql -u root -e "CREATE USER 'tarantool'@'localhost' IDENTIFIED BY 'tarantool';"
mysql -u root -e "GRANT ALL ON *.* TO tarantool@localhost;"
mysql -u root -e "GRANT ALL PRIVILEGES ON *.* TO 'root'@'localhost' WITH GRANT OPTION;"
mysql -u root -e "GRANT SELECT ON *.* TO 'tarantool'@'localhost';"
mysql -u root -e "GRANT REPLICATION CLIENT ON *.* TO tarantool@localhost;"
mysql -u root -e "GRANT REPLICATION SLAVE ON *.* TO tarantool@localhost;"
mysql -u root -e "FLUSH PRIVILEGES;"
mysql -e "CREATE DATABASE IF NOT EXISTS tarantool_mysql_test;"
mysql -e "SHOW DATABASES;"
mysql -e "SHOW VARIABLES LIKE '%bin%';"
mysql -e "SHOW BINARY LOGS;"
