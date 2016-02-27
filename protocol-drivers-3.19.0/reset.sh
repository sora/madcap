#!/bin/sh
# (re)set up modified protocol drivers

s=sudo
ip=../iproute2-4.4.0/ip/ip


# 1 is madcap offload, 0 is normal tx.
madcap=0

echo Unload modules.
$s rmmod ipip
$s rmmod ip_gre
$s rmmod gre
$s rmmod vxlan
$s rmmod nshkmod

$s rmmod raven
$s rmmod madcap

$s rmmod ip_tunnel
$s rmmod udp_tunnel


$s insmod ../madcap/madcap.ko
$s insmod ../raven/raven.ko drop_mode=1


echo set up normal raven device
if [ $madcap -eq 0 ]; then
	$s $ip link add name raven0 type raven
	$s ifconfig raven0 up
	$s ifconfig raven0 172.16.0.1/24
	sudo arp -s 172.16.0.2 7a:a3:28:27:a3:00

	madcap_enable="madcap_enable=0"
else
	madcap_enable="madcap_enable=1"
fi


echo set up ipip
$s modprobe ip_tunnel
$s modprobe tunnel4
$s insmod ipip/ipip.ko $madcap_enable

if [ $madcap -ne 0 ]; then
	$s $ip link add name r0-ipip type raven
	$s $ip madcap set dev r0-ipip offset 0 length 0 proto ipip
	$s $ip madcap add dev r0-ipip id 0 dst 10.10.10.10
	$s ifconfig r0-ipip up
	link="dev r0-ipip"
else
	link="dev raven0"
fi

$s ip tunnel add ipip1 mode ipip remote 172.16.0.2 local 172.16.0.1 $link
$s ifconfig ipip1 up
$s ifconfig ipip1 172.16.1.1/24



echo set up gre and gretap
$s modprobe gre
$s insmod gre/ip_gre.ko $madcap_enable

if [ $madcap -ne 0 ]; then
	$s $ip link add name r1-gre type raven
	$s $ip madcap set dev r1-gre offset 0 length 0 proto gre
	$s $ip madcap add dev r1-gre id 0 dst 10.10.10.10
	$s ifconfig r1-gre up
	link="dev r1-gre"
else
	link="dev raven0"
fi


$s ip tunnel add gre1 mode gre remote 172.16.0.2 local 172.16.0.1 $link
$s ifconfig gre1 up
$s ifconfig gre1 172.16.2.1/24

sudo ip link add type gretap local 172.16.0.1 remote 172.16.0.2 $link
sudo ifconfig gretap1 up
sudo ifconfig gretap1 172.16.3.1/24
sudo arp -s 172.16.3.2 7a:a3:28:27:a3:aa


echo setup vxlan
$s modprobe udp_tunnel
$s modprobe ip6_udp_tunnel
$s insmod vxlan/vxlan.ko $madcap_enable

if [ $madcap -ne 0 ]; then
	$s $ip link add name r2-vxlan type raven
	$s $ip madcap set dev r2-vxlan offset 8 length 48 proto udp
	$s $ip madcap set dev r2-vxlan udp enable dst-port 4789 src-port 4789 
	$s $ip madcap add dev r2-vxlan id 0 dst 10.10.10.10
	$s ifconfig r2-vxlan up
	link="dev r2-vxlan"
else
	link="dev raven0"
fi

$s $ip link add type vxlan local 172.16.0.1 remote 172.16.0.2 id 0 $link
$s ifconfig vxlan0 up
$s ifconfig vxlan0 172.16.4.1/24
$s arp -s 172.16.4.2 7a:a3:28:27:a3:a9


echo setup nsh
ni=nsh/iproute2-3.19.0/ip/ip
$s insmod nsh/nshkmod.ko $madcap_enable

if [ $madcap -ne 0 ]; then
	$s $ip link add name r3-nsh type raven
	$s $ip madcap set dev r3-nsh offset 12 length 32 proto udp
	$s $ip madcap set dev r3-nsh udp enable dst-port 4790 src-port 4790
	$s $ip madcap add dev r3-nsh id 0 dst 10.10.10.10
	$s ifconfig r3-nsh up
	link="dev r3-nsh"
else
	link="dev raven0"
fi

$s $ni link add type nsh spi 10 si 5
$s ifconfig nsh0 up
$s ifconfig nsh0 172.16.5.1/24
$s arp -s 172.16.5.2 7a:a3:28:27:a3:ab

$s $ni nsh add spi 10 si 5 encap vxlan remote 172.16.0.2 local 172.16.0.1 vni 0 $link
