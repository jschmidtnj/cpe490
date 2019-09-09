# Day 2

- arpanet is the precursor to the internet
- first step was to make the wires connected directly
- then there was a hub that was used to connect wires together manually
  - like a spider web going to the middle (the hub)
  - circuit switching
- all the intelligence in modern day networks are in the nodes - on the edges, as opposed to in the center (the hubs / core)
- the road belongs to everyone - not just one person
- 7 (main) layers - application -> presentation -> session -> transport -> network -> data link -> physical
- nested packets are the main data type (like a car or truck on a road network)
- reliability and speed are important characteristics of a network, and one is usually a tradeoff of the other
- "session" - the current user interacting with the application
- 3 big things that the computers do - software layers (first 3, found in any application like chrome), heart of OSI, hardware layers (last 3)
- transport layer is the heart - the bridge, determining what it needs for transport
  - has most of the logic for the network functionality
- transport layers talk to each other, and application layers talk (obviously)

## languages for the different layers

- http, snmp, smtp, profinet, mqtt
  - tcp, udp
    - ip
      -ethernet, 4g, 5g, etc
- most of the intelligence happens in your laptop
  - application and transport layers
- internet, link are the next two (major ones)
  - require much less intelligence
- `socket` connects one layer to another

## tcp

- tcp header has checksum, content, etc
- also contains time to live (ttl), source ip address, destination address
- https is secure + encrypted http, which uses tcp underneath
- *edge computing*

```bash
gedit & # creates a background process
kill -9 9353
ps -e # shows all processes running
pwd # show current directory
man ps # manual for commands
ping google.com # canonical shows nearest airport is laguardia
traceroute google.com # see route for packet to move
ls # stands for list
sudo su - # sudo stands for super user do
wireshark # see all packets that enter and leave your computer
ping -c10 google.com > asdf.txt # dump in a file
awk -f process.awk asdf.txt # print the data using awk
```
