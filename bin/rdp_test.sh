#!/bin/sh

# 1. cage を立ち上げてからこのスクリプトを実行
# 2. un /tmp/rdp_connect でクライアント側の
# あとは、クライアント側でメッセージを送ってみる．

expect -c "
set timeout 2
spawn ./cli
expect \"send_message:\"
send \"new,kris1,12001,global\n\"
expect \"send_message:\"
send \"new,kris2,12002,global\n\"
expect \"send_message:\"
send \"join,kris2,localhost,12001\n\"
expect \"send_message:\"
send \"get_id,kris1\n\"
expect \"send_message:\"
set i [split \$expect_out(buffer) \"\n\"]
set j [lindex \$i 1]
set k [split \$j \",\"]
set l [lindex \$k 3]
set m [string trim \$l]
send \"rdp_listen,kris1,/tmp/rdp_listen,1000\n\"
expect \"send_message:\"
send \"rdp_connect,kris2,/tmp/rdp_connect,1000,\$m\n\"
#send \"rdp_connect,kris2,/tmp/rdp_connect,1000,1a7d7677b099214ded676c8e677b20521645faebn\n\"
expect \"send_message:\"
send \"quit\n\"
expect
spawn ./un /tmp/rdp_listen
interact
"
