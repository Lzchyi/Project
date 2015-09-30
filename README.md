# Project


Pang Yu Hau 1112703577

Lee Zhen Chyi 1112703477

Lecturer: Dr. Anang

Tasks

This project is mainly to create a file repository system using client-server TCP/IP model in c program. It is used for the user file storage.

We have followed the following requirements while developing.

IPC - Using PIPE and Message Queue on CLIENT side

Process Control - Using fork() and signals on CLIENT side

Socket - TCP on BOTH sides

Others - I/O Multiplexing on SERVER side


We also try to include the bonus part. For now, we only make the user (auth) login.

Work Distribution

Pang

    Do research
    Run the program
    Create the repository in Github
    Complete README file
    Do the finalization

Lee

    Do research
    Create and complete the code
    Run the program
    Check README file

Requirements:

Linux kernel 3.0+
GCC 4.8+ or Clang 3.5

Can install gcc for compiling c++ file via key in 

    sudo apt-get install g++

Compile:

for server:

    g++ server.cpp network_server.cpp util.cpp client_connection.cpp filesync_manager.cpp --std=c++11 -Wall -W -Wpedantic -o server


for client:

    g++ client.cpp network_client.cpp shell.cpp util.cpp --std=c++0x -Wall -W -Wpedantic -o client


Run:

    ./server <port>

Server uses working directory (/tmp/fileserver) for storing user files.
By default server listen 8000/tcp port. You can change it:

    ./server 8001


Client provides help menu, just type 'help' and follow commands.
By default client trying connect to server with IP 127.0.0.1 and 8000/tcp port.
To change it:

    ./client <address> <port>


Please key in 'auth' (after you type 'help' for menu) for login.

    username: root
    password: passwd123
