mncc_client
===========

OpenBSC mobile network call control client

1- Run OpenBSC:
	osmo-nitb -m
  	(-m --mncc-sock Disable built-in MNCC handler and offer socket)

2- Compile and run the control client
	gcc cc.c -o cc
	./cc

3- Make sure that OpenBSC see the client connecting
	<0006> mncc_sock.c:274 MNCC Socket has connection with external call control application





