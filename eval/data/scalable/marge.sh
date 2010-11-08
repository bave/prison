#!/bin/sh

#check1=$1/1check.log
#prison1=$1/1check-p.log

check2=$1/2check.log
prison2=$1/2check-p.log
#awk -F" " 'FILENAME == ARGV[1] {
#	split($1, tmp, ":")
#	sec = tmp[2]
#	split($2, tmp, ":")
#	msec = tmp[2] 
#	check[NR] = sec "." msec
#	lastnr = NR
#}
#FILENAME == ARGV[2] {
#	if( NR % 2 == 1) {
#		split($2, tmp, ":")
#		sec = tmp[2]
#		split($3, tmp, ":")
#		msec = tmp[2] 
#		line = (NR - 40 + 1) / 2
#		cache[line] = sec "." msec
#	}
#	else if( NR % 2 == 0) {
#		split($2, tmp, ":")
#		sec = tmp[2]
#		split($3, tmp, ":")
#		msec = tmp[2] 
#		line = (NR - 40) / 2
#		import[line] = sec "." msec
#	}
#}
#END {
#	print "check.sh set_cache import"
#	for(line = 1; line <= lastnr; line++) {
#		print check[line], cache[line], import[line]
#	}
#}' $check1 $prison1 > $1/1st_check.log

awk -F" " 'FILENAME == ARGV[1] {
	split($1, tmp, ":")
	sec = tmp[2]
	split($2, tmp, ":")
	msec = tmp[2] 
	check[NR] = sec "." msec
	lastnr = NR
}
FILENAME == ARGV[2] {
	split($2, tmp, ":")
	sec = tmp[2]
	split($3, tmp, ":")
	msec = tmp[2] 
	line = (NR - 40)
	cache[line] = sec "." msec
}
END {
	print "check.sh set_cache"
	for(line = 1; line <= lastnr; line++) {
		print check[line], cache[line], import[line]
	}
}' $check2 $prison2 > $1/2nd_check.log
