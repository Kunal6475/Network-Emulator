#####################################################
# The code developped by Kunal SinghaRoy for the course
#
# CNT 5505
# Spring 2016 Computer Science, FSU
#####################################################
1. Compiled on Linux machine. 

2. Commands you can run in stations/hosts. router and bridges

   send <destination> <message> // send message to a destination host
   route // show the routing table information (for station & router)
   arp // show the ARP cache table information (for station & router)
   interface// show the interface information (for station & router)
   table// show the bridge table information (for bridge)

3. To start the emulation, run

   emulation.sh

   which emulates the following network topology

   
          B              C                D
          |              |                |
         cs1-----R1------cs2------R2-----cs3
          |              |                |
          -------A--------                E

    cs1, cs2, and cs3 are bridges.
    R1 and R2 are routers.
    A to E are hosts/stations.
    Note that A is multi-homed, but it is not a router.

4. The maximum length of the message that can be send to another station is 75
   characters.

5. On closing the connected bridge the station or router displays the message
   the connected bridge has been closed and exits.If a station has not send
   any frames through bridge for more than 10 minutes that entry is removed
   from the bridge table.
