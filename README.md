# Command Server Program

This project is a Linux-based command server program that facilitates communication and execution of multiple processes. It implements a server-client architecture where the server executes commands received from clients and returns results. The server supports concurrent connections and utilizes inter-process communication mechanisms such as message queues and named pipes.

---

## Features

- **Server-Client Architecture**: The server communicates with clients through message queues and named pipes.
- **Concurrent Connections**: Handles multiple client connections simultaneously using child processes.
- **Command Execution**: 
  - Executes single commands using runner child processes.
  - Handles compound commands using unnamed pipes for communication between runner processes.
- **Interactive and Batch Modes**:
  - **Interactive Mode**: Clients can manually enter single or compound commands.
  - **Batch Mode**: Clients can execute commands from an input file.
- **Termination Mechanisms**:
  - Clients can terminate with a `quit` command in interactive mode or at the end of the input file in batch mode.
  - A `quitall` command terminates all child processes and cleans up server resources.

---

## System Architecture

1. **Server**:
   - Listens for incoming client connection requests on a message queue.
   - Spawns child processes for handling commands and communicating with clients.
   - Uses named pipes for all subsequent communication after the initial connection.
2. **Client**:
   - Connects to the server through a message queue.
   - Operates in either:
     - **Interactive Mode**: Users enter commands one by one.
     - **Batch Mode**: Commands are read from a file.

---

## Installation and Setup

### Prerequisites

- Linux operating system.
- GCC compiler for compiling C programs.
- Basic knowledge of inter-process communication (IPC) in Linux.

### Steps

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/<your-username>/command-server.git
   cd command-server
   
2. **Compile the Program: Use the Makefile:**
    ```bash
    make
   
3. **Run the Server: Start the server to listen for incoming client connections:**
    ```bash
    ./server
   
5. **Run the Client: Launch the client in either interactive or batch mode:**

    **Interactive Mode:**
    ```bash
    ./client
    **Batch Mode:**
    ```bash
    ./client <input_file>
    
### Usage
  **Interactive Mode:**

    Enter commands directly in the client terminal.
    Use quit to terminate the client session.
    
  **Batch Mode:**
    Provide a file containing commands as input to the client.
    Termination occurs automatically at the end of the input file.
  
  **Compound Commands:**
    Use | to chain commands (e.g., ls | grep .txt).
    The server creates unnamed pipes to handle such commands.

  **Server Termination:**
    Use the quitall command to terminate all client connections and clean up resources.
