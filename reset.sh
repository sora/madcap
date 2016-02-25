#!/bin/sh

# example setup and usage for test.

s=sudo
ip=./iproute2-4.4.0/ip/ip

echo - unload/load modules, create and setup raven interface 'r0' and 'r1'
$s rmmod raven
$s rmmod madcap

$s insmod madcap/madcap.ko
$s insmod raven/raven.ko

$s $ip link add name r0 type raven link eth0
$s $ip link add name r1 type raven link eth0

echo
$ip -d link show dev r0
$ip -d link show dev r1

echo
echo - config llt and add table entries
$s $ip madcap set dev r0 offset 10 length 10 proto udp
$s $ip madcap set dev r1 offset 11 length 11 proto ip

$s $ip madcap add id 11 dst 1.1.1.1 dev r0
$s $ip madcap add id 12 dst 2.2.2.2 dev r0

$s $ip madcap add id 21 dst 1.1.1.1 dev r1
$s $ip madcap add id 22 dst 2.2.2.2 dev r1

$s $ip madcap set dev r0 udp enable src-port hash dst-port 4790

echo
echo $ip madcap show
$ip madcap show

echo
echo $ip madcap show config
$ip madcap show config

echo
echo $ip madcap show udp
$ip madcap show udp
