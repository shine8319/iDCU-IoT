#ifndef SFSOURCE_H
#define SFSOURCE_H

int open_sf_source(const char *host, int port);
/* Returns: file descriptor for serial forwarder at host:port, or
     -1 for failure
     (see init_sf_source for description of platform handling)
 */

int init_sf_source(int fd);
/* Effects: Checks that fd is following the serial forwarder protocol.
     Use this if you obtain your file descriptor from some other source
     than open_sf_source (e.g., you're a server)
     Sends 'platform' for protocol version '!', and sets 'platform' to
     the received platform value.
   Modifies: platform
   Returns: 0 if it is, -1 otherwise
 */

void *read_sf_packet(int fd, int *len);
/* Effects: reads packet from serial forwarder on file descriptor fd
   Returns: the packet read (in newly allocated memory), and *len is
     set to the packet length
*/

int write_sf_packet(int fd, const void *packet, int len);
/* Effects: writes len byte packet to serial forwarder on file descriptor
     fd
   Returns: 0 if packet successfully written, -1 otherwise
*/


#endif
