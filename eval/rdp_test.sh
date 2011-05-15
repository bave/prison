#!/bin/sh

# 1. bin/cage を立ち上げてからこのスクリプトを実行
#  - node1, node2 を作成
#  - node2 が node1 へ join
#  - node1 が rdp_listen を作成
#  - node2 が rdp_connect を作成
#  - node1 が /bin/unでlisten ソケットへコネクト
# 2. bin/un /tmp/rdp_connect で node2 へコネクト
#  - メッセージを送ってみる


# bin/un named socket のインタラクションツール
#  - FLOW, HANDLER, MESSAGE, のエスケープがある cage/header.hppを参照
#  - FLOW
#    - FLOW: {listen,connect} ->  {Top2Bottom,Bottom2Top}
#    - MSSAGE: CLOSED,ACCEPT,CONNECT,,,etc
#    - HANDLER: int の整数   

expect -c "
set timeout 2
spawn ../bin/cli
expect \"send_message:\"
send \"new,node1,12001,global\n\"
expect \"send_message:\"
send \"new,node2,12002,global\n\"
expect \"send_message:\"
send \"join,node2,localhost,12001\n\"
expect \"send_message:\"
send \"get_id,node1\n\"
expect \"send_message:\"
set i [split \$expect_out(buffer) \"\n\"]
set j [lindex \$i 1]
set k [split \$j \",\"]
set l [lindex \$k 3]
set m [string trim \$l]
send \"rdp_listen,node1,/tmp/rdp_listen,1000\n\"
expect \"send_message:\"
send \"rdp_connect,node2,/tmp/rdp_connect,1000,\$m\n\"
#send \"rdp_connect,node2,/tmp/rdp_connect,1000,1a7d7677b099214ded676c8e677b20521645faebn\n\"
expect \"send_message:\"
send \"quit\n\"
expect
spawn ../bin/un /tmp/rdp_listen
interact
"
