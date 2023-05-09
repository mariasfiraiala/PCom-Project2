Copyright 2023 Maria Sfiraiala (maria.sfiraiala@stud.acs.upb.ro)

# Subscribe TCP Protocol - Project2

## Description

### Context

The project aims to implement a simple subscribe protocol between a server and several clients.
The clients (or subscribers) are identified by an unique id, decided at their first connection and **can get back and forth between connecting and disconnecting from the platform**.
For a certain flag specifically set in the subscription request, the message sent to the client while disconnected, **gets stored in a server map** and forwarded to the user when it gets back online.

The client decides to what topic wishes to get subscribed and whether the store and forward functionality makes sense for them and sents a request.
**The server gets messages from other UDP clients**, containing bits of information regarding various topics and **forwards these packets directly to the interested TCP clients (if online)**.

### Project Flow

The main part of the project logic is found in the **server implementation**:

* The server starts 2 sockets:

   * a passive one, used for getting TCP connections: `listenfd`

   * a socket used for getting messages from the UDP clients: `udp_cli_fd`

* It performs I/O multiplexing using `poll()`:

   * The first 3 fds watched are: the one for TCP connections, the one for UDP connections and stdin, in case the server gets the exit command

   * If `listenfd` gets triggered, a new TCP client emerged: we accept its wish to connect, we store its credentials (IP and port) in order to print them when the connection officially begins and we add it to the list of poll fds.

   * If `udp_cli_fd` gets triggered, a new message from the UDP client was sent: we receive the information, we parse it in a more meaningful manner (in a structure, `stored_message_t`), as it needs to get sent to the TCP subscribers, and we iterate through the list of subscribed clients to check which one of them is either online, or wants the message stored until it gets connected again.

   > **Note**: The `stored_message_t` instance simulates the `std::shared_ptr` behaviour: only one instance of the onbject exists and it's shared between all subscribed and disconnected clients.
   The rationale behind simulating `std::shared_ptr` and not using it directly: it's easier to manage (I still do the memory management) and I get to avoid weird C++ errors.

   * If `stdin` gets triggered, we parse the command and we close all the fds from the poll struct if the server wants to exit.

   * Otherwise, we have some information sent from a TCP subscriber. The command is one from the list below:

      1. **CONNECT**: Right after establishing a connection with the server, the default package that the subscriber sends is one of this type. It means inserting the client in the id database (if it weren't already there) and sending back lost messages.

      1. **SUBSCRIBE**: Create the subscription topic if it doesn't exist, add the client to it, and mark the SF flag.

      1. **UNSUBSCRIBE**: Delete the client from the list of subscribers to the topic, and remove the topic from the client list.

      1. **EXIT**: Mark the client as offline, close its fd and remove the fd entry from the poll fd array.

The **subscriber implementation** is based off:

* starting a connection and sending a packet mirroring this.

* I/O multiplexing:

    * the subscriber reads from `stdin` commands: subscribe, unsubscribe, exit

    * the subscriber gets info from the server, interprets it and pretty prints it to `stdout`

### Data Structures

In order to keep the credentials, the ids and the topics ordered and easy to query, we've used 3 maps:

* `ids`:

   &rarr; the key: the string by which a user is identified

   &rarr; the value: all the other information regarding the subscriber (its fd, state of connectivity, a map with topics and the flag associated with them, and lost messages while offline)

   &rarr; used for: ids conflicts

* `topics`:

   &rarr; the key: the string representing the topic name

   &rarr; the value: array of users subscribed to the topic

   &rarr; used for: forwarding messages received from UDP clients

* `ips_ports`:

   &rarr; the key: fd

   &rarr; the value: pair consisting of an ip and port associated with the fd subscriber

   &rarr; used for: printing credentials when a client gets connected

> **Note**: I've used C++ for the implementation of this project specifically for the STL data structures.
No other reason!
This is exactly why I haven't dropped `printf()` and `scanf()` and the reason for the project looking so C like.

### Hierarchy

```console
.
├── common.cpp
├── common.h
├── Makefile
├── parser.cpp
├── parser.h
├── README.md
├── server.cpp
├── subscriber.cpp
└── utils.h
```

* `common.cpp`, `common.h`: `recv_all()`, `send_all()` implementations, because TCP fragments the data it sends and receives

* `parser.cpp`, `parser.h`: functions used for parsing `stdin` data and messages sent from the server

## Observations Regarding the Project

I really enjoyed this one!
For a long time, I've been waiting to have a project that would deep dive me into the socket API and communication across different processes and this one did it for me.

A fairly entertaining, well explained and easy project, though I would have liked it to also be posted to the same [pcom.pages](https://pcom.pages.upb.ro/tema1/) interface as the first and third projects.
