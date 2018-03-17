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
   int bytes_sent, bytes_recd; /* number of bytes sent or received */
   FILE *fp;
   size_t nread;
   int client_packet_count, total_data_bytes_transmitted;
   struct timespec tim, tim2;

   //Sleep for 500000000 nanoseconds = 500ms
   tim.tv_sec  = 0;
   tim.tv_nsec = 500*100000L;
   /* open a socket */
   struct Header *head = malloc(sizeof(struct Header));
   head->sequence = 0;

   printf("Header size: %lu\n", sizeof(struct Header));

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

   printf("Enter hostname of server: ");
   scanf("%s", server_hostname);
   if ((server_hp = gethostbyname(server_hostname)) == NULL) {
      perror("Client: invalid server hostname");
      close(sock_client);
      exit(1);
   }

   printf("Enter port number for server: ");
   scanf("%hu", &server_port);

   /* Clear server address structure and initialize with server address */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
                                    server_hp->h_length);
   server_addr.sin_port = htons(server_port);

    /* connect to the server */

   if (connect(sock_client, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Client: can't connect to server");
      close(sock_client);
      exit(1);
   }

   /* user interface */

  //Open the text file to send
  fp = fopen("test1.txt", "r");
  //Set counts to 0
  client_packet_count = 0;
  total_data_bytes_transmitted = 0;
  //If it successfully opened then start reading
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
        close(sock_client);
        exit(1);
      }

      msg_len = strlen(sentence);
      head->sequence = head->sequence + 1;
      head->count = nread;
      send(sock_client, head, sizeof(struct Header), 0);
      bytes_sent = send(sock_client, sentence, nread, 0);
      printf("Packet %i transmitted with %i data bytes\n", head->sequence, head->count);
      client_packet_count += 1;
      total_data_bytes_transmitted += bytes_sent;

      if(nanosleep(&tim , &tim2) < 0 )
      {
         printf("Nano sleep system call failed \n");
         return -1;
      }
    }

    head->sequence = 0;
    head->count = 0;
    send(sock_client, head, sizeof(struct Header), 0);
    bytes_sent = send(sock_client, sentence, 0, 0);

    printf("End of Transmission Packet with sequence number %i transmitted with %i data bytes\n", head->sequence, head->count);
    printf("Number of packets received: %i\n", client_packet_count);
    printf("Total number of data bytes received: %i\n", total_data_bytes_transmitted);
  }

  fclose(fp);


  /* send message */



   /* get response from server */

   //bytes_recd = recv(sock_client, modifiedSentence, STRING_SIZE, 0);

   // if(bytes_recd < 0){
   //   perror("There was an error upon receiving");
   //   close(sock_client);
   //   exit(1);
   // }
   //
   // printf("\nThe response from server is:\n");
   // printf("%s\n\n", modifiedSentence);

   /* close the socket */

   close (sock_client);
   free(head);
}
