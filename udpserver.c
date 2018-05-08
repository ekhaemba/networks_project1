/* udp_server.c */
/* Programmed by Adarsh Sethi */
/* February 21, 2018 */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include "header.h"
#include <math.h>

#define STRING_SIZE 1024

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 45000

int powah(int base, int power){
  if(power <= 1){
    return base;
  }else{
    return base*powah(base,power - 1);
  } 
}

void debrief(int init_trans, int total_bytes, int retrans, int total_trans, int acks, int timeouts){
  printf("Number of data packets transmitted: %d\n",init_trans);
  printf("Total number of data bytes transmitted: %d\n",total_bytes);
  printf("Total number of retransmissions: %d\n", retrans);
  printf("Total number of data packets transmitted: %d\n", total_trans);
  printf("Number of ACKs received: %d\n", acks);
  printf("Count of how many times timeout expired: %d\n", timeouts);
}

int main(int argc, char** argv) {

   int sock_server, bytes_recd;  /* Socket on which server listens to clients */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   unsigned short server_port;  /* Port number used by server (local port) */

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */

   char sentence[STRING_SIZE];  /* receive message */
   /* number of bytes sent*/
   unsigned int i;  /* temporary loop variable */
   size_t nread;
   int init_packet_count, total_bytes_trans, retrans, acks, timeouts;
   init_packet_count = total_bytes_trans = retrans = acks = timeouts = 0;
   FILE *fp;

   struct HeaderUDP head;
   struct timeval timeout;
   short int expectedACK = 0;
   short int receivedACK;

  if(argc != 2){//Incorrect number of arguments
    perror("Format: ./udpserver <timeout_n>\n");
    exit(1);
  }

  int n = atoi(argv[1]);
  if(n <=0 || n > 10){
    perror("<timeout_n> must be between the range 1 <= n <= 10\n");
    exit(1);
  }

   timeout.tv_sec= 0;
   timeout.tv_usec = powah(10,n);

   /* open a socket */

   if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Server: can't open datagram socket\n");
      exit(1);
   }

   /* initialize server address information */

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                        any host interface, if more than one
                                        are present */
   server_port = SERV_UDP_PORT; /* Server will listen on this port */
   server_addr.sin_port = htons(server_port);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
      perror("Server: can't bind to local address\n");
      close(sock_server);
      exit(1);
   }

   /* wait for incoming messages in an indefinite loop */

   printf("Waiting for incoming messages on port %hu\n\n", 
                           server_port);

   client_addr_len = sizeof (client_addr);


   // int init_packet_count, total_bytes, retrans, total_packet_count, acks, timeouts;
   for (;;) {//Wait for call from above state

      bytes_recd = recvfrom(sock_server, &sentence, STRING_SIZE, 0, (struct sockaddr *) &client_addr, &client_addr_len);

      if(bytes_recd < 0){
        perror("File name receive error\n");
        close(sock_server);
        exit(1);
      }

      //From now on timeouts exist
      setsockopt(sock_server, SOL_SOCKET, SO_RCVTIMEO,(const void *) &timeout, sizeof(timeout));

      fp = fopen(sentence, "r");
      if(fp){
        /*
        Read 80 bytes from the text file, newlines included
        If we read less than 80 bytes it means we are at the end of the file
        Next packets will be an EOF pair
        */
        while((nread = fread(sentence, 1, 80, fp)) > 0){

          //If you throw an error shut it down
          if(ferror(fp)){
            perror("Threw a file error");
            fclose(fp);
            close(sock_server);
            exit(1);
          }
         //Assemble data packet
         head.sequence = expectedACK;
         head.count = nread;
         strcpy(head.data, sentence);
         
         //Send data packet
         sendto(sock_server, &head, sizeof(struct HeaderUDP), 0, (struct sockaddr *) &client_addr, sizeof (client_addr));
         printf("Packet %d transmitted with %d data bytes\n", head.sequence, head.count);
         init_packet_count += 1;
         total_bytes_trans += head.count;

          //Wait for ACK stage
          while(1){
            bytes_recd = recvfrom(sock_server, &receivedACK, sizeof(short int), 0, (struct sockaddr *) &client_addr, &client_addr_len);
            if(bytes_recd <= 0){//A timeout occurred
               printf("Timeout expired for packet number %d\n", head.sequence);
               sendto(sock_server, &head, sizeof(struct HeaderUDP), 0, (struct sockaddr *) &client_addr, sizeof (client_addr));
               printf("Packet %d retransmitted with %d data bytes\n", head.sequence, head.count);
               retrans += 1;
               timeouts += 1;
               continue;
            }
            else if(expectedACK != receivedACK){//Received incorrect ACK
               printf("ACK %d received\n", receivedACK);
               acks += 1;
               continue;
            }
            else{//Correct ACK received go to next data packet
               printf("ACK %d received\n", receivedACK);
               expectedACK = 1 - receivedACK;
               acks += 1;
               break;
            }
          }
        }
        //We finished transmitting all of the data packets
        //Send EOT packet

        head.sequence = 0;
        head.count = 0;
        strcpy(head.data,"");
        printf("End of Transmission Packet with sequence number %d transmitted with %d data bytes\n", head.sequence, head.count);
        sendto(sock_server, &head, sizeof(struct HeaderUDP), 0, (struct sockaddr *) &client_addr, sizeof (client_addr));
        //Debrief
        printf("\n");
        debrief(init_packet_count, total_bytes_trans, retrans, retrans + init_packet_count, acks, timeouts);
      }

      /* prepare the message to send */

      /* send message */
      close(sock_server);
      break;
   }
}
