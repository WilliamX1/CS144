# CS144 Introduction to Computer Networking

`next`:

https://www.youtube.com/watch?v=KSTAsURGm5g&list=PL6RdenZrxrw9inR-IJv-erlOKRHjymxMN&index=43

分层结构，网络传输时从上至下依次调用，主机之间仅同层交流，同一主机内仅上下层交流。

- Application：http，smtp，ssh，ftp
- Transport：TCP，UDP，RTP
- Network：IP
- Link：Ethernet，WiFi，DSL，3G

![1](./README/1.png)

### IP Service Model

| Property | Behavior |
| :--: | :--: |
| Datagram | Individually routed packets. Hop-by-hop routing. |
| Unreliable | Packets might be dropped |
| Best effort | Only drop datagrams only if necessary |
| Connectionless | No per-flow state. Packets might be mis-sequenced |

1. Tries to prevent packets looping forever using `ttl`. If it exceeds 128 (default), then we think it might be stuck in a loop and just drop it.
2. Will fragment packets if they are too long because of the limit of link layer.
3. Uses a header checksum to reduce chances of delivering datagram to wrong destination.
4. Allows for new versions of IP, such as IPV6.
5. Allows for new options to be added to header.

![2](./README/2.png)

### TCP Service Model

![3](./README/3.png)

### UDP Service Model

![4](./README/4.png)

### 3-Way Handshake

1. Active opener sends first packet. (SYN with sequence number)
2. Passive opener responds. (SYN with sequence number, ACKs active opener's SYN packet)
3. Active opener responds. (ACKs passive opener's SYN packet)

**Connection Teardown**

- $A \rightarrow B$: FIN, seq $S_A$, ack $S_B$.
- $B \rightarrow A$: ack $S_{A + 1}$.
- $B \rightarrow A$: FIN, seq $S_B$, ack $S_{A + 1}$
- $A \rightarrow B$: ack $S_{B + 1}$.

