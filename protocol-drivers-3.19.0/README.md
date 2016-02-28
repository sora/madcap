## madcap test and evaluation.

### evaluation set-up memo

- At first, compile kernel modules with OVBENCH=yes option.
 - git clone https://github.com/upa/madcap.git
 - cd madcap
 - make OVBENCH=yes
  - 'make OVBENCH=yes' requires modified linux kernel (https://github.com/upa/linux-madcap-msmt). make-kpkg it, and replace the kernel of the host madcap running on.


- Setup raven and overlay protocols virtual devices.
 - cd protocol-drivers-3.19.0
 - ./reset.sh {0|1}
  - ./reset.sh 0 means madcap offload is disabled.
   - Packet TX path: L3 stack -> L2 stack -> dev_queue_xmit -> protocol driver -> L3 stack -> L2 stack -> dev_queue_xmit -> raven_xmit.
  - ./reset.sh 1 means madcap offload is enabled.
   - Packet TX path: L3 stack -> L2 stack -> dev_queue_xmit -> protocol driver -> madcap_queue_xmit -> dev_queue_xmit -> raven_xmit.


- Generate test packet and show timestamp values.
 - cd madcap/netdevgen
 - make && insmod netdevgen.ko
 - To generate test packet (OVBENCH: sk_buff timestamping), 'echo {noencap|ipip|gre|gretap|vxlan|nsh} > /proc/devier/netdevgen'.
 - Then, timestamp of the packet is displayed in /proc/driver/raven.
  - cat /proc/driver/raven
  - if './reset.sh 0', raven shows normal TX path progress time.
  - if './reset.sh 1', raven shows madcap offloaded TX path progress time.


