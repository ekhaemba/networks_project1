/* udp_client.c */ 
/* Programmed by Adarsh Sethi */
/* February 21, 2018 */

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include "header.h"
#include <time.h>

#define STRING_SIZE 1024
#define SERV_UDP_PORT 45000

short simulateLoss(float lossRate){
   if((float)rand()/RAND_MAX < lossRate){
      return 1;
   }
   else{
      return 0;
   }
}

void debrief(int recd_success, int delivered, int duplicates, int dropped, int total_packets, int acks_success, int acks_dropped, int acks_total){
  printf("Number of data packets received successfully: %d\n",recd_success);
  printf("Total number of data bytes which are delivered to the user: %d\n",delivered);
  printf("Total number of duplicate data packets received: %d\n", duplicates);
  printf("Number of data packets received but dropped due to loss: %d\n", dropped);
  printf("Total number of data packets received: %d\n", total_packets);
  printf("Number of ACKs transmitted without loss: %d\n", acks_success);
  printf("Number of ACKs generated but dropped due to loss: %d\n", acks_dropped);
  printf("Total number of ACKs generated: %d\n", acks_total);
}

int main(int argc, char** argv) {

   int sock_client;  /* Socket used by client */ 

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned short client_port;  /* Port number used by client (local port) */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   struct hostent * server_hp;      /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE]; /* Server's hostname */
   unsigned short server_port;  /* Port number used by server (remote port) */

   char sentence[STRING_SIZE];  /* send message */
   char modifiedSentence[STRING_SIZE]; /* receive message */
   unsigned int msg_len;  /* length of message */
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
   FILE *fp;
   struct HeaderUDP head;
   short int nextACK = 0;
   float packLoss, ackLoss;
   int recd_success,  delivered,  duplicates,  dropped,  acks_success,  acks_dropped;
   recd_success = delivered = duplicates = dropped = acks_success = acks_dropped = 0;

   srand(time(NULL));//Seed the random number generator

   if(argc != 3){
   //Incorrect number of arguments
    perror("Format: ./udpclient <packet_loss> <ack_loss>\n");
    exit(1);
   }

   packLoss = atof(argv[1]);
   ackLoss = atof(argv[2]);

   if(packLoss < 0 || packLoss > 1 || ackLoss < 0 || ackLoss > 1){//Incorrect range of arguments
    perror("The loss arguments must be between [0,1)\n");
    exit(1);
   }

   /* open a socket */

   if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Client: can't open datagram socket\n");
      exit(1);
   }

   /* Note: there is no need to initialize local client address information
            unless you want to specify a specific local port.
            The local address initialization and binding is done automatically
            when the sendto function is called later, if the socket has not
            already been bound. 
            The code below illustrates how to initialize and bind to a
            specific local port, if that is desired. */

   /* initialize client address information */

   client_port = 0;   /* This allows choice of any available local port */

   /* Uncomment the lines below if you want to specify a particular 
             local port: */
   /*
   printf("Enter port number for client: ");
   scanf("%hu", &client_port);
   */

   /* clear client address structure and initialize with client address */
   memset(&client_addr, 0, sizeof(client_addr));
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
                                        any host interface, if more than one 
                                        are present */
   client_addr.sin_port = htons(client_port);

   /* bind the socket to the local client port */

   if (bind(sock_client, (struct sockaddr *) &client_addr,
                                    sizeof (client_addr)) < 0) {
      perror("Client: can't bind to local address\n");
      close(sock_client);
      exit(1);
   }

   /* end of local address initialization and binding */

   /* initialize server address information */

   if ((server_hp = gethostbyname("localhost")) == NULL) {
      perror("Client: invalid server hostname\n");
      close(sock_client);
      exit(1);
   }

   /* Clear server address structure and initialize with server address */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
                                    server_hp->h_length);
   server_addr.sin_port = htons(SERV_UDP_PORT);

   /* user interface */

   printf("Please input a filename:\n");
   scanf("%s", sentence);
   msg_len = strlen(sentence) + 1;

   unsigned int server_addr_len = sizeof(server_addr);
   /* send message */
  
   bytes_sent = sendto(sock_client, sentence, msg_len, 0, (struct sockaddr *) &server_addr, sizeof(server_addr));

   /* get response from server */
   
   
   fp = fopen("out.txt", "w+");
   if(fp){
      for(;;){
         printf("Waiting for response from server...\n");
         bytes_recd = recvfrom(sock_client, &head, sizeof(struct HeaderUDP), 0, (struct sockaddr *) &server_addr, &server_addr_len);
         if(head.count == 0){//If we received an EOT packet break out of the loop
            printf("End of transmission packet with sequence number %d received with %d data bytes\n", head.sequence, head.count);
            break;
         }

         else if(simulateLoss(packLoss)){
          printf("Packet %d lost\n", head.sequence);
          dropped += 1;
         }

         //------
         //Simulate packet loss
         //------
         else if(head.sequence != nextACK){//If the sequence number is duplicate, send an ACK.
            printf("Duplicate packet %d received with %d data bytes\n", head.sequence, head.count);
            short int pseudoACK = 1 - nextACK;
            duplicates += 1;
            if(simulateLoss(ackLoss)){
              printf("ACK %d lost\n", head.sequence);
              acks_dropped += 1;
            }
            else{
              printf("ACK %d transmitted\n", head.sequence);
              bytes_sent = sendto(sock_client, &pseudoACK, sizeof(short int), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
              acks_success += 1;
            }
         }
         else{//If this is the correct ACK Deliver the data and transition states
            printf("Packet %d received with %d data bytes\n", head.sequence, head.count);
            strcpy(sentence, head.data);
            fwrite(sentence, 1, head.count, fp);
            delivered += head.count;
            recd_success += 1;
            if(simulateLoss(ackLoss)){
              printf("ACK %d lost\n", head.sequence);
              acks_dropped += 1;
            }
            else{
              printf("ACK %d transmitted\n", head.sequence);
              bytes_sent = sendto(sock_client, &nextACK, sizeof(short int), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
              acks_success += 1;
            }
            nextACK = 1 - nextACK;
         }
      }
   }
   // printf("%s\n\n", head.data); //For reasons that are beyond me I can not print out the character array from the head object directly.
   printf("\n");
   debrief(recd_success, delivered, duplicates, dropped, recd_success + duplicates + dropped, acks_success, acks_dropped, acks_dropped + acks_success);
   /* close the socket */

   close (sock_client);
}
