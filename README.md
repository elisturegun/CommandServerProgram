The project is a command server program providing
communication and execution of multiple processes in a
Linux environment. It involves the creation of a command server and a client system. The server executes
the commands clients enter, and results are returned to
clients. The server can handle concurrent connections
from multiple clients. Each server-client interaction is
enclosed within a child process. A message queue is used
to establish connection. Afterwards, all connections between the client and the server are done through named
pipes.
Upon initiation, the server waits on a message queue
for upcoming client connection requests. Once a client
creates the message queue and the connection is established, if the client is in interactive mode, it asks the user
to enter commands as single commands or compound
commands. If the client is in batch mode, the commands
are read from an input file. The single commands cause
the creation of a runner child process that executes the
command. Compound commands lead to creating an unnamed pipe for communication between runner processes
executing each component of the compound commands.
The system supports termination mechanisms for
clients. Clients can seek termination by quit command in
non-batch mode. In batch mode, termination is done by
the end of the input text file. Furthermore, quitall command leads the server to terminate all child processes and
clean up resources afterward.
