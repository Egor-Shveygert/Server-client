# Server-client
The goal of my project was to create a program to evaluate the given score (grade scale). At the beginning, the program expects a number in the range of 0 to 100 as input, any other input (negative value, letters, etc.) the program evaluates as an error and then shuts down. After inputting score from that range into the program, the program evaluates the input score and, as a result, returns to the user the grade assigned according to the scale (A = 100- 90, B = 90 - 80, etc.).
The assignment was handled through four clients and a server. The first two clients were connected via sockets to the server, and the server was tasked with forwarding the data entered from the user to the first client to the second client. Subsequent communication was implemented through pipes, which were used to forward variables that the other clients were working with.

The division of tasks between the server and the clients and implemented technologies:

- Server: connecting the 1st and 2nd client, and providing communication between them. (Implemented technologies: _**sockets, semaphore, threads**_)

- Client 1: Getting input data from the user. (Implemented technologies: sockets, timer, signal)

- Client 2: Checking the input to see if it is within the specified range. (Implemented technologies: **_sockets, process, pipes_**)

- Client 3: Evaluate the input, assign a grade to the specified points. (Implemented technologies: _**process, pipes**_)

- Client 4: Writing out the grade, to the user.  (Implemented technologies: **_process, pipes_**)

+ makefile and "gobal" semaphore

Starting the program:

Using the **_"make"_** program, on the Linux (Ubuntu) operating system, I created a makefile, which in the console, using the _**"make all"**_ argument, compiles the server and the clients, and then with the _**"make runs"**_ command I start the server, and with the _**"make runc"**_ command I start the clients. With the _**"make clear"**_ command, the executables created by us are cleared.
