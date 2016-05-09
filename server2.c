#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "functions.h"

#define BUFLEN 512
#define PORT_DATA "5555"

int main(int argc, char *argv[])
{
  int seq_nb_size = 6;
  int BUFTEMPLEN = BUFLEN - seq_nb_size;
  struct sockaddr_in SA_me_con, SA_me_data, SA_client;//to listen to the port conexion ; listen to the data port ; get info from client in recvfrom
  int s_con, s_data;//socket for connexion ; data
  socklen_t sLen=sizeof(SA_client);
  char buf[BUFLEN-6];
  char buf_final[BUFLEN];
  char buf2[6];
  char seq_nb_char[6];
  FILE* fsend;
  int file_size, last_seq_nb, seq_nb, last_buf_size;
  int i, num_ACK;
  char buf_ACK[6];

  if(argc != 2){
    printf("./<server_name> <port_server>\n");
    exit(1);
  }

  int PORT_CON = atoi(argv[1]);

 /*************************************/
/***** CREATION OF THE SOCKETS *******/


  if ((s_con=socket(AF_INET, SOCK_DGRAM, 0))==-1)
    diep("socket connexion");
  if ((s_data=socket(AF_INET, SOCK_DGRAM, 0))==-1)
    diep("socket data");

  int optval = 1;
  setsockopt(s_con, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  memset((char *) &SA_me_con, 0, sizeof(SA_me_con));
  SA_me_con.sin_family = AF_INET;
  SA_me_con.sin_port = htons(PORT_CON);//port used to create the connexion
  SA_me_con.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s_con, (struct sockaddr*)&SA_me_con, sizeof(SA_me_con))==-1)//SA with the PORT_CON
      diep("bind");

  optval = 1;
  setsockopt(s_data, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  memset((char *) &SA_me_data, 0, sizeof(SA_me_data));
  int port_data= atoi(PORT_DATA);
  SA_me_data.sin_family = AF_INET;
  printf("port data %d\n", port_data);
  SA_me_data.sin_port = htons(port_data);//port used to exchange data
  SA_me_data.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s_data, (struct sockaddr*)&SA_me_data, sizeof(SA_me_data))==-1)//name the socket s_data
      diep("bind");

/*                                  */
/************************************/


/*************************************/
/* OPENING OF THE CONNECTION SYN/ACK */

  memset((char*)&SA_client, 0, sizeof(SA_client));
  if (recvfrom(s_con, buf, BUFLEN, 0, (struct sockaddr*)&SA_client, &sLen)==-1)
    diep("recvfrom()");
  printf("Received packet from %s : %d\n%s\n", inet_ntoa(SA_client.sin_addr), ntohs(SA_client.sin_port), buf);
  
  if (strcmp(buf, "SYN")==0){
    init(buf, BUFLEN);
    sprintf(buf, "SYN-ACK");
    strcat(buf, PORT_DATA);
    display(buf, BUFLEN);

    printf("Sending packet %s\n", buf);
    if(sendto(s_con, buf, BUFLEN, 0, (struct sockaddr*)&SA_client, sizeof(SA_client))==-1)
      diep("sendto() SYN-ACK");
  }
  init(buf, BUFLEN);
  if (recvfrom(s_con, buf, BUFLEN, 0, (struct sockaddr*)&SA_client, &sLen)==-1)
    diep("recvfrom() ACK");
  display(buf, BUFLEN);

  printf("___________________________________________________________\n\n");


/*                                         */
/*******************************************/


/********************************/
/******* FILE TRANSFERT ********/


  //Récéption du nom du fichier

  init(buf, BUFLEN);
  if (recvfrom(s_data, buf, BUFLEN, 0, (struct sockaddr*)&SA_client, &sLen)==-1)
    diep("recvfrom() file name");
  printf("I received the name of the file : %s\n", buf);

  //Calcul de la taille du fichier

  fsend = fopen(buf, "rb");
  if(fsend==NULL) {printf("error opening file\n"); exit(-1);}
  if(fseek(fsend, 0, SEEK_END)!=0)
    printf("error in fseek");
  file_size = ftell(fsend);//get the size of the file
  printf("The size of the file is %d bytes\n", file_size);

  last_seq_nb = (int)(file_size/(BUFLEN-1)) + 1;

  if (fseek(fsend, 0, SEEK_SET)!=0)//reset the cursor at the beginning of the file
    printf("error fseek SET\n");

  //Envoi des séquences

  for (seq_nb=1; seq_nb<=last_seq_nb; seq_nb++){
    int cpt = 0;
    
    init(buf, BUFLEN);
    init(buf2, 6);

    if(feof(fsend)!=0)
      printf("EOF reached too early\n");
    if(ferror(fsend))
      printf("error reading file\n");

    fread(buf, sizeof(char), BUFLEN-1, fsend);

    sprintf(buf2,"%06d",seq_nb);
    sprintf(buf_final, "%s%s", buf2, buf);
    printf("seq_number = %d\n", seq_nb);    

    do{ //Revoir le do/while
      
      if (cpt == 1){ //Reviens au début si le premier envoi à échoué
        if (fseek(fsend, seq_nb*BUFLEN, SEEK_SET)!=0)
          printf("error fseek SET\n");
      }

      if (sendto(s_data, buf_final, strlen(buf_final), 0, (struct sockaddr*)&SA_client, sizeof(SA_client))==-1)
        diep("sendto() data");
      printf("length of buf sent : %d\n", strlen(buf_final));
      init(buf_final, BUFLEN);
      init(buf, BUFLEN);    

      //Récéption de l'ACK

      if (recvfrom(s_data, buf, BUFLEN, 0, (struct sockaddr*)&SA_client, &sLen)==-1)
        diep("recvfrom() ack");
      printf("I received ack : \033[31m%s\033[0m\n", buf);

      //Vérification de l'ACK 
  
      for(i=0; i<6; i++){
        buf_ACK[i] = buf[i+3];
      }
    
      num_ACK = atoi(buf_ACK);
      printf("Le numéro d'ACK reçu est %d\n", num_ACK);   

      cpt = 1;
   
    }while(num_ACK != seq_nb);

  }

/*********************************/


/*************************/
/* END OF THE CONNECTION */

  init(buf, BUFLEN);
  sprintf(buf, "FIN");
  display(buf, BUFLEN);

  printf("Sending packet %s\n", buf);
  if(sendto(s_con, buf, BUFLEN, 0, (struct sockaddr*)&SA_client, sizeof(SA_client))==-1)
    diep("sendto() FIN");


  fclose(fsend);

  close(s_con);
  close(s_data);

/*                     */ 
/***********************/

 return 0; 
}
