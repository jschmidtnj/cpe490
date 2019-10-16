---
title: "CPE-490 Midterm"
author: Joshua Schmidt
geometry: margin=2cm
---

*I pledge my honor that I have abided by the Stevens Honor System.* - Joshua Schmidt, 10/16/2019

## Theory based questions

(answer the following in a paragraph or two, in less than 20 lines of text)
[10 points per questions]

1. Briefly explain the five layers of the Internet, including: Application, Transport, Network, Data-link and Physical. How does the simple act of “checking email” make use of these five layers?

The physical layer handles transmitting the raw bits over a physical connection and communication link. The data sent would be the raw bits that form the email itself, with all of the characters in the email (encoded in utf-8 for example) compressed and then converted to binary. Above the physical layer is the data link layer, which aggregates the bits into a stream of frames. The data sent in this layer would be a stream of frames, which are collections of the raw bits sent in the physical layer. The layer above is the network layer, which manages routing packets between different nodes in the network. The data type is a packet, each of which encapsulates a frame. Above is the transport layer, which communicates with the network layer and implements what is needed for transport (the bottom 3 layers). Data sent over the transport layer encapsulates the packet and is typically called a message. The application layer is the top layer, responsible for making the program visible to the end user work correctly. So in the case of an email, the email message would be queried through an api typically with an http get request, which would then go through all of the previous layers, to the server, and pop back up to the application on the server side. The response will then be sent over the same five layers until it gets back to the end-user with the email contents.

2. Which layers are “hop-by-hop” and which are “end-to-end”? What’s the difference between these two modes?

Hop-by-hop layers happen at every node in the network. These include physical and data-link layers. The end-to-end layers happen only at the start and end nodes in the network - the source and the destination. These include the application, transport, and network layers.

3. What are the major differences between TCP and UDP? Give an example application each that is best served by TCP and UDP.

The main difference between TCP and UDP is TCP guarantees the delivery of data to the destination router. UDP cannot guarantee this delivery. Additionally, TCP is connection-oriented, meaning the communicating devices establish a connection before they start to transmit data, and close the connection after they finish transmitting. UDP on the other hand does not do anything for starting, maintaining, and closing a connection. UDP is simpler and faster than TCP, TCP retransmits packets if they are lost, TCP has a much longer header than UDP (20-80 bytes vs 8 bytes), and UDP supports broadcasting whereas TCP does not. Live video and audio streaming would be best served with UDP because it is more important to have a continuous, live stream instead of a choppy, delayed high-quality stream. Web pages are better served with TCP because it is more important that the page comes in as one piece, with all of the formatting in-tact, so that the browser can properly render the html data and javascript.

4. How does TCP estimate round-trip-times (RTT) despite it being impossible to measure one way latency on the Internet? Expand on the idea of EWMA and it’s application to RTT estimation.

It is impossible to measure one way latency on the Internet because once the packet is sent and received at the destination, it takes time for the node at the destination to send a packet to the receiver saying that it received the original packet. To remedy this, TCP estimates the round-trip-times using an exponential weighted moving average (EWMA). When the TCP connection is established, the sender node sends a packet to the receiving node and starts a timer, which will time out when it reaches a timeout interval value. Each packet has a sequence number, and when the sender receives an `ACK` (acknowledgement) for a packet, it stops that specific timer (sample_rtt). The original estimated_rtt is equal to this first sample, but subsequent estimations are calculated with $estimated\_rtt = \alpha \cdot estimated\_rtt + (1 - \alpha) \cdot sample\_rtt$. The value of $\alpha$ can vary but we were using $0.8$.

5. How does TCP ensure reliable data delivery? Expand on the following TCP concepts: (i) Stop-and-wait, (ii) Go-back-N, and (iii) TCP-SACK.

TCP uses several methods to ensure the data is delivered. The first method is stop and wait, where the sender sends the packet and waits for an acknowledgement (`ACK`) for the packet. When the sender receives the `ACK`, it can then send the next packet. If the sender does not receive the `ACK`, it sends the same packet again. The second method is Go Back N. In this process, the sender sends N packets (N is the window size), and when these packets are sent, the sender waits for a cumulative acknowledgement before sending more packets. The receiver receives only in-order packets and ignores packets that are out of order. If there is any packet loss, the whole window is transmitted again. The last method is Selective Acknowledgements (TCP-SACK). In this process, the receiver acknowledges all packets received, regardless of if they were received in-order or not. For any packets not received, the sender will send those again. TCP also uses checksums to ensure that data is not corrupted in the packet itself. With these methods, TCP is able to ensure near-perfect data delivery.

6. Describe the AIMD congestion control algorithm employed by TCP. What is the specific need to increase “additively” and decrease “multiplicatively”?

The Additive Increase / Multiplicative Decrease (AIMD) congestion control algorithm is used to limit the amount of data being transferred at a given time, to leave bandwidth for others on the network, and allow for more bandwidth when the network has less congestion. To calculate the current "max window", or the amount of data that the sender can send without saturating the network, the sender takes the minimum of the advertised window - the window for the receiving side, and the congestion window - the window for the sending side. The receiving side provides the advertising window to the sender, and the sender can get the congestion window based on the amount of congestion it perceives in the network, increasing the window when congestion goes down and decreasing it when congestion goes up. The sender determines the congestion window based on timeouts, so whenever a packet is lost the congestion window halves (decreasing multiplicatively) and whenever the sender receives an `ACK`, the congestion window increases by one packet in size (increasing addatively). The decrease is multiplicative because the sender is more willing to reduce its congestion window than to increase it, since the consequences of having a window that is too large are much higher than of having a smaller window.

7. Describe the “three-way-handshake” used by TCP to establish a connection.

A three-way handshake is the algorithm used by TCP to create and end a connection, and like the name implies, involves exchanging three messages between two parties. For TCP, it is used so that the two participants can agree on the starting sequence numbers for their byte streams, but can really be used for agreeing upon any set of parameters for communication. In the first step, the client sends a message to the server stating the initial sequence number that it will be using (`SYN = a`). The server then responds with a segment that acknowledges it received the client's sequence number (`ACK = a + 1`) and adds it's own sequence number (`SYN = b`). Then the client responds with acknowledging the server's sequence number (`ACK = b + 1`). Timeouts are used to ensure that if the segments are transmitted and the acknowledgements are not received within a given amount of time, the sender will retry. The acknowledgement includes the increment of the given synchronization number because this implies the receiver understood the message.

1. What is the difference between medium access that is time-based (TDMA) and carrier-sense based (CSMA/CD)?

With Time Division Multiple Access (TDMA), every node in the network is set a specific amount of time to send and receive data. Everyone in the network is typically equal, with the high bandwidth nodes having the exact same amount of time to send and receive data as the nodes requiring little bandwidth. Carrier Sense Multiple Access with Collision Detection (CSMA/CD), on the other hand, takes a more active approach that allows for higher throughput. Every node in the network can listen to the link and determine if it is in use or is idle, and can check if two nodes are transmitting on the link simultaneously (collision detection). There are several algorithms to avoid collisions and find the right time for each node to transmit data. One example is the back-off algorithm, which is used in ethernet to reschedule transmissions after collisions. Both TDMA and CSMA/CD are focused on avoiding collisions and allowing each node in the network to have a turn. TDMA focuses on a simple time scheduling, while CSMA/CD focuses on measuring the amount of bandwidth that every node uses and creating a schedule based off of that.

## Plot this on a map

A traceroute to a remote destination in Osaka, Japan yielded the following router codes. Describe the cities that were traversed to reach Osaka from Hoboken, NJ. Do you think this is a direct path that a flight would take from here to there?

|                        |                                   |
|------------------------|-----------------------------------|
| mco.454a.lightpath.net | Orlando International Airport     |
| atl.as32112.level3.net | Atlanta International Airport     |
| stl.as39887.level3.net | St. Louis International Airport   |
| sat.as889.telegrid.net | San Antonio International Airport |
| pdx.as399.level3.net   | Portland International Airport    |
| Itm.7689.bigglobe.jp   | Osaka International Airport       |

You can often determine the cities that the packet passed through based off of the first letters of the router codes, which correspond to the nearest airport. So the packet started in Hoboken, then went to Orlando, then to Atlanta, then to St. Louis, then to Portland, and finally arrived in Osaka. This is not a direct path necessarily, as the packet stayed on the East Coast, backtracking to Atlanta from Orlando. But it is relatively direct, as the hops to St. Louis, San Antonio, and then Portland were heading in the right direction. A more direct path would be to go directly from Hoboken to Portland and then to Osaka, but the path given is not much worse.
