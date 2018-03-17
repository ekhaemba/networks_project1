/* tcpserver.c */
/* Programmed by Adarsh Sethi */
/* February 21, 2018 */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, bind, listen, accept */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include "header.h"

#define STRING_SIZE 1024

/* SERV_TCP_PORT is the port number on which the server listens for
   incoming requests from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_TCP_PORT 45000

int main(void) {

   int sock_server;  /* Socket on which server listens to clients */
   int sock_connection;  /* Socket on which server exchanges data with client */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   unsigned int server_addr_len;  /* Length of server address structure */
   unsigned short server_port;  /* Port number used by server (local port) */

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */

   char sentence[STRING_SIZE];  /* receive message */
   char modifiedSentence[STRING_SIZE]; /* send message */
   unsigned int msg_len;  /* length of message */
   int bytes_header, bytes_sentence; /* number of bytes sent or received */
   unsigned int i;  /* temporary loop variable */
   unsigned short server_packet_count, total_data_bytes_recieved;
   FILE *fp;

   struct Header *received_header = malloc(sizeof(struct Header));
   /* open a socket */

   if ((sock_server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("Server: can't open stream socket");
      exit(1);
   }

   /* initialize server address information */

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                        any host interface, if more than one
                                        are present */
   server_port = SERV_TCP_PORT; /* Server will listen on this port */
   server_addr.sin_port = htons(server_port);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Server: can't bind to local address");
      close(sock_server);
      exit(1);
   }

   /* listen for incoming requests from clients */

   if (listen(sock_server, 50) < 0) {    /* 50 is the max number of pending */
      perror("Server: error on listen"); /* requests that will be queued */
      close(sock_server);
      exit(1);
   }
   printf("I am here to listen ... on port %hu\n\n", server_port);

   client_addr_len = sizeof (client_addr);

   /* wait for incoming connection requests in an indefinite loop */

   server_packet_count = 0;
   total_data_bytes_recieved = 0;
   short finished_executing = 0;
   for (;;) {

      sock_connection = accept(sock_server, (struct sockaddr *) &client_addr,
                                         &client_addr_len);
                     /* The accept function blocks the server until a
                        connection request comes from a client */
      if (sock_connection < 0) {
         perror("Server: accept() error\n");
         close(sock_server);
         exit(1);
      }
      fp = fopen("test1-out.txt", "w+");

      while(finished_executing != 1){
        /* receive the message */
        bytes_header = recv(sock_connection, received_header, sizeof(struct Header), 0);
        bytes_sentence = recv(sock_connection, sentence, STRING_SIZE, 0);

        if(bytes_header < 0 || bytes_sentence < 0){
          perror("There was an error upon receiving");
          close(sock_connection);
          exit(1);
        }

        if (bytes_header > 0){
             printf("Packet %i received with %i data bytes\n", received_header->sequence, bytes_sentence);
             server_packet_count += 1;
             total_data_bytes_recieved += bytes_sentence;

             if(received_header->sequence == 0){
               finished_executing = 1;
             }else{
               if(fp){
                 fwrite(sentence, 1, bytes_sentence, fp);
               }
             }
        }
      }
      printf("End of Transmission Packet with sequence number %i received with %i data bytes\n", received_header->sequence, received_header->count);
      printf("Number of packets received: %i\n", server_packet_count);
      printf("Total number of data bytes received: %i\n", total_data_bytes_recieved);

      /* close the socket */
      close(sock_connection);
      free(received_header);
      break;
   }

}
