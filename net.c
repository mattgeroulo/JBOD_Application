#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */

int cli_sd = -1;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {
  
  int n=0; 
  int x=0;
  while (n<len){
    x = read(fd,buf+n,len-n);
    if (x<0){
      return false;
    }
    n+= x;
    
  }
 
  return true;
  
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
  
  int n = 0;
  int x =0;
  while (n<len){
    x = write(fd,buf+n,len-n);
    if (x<0){
      return false;
    }
    n+= x;
  }
  
  return true;
  
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
  uint16_t len;
  uint8_t header[HEADER_LEN];
	if (nread(fd, HEADER_LEN, header) == false){
		return false;
	}

	// update the desired bit values
	  memcpy(&len, header, 2);
	  memcpy(op, header+ 2, 4);
	  memcpy(ret, header + 6, 2);
    len = htons(len);
    *op=htonl(*op);
    *ret = htons(*ret);

	// finds if length of packet is  == header +256
	if (len == (HEADER_LEN + JBOD_BLOCK_SIZE)){
		if (nread(fd, JBOD_BLOCK_SIZE, block) == true) {
			return true;
		}
		
		else{
			return false;
		}
		
	}
	return true;
}




/* attempts to send a packet to sd; returns true on success and false on
 * failure */
// helper function to make packet, similar to recv_packet
uint8_t * createpacket(uint16_t len, uint16_t ret_code,  uint32_t opcode,uint8_t *block){
  int packet_size = len;
	uint8_t packet[packet_size + HEADER_LEN];
  uint8_t *ptr = packet;
  //if (len ==256){
  opcode = ntohl(opcode);
	ret_code = ntohs(ret_code);
	len = ntohs(len);

	
	//assigning values
	memcpy(packet, &len,2);

	memcpy(packet+2, &opcode,4);

	memcpy(packet+6,&ret_code,2);
	// checks size of packet, decides if we need block size
	if (packet_size == JBOD_BLOCK_SIZE+HEADER_LEN){
		memcpy(packet+8, block, JBOD_BLOCK_SIZE);
	}
	
	return ptr;
  }
  


static bool send_packet(int sd, uint32_t op, uint8_t *block) {
	uint32_t len;
	uint16_t ret_code = 0;
  // seeing if op is write command, tells us length of packet
	if ((op >> 26) == JBOD_WRITE_BLOCK)
	  {
		  len = HEADER_LEN + 256;
	  }
	
	  else
	  {
	  	len = HEADER_LEN;
	  }
	
	uint8_t *packet;
	packet = createpacket(len, ret_code,op, block);
	
	if (nwrite(sd, len, packet) == false)
	  {
		  return false;
	  }
	  return true;
	
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {

  struct sockaddr_in caddr;
  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(port);
  cli_sd = socket(AF_INET, SOCK_STREAM, 0);

  if (cli_sd== -1){
    return false;
  }
  if (inet_aton(ip, &caddr.sin_addr)==0){

    return false;
  }
  
  if(connect(cli_sd, (const struct sockaddr *) &caddr,sizeof(caddr))==-1){
    return false;
  }
  return true;



}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  close(cli_sd);
  cli_sd =-1;
}
    
/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {
    uint16_t ret;
    send_packet(cli_sd,op,block);
    recv_packet(cli_sd,&op,&ret,block);
    return ret;
}
