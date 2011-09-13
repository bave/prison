#!/bin/sh

rm -f average.log
touch average.log
rm -f sd.log
touch sd.log

for i in `ls -1 p4d*/1st*`
do
	awk -F" " 'BEGIN {
		split(ARGV[1], str, "/")
		split(str[1], process, "p")
		split(process[2], node, "d")
		nodes = node[1] * node[2] * 10
		sum = 0
	}
	{
		if(NR == 1) {
			next
		}
		sum += $1
	}
	END {
		print nodes, (sum / (NR - 1)) * 1000
	}' $i > average.log

	awk -F" " 'BEGIN {
		sum = 0
		line = 0
	}
	FILENAME == ARGV[1] {
		fline = NR
		nodes = $1
		average = $2
	}
	FILENAME == ARGV[2] {
		line = NR - fline - 1
		if(line == 1) {
			next
		}
		data = $1 * 1000
		sum = (data - average) * (data - average)
	}
	END {
#		print line
#		print sum
		dispersion = sum / line
#		print dispersion
		sd = sqrt(dispersion) / 2
#		print sd
		print nodes, average, sd
	}' average.log $i >> sd.log
done

cat sd.log | sort -n > graph.data
#rm -f sd.log
#rm -f average.log

gnuplot Average_Name_Resolv_Time.plt
