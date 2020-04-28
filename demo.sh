#!/bin/bash

sudo dmesg --clear
echo "----------------------TIME_MEASUREMENT.txt---------------------------"
sudo ./project_1 < "OS_PJ1_Test/TIME_MEASUREMENT.txt" 
dmesg | grep Project1
printf "\n" 

sudo dmesg --clear
echo "----------------------FIFO_1.txt---------------------------"
sudo ./project_1 < "OS_PJ1_Test/FIFO_1.txt" 
dmesg | grep Project1
printf "\n" 

sudo dmesg --clear
echo "----------------------PSJF_2.txt---------------------------"
sudo ./project_1 < "OS_PJ1_Test/PSJF_2.txt" 
dmesg | grep Project1
printf "\n" 

sudo dmesg --clear
echo "----------------------RR_3.txt---------------------------"
sudo ./project_1 < "OS_PJ1_Test/RR_3.txt" 
dmesg | grep Project1
printf "\n" 

sudo dmesg --clear
echo "----------------------SJF_4.txt---------------------------"
sudo ./project_1 < "OS_PJ1_Test/SJF_4.txt" 
dmesg | grep Project1
printf "\n" 
