#!/bin/bash

for t in RR SJF PSJF FIFO ; do
    for i in {1..5} ; do
	sudo dmesg --clear
        echo "----------------------"$t"_$i----------------------------"
        sudo ./project_1 < "OS_PJ1_Test/"$t"_$i.txt" 1> "output/"$t"_"$i"_stdout.txt"
	dmesg | grep Project1 > "output/"$t"_"$i"_dmesg.txt"
    done
done
