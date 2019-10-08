# data link layer

- scheduling which node goes first and next, etc. don't want data to get garbled
- like a traffic signal
- channels are always "broadcast" - everyone can send and hear at the same time
- started with a random delay thrown in to retry the send (`aloha`, `slotted aloha` into time slots)
  - only got about 40% yield
- next got `TDMA` - time division multiple access. every node gets a specific amount of time
  - creates a line of stuff. everyone gets 5 seconds, round and round
- ethernet `token ring` algorithm. there's a token floating around the network and when the computer gets the token it is free to send data
- divide with time and frequency for higher throughput
- CSMA/CD - carrier sense multiple access with collision detection - makes sure wifi stuff works
- 802.11 = wifi, uses csma/cd
  - csma/cd transmits data, fixes collisions, etc
- mtu - max transportational unit = 1024 bytes (max packet size)

## midterm

- lots of theoretical questions
- Saturday
- chapters 3, 4, 5
- test Oct 15th, 24 hrs to finish
