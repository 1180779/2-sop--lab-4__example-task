# Chat server solution

## Task
### Overview
The goal of this task is to write a chat server compatible with provided client application. All 
messages sent between cliend and server have always a lenght of 512 bytes. First 64 bytes are 
indented for client name (username), the rest (448 bytes) contain the messag text or in case of 
authentication message a key. 

### Stages
1. Implement connection of one client to the TCP server. Server takes port number and authentication 
key as input arguments. Sample server use: ``./sop-chat 9000 key``
After starting the server listens for clients. When connection with first client is established server 
closes it and immediately ends. 
2. Implement authenthication mechanism. After establishing the connection server reads autherntication 
message, displays user name and provided key. If the key matches the server key, then server sends back 
the message and in loop reads 5 consecutive messages and displays them in console. After 5 messages 
the connection is closed. If the keys do not match, the connection is closed immediatelly. 
3. Implement multi-client support. Implement multi-client support. Customer messages are now read 
without limits quantification. Once a new client joins, it is added to the list of current clients.
The maximum number of clients is 4. If the maximum number of clients joins, discard
new connections. When the server receives messages from any client, it displays them on the console
and forwards it to other clients. To implement this step, use the epoll function (or alternatively
pselect, or ppoll).
4. After receiving a SIGINT signal, the server closes all connections, releases resources, and terminates
work. Add correct client disconnection handling in the following cases:
    * Reading from a given client's descriptor returns a message of length 0
    * Writing to a given client's descriptor throws an EPIPE error
    * Writing or reading throws an ECONNRESET error
    * The server received C-c

### Author
The author of the task content is [@tomasz-herman](https://github.com/tomasz-herman)

## Completion
1. [X] stage 1
2. [X] stage 2
3. [X] stage 3
4. [ ] stage 4
    * [X] message of lenght 0
    * [ ] EPIPE
    * [ ] ECONNRESET
    * [ ] C-c
