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
#include <time.h>           /* for nanosleep */
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
   unsigned int i;  /* temporary loop variable */
   int bytes_sentence;
   size_t nread;
   unsigned short server_packet_count, total_data_bytes_transmitted;
   FILE *fp;

   struct Header *head = malloc(sizeof(struct Header));
   struct timespec tim, tim2;
   head->sequence = 0;

   //Sleep for 500000000 nanoseconds = 500ms
   tim.tv_sec  = 0;
   tim.tv_nsec = 500*100000L;
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
   total_data_bytes_transmitted = 0;
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

      bytes_sentence = recv(sock_connection, sentence, STRING_SIZE, 0);

      if(bytes_sentence < 0){
        perror("File name receive error\n");
        close(sock_connection);
        exit(1);
      }

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
            close(sock_connection);
            exit(1);
          }

          msg_len = strlen(sentence);
          head->sequence = head->sequence + 1;
          head->count = nread;
          send(sock_connection, head, sizeof(struct Header), 0);
          bytes_sentence = send(sock_connection, sentence, nread, 0);
          printf("Packet %i transmitted with %i data bytes\n", head->sequence, head->count);
          server_packet_count += 1;
          total_data_bytes_transmitted += bytes_sentence;

          if(nanosleep(&tim , &tim2) < 0 )
          {
             printf("Nano sleep system call failed \n");
             return -1;
          }
        }

        head->sequence = 0;
        head->count = 0;
        send(sock_connection, head, sizeof(struct Header), 0);
        bytes_sentence = send(sock_connection, sentence, 0, 0);
        
        server_packet_count += 1;
        total_data_bytes_transmitted += bytes_sentence;

        printf("End of Transmission Packet with sequence number %i transmitted with %i data bytes\n", head->sequence, head->count);
        printf("Number of packets transmitted: %i\n", server_packet_count);
        printf("Total number of data bytes transmitted: %i\n", total_data_bytes_transmitted);
      }

///
///

      /* close the socket */
      close(sock_connection);
      free(head);
      break;
   }

}
