# RPVCC
Remote Pilotless Vehicle Command Center (Senior Project)
This project was assigned to Ryan Burila, Juan B. Sanchez, Mario Robles, and Joseph Nguyen by Dr. Tamer Omar
The goal of the project was to remotely control a Razor Dune Buggy using a VRX Simulator installed on Campus (CalPoly Pomona)
The Buggy had to be modified to be able to control with signals.
We used a TCP communication between the VRX Simulator(Client) and a Raspberry Pi(Server) on the buggy.
The Raspberry Pi was connected to the internet through a modem with a SIM card, it also sends the signals to each component of the vehicle (Steering, Braking, Acceleration)
