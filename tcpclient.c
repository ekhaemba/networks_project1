/* tcp_ client.c */
/* Programmed by Adarsh Sethi */
/* February 21, 2018 */

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, connect, send, and recv */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <time.h>           /* for nanosleep */
#include "header.h"

#define STRING_SIZE 1024
#define SERVER_PORT 45000

int main(void) {

   int sock_client;  /* Socket used by client */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   struct hostent * server_hp;      /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE]; /* Server's hostname */
   unsigned short server_port;  /* Port number used by server (remote port) */

   char sentence[STRING_SIZE];  /* send message */
   //char modifiedSentence[STRING_SIZE]; /* receive message */
   unsigned int msg_len;  /* length of message */
   int bytes_header, bytes_sentence; /* number of bytes sent or received */
   FILE *fp;
   int client_packet_count, total_data_bytes_recieved;
   struct timespec tim, tim2;

   //Sleep for 500000000 nanoseconds = 500ms
   tim.tv_sec  = 0;
   tim.tv_nsec = 500*100000L;
   /* open a socket */
   struct Header *received_header = malloc(sizeof(struct Header));

   if ((sock_client = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("Client: can't open stream socket");
      exit(1);
   }

   /* Note: there is no need to initialize local client address information
            unless you want to specify a specific local port
            (in which case, do it the same way as in udpclient.c).
            The local address initialization and binding is done automatically
            when the connect function is called later, if the socket has not
            already been bound. */

   /* initialize server address information */

   if ((server_hp = gethostbyname("localhost")) == NULL) {
      perror("Client: invalid server hostname");
      close(sock_client);
      exit(1);
   }

   /* Clear server address structure and initialize with server address */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
                                    server_hp->h_length);
   server_addr.sin_port = htons(SERVER_PORT);

    /* connect to the server */

   if (connect(sock_client, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Client: can't connect to server");
      close(sock_client);
      exit(1);
   }

   printf("Please input the filename:\n");
  scanf("%s", sentence);
  msg_len = strlen(sentence) + 1;

      /* send message */

   bytes_sentence = send(sock_client, sentence, msg_len, 0);

   if(bytes_sentence < 0){
     perror("Filename send error");
     close(sock_client);
     exit(1);
   }

   /* user interface */

  //Open the text file to send
  fp = fopen("out.txt", "w+");
  //Set counts to 0
  client_packet_count = 0;
  total_data_bytes_recieved = 0;
  short finished_executing = 0;

  while(finished_executing != 1){
    /* receive the message */
    bytes_header = recv(sock_client, received_header, sizeof(struct Header), 0);
    bytes_sentence = recv(sock_client, sentence, STRING_SIZE, 0);

    if(bytes_header < 0 || bytes_sentence < 0){
      perror("There was an error upon receiving");
      close(sock_client);
      exit(1);
    }

    if (bytes_header > 0){
         printf("Packet %i received with %i data bytes\n", received_header->sequence, bytes_sentence);
         client_packet_count += 1;
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
  printf("Number of packets received: %i\n", client_packet_count);
  printf("Total number of data bytes received: %i\n", total_data_bytes_recieved);

  fclose(fp);
  close (sock_client);
  free(received_header);
}
