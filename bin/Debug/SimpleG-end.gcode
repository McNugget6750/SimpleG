// These gcodes will be send to grbl in case the user decides to abort by pressing CTRL+C to get the laser into a safe state!
M5 S0
// My grbl seems to have a bug! G0 commands don't accept feedrates and they also sometimes don't shut off the laser. Use G1.
G1 X0 Y0 F3000
