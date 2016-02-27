#!/bin/sh
# (re)set up modified protocol drivers

s=sudo
ip=../iproute2-4.4.0/ip/ip

echo Unload modules.
$s rmmod ipip
$s rmmod ip_gre
$s rmmod gre

$s rmmod raven
$s rmmod madcap

$s insmod ../madcap/madcap.ko
$s insmod ../raven/raven.ko


echo set up ipip
$s modprobe ip_tunnel
$s modprobe tunnel4
$s insmod ipip/ipip.ko

if [ $? -ne 0 ]; then
	echo failed, exit!
	exit
fi

$s $ip link add name r0-ipip type raven
$s $ip madcap set dev r0-ipip offset 0 length 0 proto ipip
$s $ip madcap add dev r0-ipip id 0 dst 10.10.10.10
$s ifconfig r0-ipip up

$s ip tunnel add ipip1 mode ipip remote 172.16.0.2 local 172.16.0.1 dev r0-ipip
$s ifconfig ipip1 up
$s ifconfig ipip1 172.16.1.1/24



echo set up gre and gretap
$s modprobe gre
$s insmod gre/ip_gre.ko

if [ $? -ne 0 ]; then
	echo failed, exit!
	exit
fi

$s $ip link add name r1-gre type raven
$s $ip madcap set dev r1-gre offset 0 length 0 proto gre
$s $ip madcap add dev r1-gre id 0 dst 10.10.10.10
$s ifconfig r1-gre up

$s ip tunnel add gre1 mode gre remote 172.16.0.2 local 172.16.0.1 dev r1-gre
$s ifconfig gre1 up
$s ifconfig gre1 172.16.2.1/24

sudo ip link add type gretap local 172.16.0.1 remote 172.16.0.2 dev r1-gre
sudo ifconfig gretap1 up
sudo ifconfig gretap1 172.16.3.1/24
sudo arp -s 172.16.3.2 7a:a3:28:27:a3:aa
