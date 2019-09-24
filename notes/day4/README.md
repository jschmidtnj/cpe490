# data

- live content (soccer) vs. elastic content (netflix)
- tcp = reliable delivery, segmentation and reassembly, highly decentralized
- udp = unreliable delivery, connectionless, no congestion control
- all browsing is tcp
  - host sends a SYN message (syncronize), destination receives it, gets x+1 to send back. this is a 3 way handshake
  - connection is live after handshake as long as you are on site
  - ACK = acknowledge, for saying that yes, you did get something (saying hello)

## time

- paradox of time
- cannot measure time accurately because if one server had the time, everyone couldn't get the time, etc.

### how ping works

$RTT _{new} = (1- \alpha) \cdot RTT _{old} + \alpha \cdot RTT _{current}$

### SACK [Selective Acknowledgments]

this allows for the server to send one packet and if it fails to keep sending the other packets and say that the first one didn't work, instead of keep trying to send the first packet

### AIMD

- solved internet congestion problems, with some packets hogging all the bandwidth

### TCP

- tcp doesn't have any sense of time - no time stamps, etc
- ACK numbers are in the tcp header

### udp

- has source port, destination port, length, and checksum
- has very little reliability
