ip tuntap del dev Tap-acc1 mode tap
ip tuntap add dev Tap-acc1 mode tap
ifconfig Tap-acc1 up
ip tuntap del dev Tap1 mode tap
ip tuntap add dev Tap1 mode tap
ifconfig Tap1 up
ip tuntap del dev Tap2 mode tap
ip tuntap add dev Tap2 mode tap
ifconfig Tap2 up
