## TCP/IP Networking

Operating Systems 2 - Lecture 5

-----

Obtain addresses via `ip` command:
```shell
ip address
ip a
```

Example:

```
[...]
2: wlp0s20f3: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether dc:21:5c:08:bf:83 brd ff:ff:ff:ff:ff:ff
    inet 192.168.178.28/24 brd 192.168.178.255 scope global dynamic noprefixroute wlp0s20f3
       valid_lft 863466sec preferred_lft 863466sec
    inet6 fe80::2ecb:209a:b7d2:bd77/64 scope link noprefixroute 
       valid_lft forever preferred_lft forever
[...]
```

Interface `wlp0s20f3` has MAC address `dc:21:5c:08:bf:83` and IP addresses `192.168.178.28` and `fe80::2ecb:209a:b7d2:bd77` assigned.

### Encapsulation

##### Setup

Start wireshark capture on used interface.
```shell
sudo systemd-resolve --flush-caches
```

Try to connect to a different host while having a Wireshark capture enabled:

```shell
telnet pw.edu.pl 80
```

ARP flush:

```shell
# Start wireshark capture on `docker0`
sudo ip neigh flush all
ip neigh
```

### Address conversions

```shell
gcc 02_addr.c -o /tmp/02_addr -g
/tmp/02_addr 127.0.0.1
```

```shell
/tmp/02_addr asdf
```

```shell
/tmp/02_addr 255.255.255.255
```

```shell
gcc 03_pton.c -o /tmp/03_pton -g
/tmp/03_pton 127.0.0.1
```

```shell
/tmp/03_pton 255.255.255.255
```

### DNS resolution

```shell
gcc 04_getaddrinfo.c -o /tmp/04_getaddrinfo -g
```

```shell
/tmp/04_getaddrinfo localhost
```

```shell
/tmp/04_getaddrinfo localhost 22
```

```shell
/tmp/04_getaddrinfo localhost ssh
```

Run under wireshark with `dns` filter:

```shell
/tmp/04_getaddrinfo google.pl
```

### UDP and socket addressing

```shell
gcc 05_udp.c -o /tmp/05_udp -g
/tmp/05_udp localhost 1234
```

```shell
gcc 06_udp_eph.c -o /tmp/06_udp_eph -g
/tmp/06_udp_eph
```

### TCP

```shell
gcc 07_tcp.c -o /tmp/07_tcp -g
/tmp/07_tcp localhost 1234
```

```shell
gcc 08_tcp_forked.c -o /tmp/08_tcp_forked -g
/tmp/08_tcp_forked localhost 1234
```
