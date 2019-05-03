CREATE DATABASE IF NOT EXISTS dialer;
GRANT ALL ON dialer.* TO 'dialer'@'localhost' IDENTIFIED BY 'dialeri6net';
GRANT FILE ON *.* TO 'dialer'@'localhost';
SET GLOBAL local_infile = 1;
FLUSH PRIVILEGES;


