/*
 *Course: CNT 5505 Data and Computer Communications
 *Semester: Spring 2016
 *Name: Kunal SinghaRoy
 * */
#define _GNU_SOURCE
#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <pthread.h>
#include "ether.h"
#include "ip.h"
#include "bridge.h"

/*This function creates and monitors the socket*/
void createmonitorSocket(char* lan_name);
/*This function will intialise the client list*/ 
void intialiseClientlist(int clients[]);
/*This function will store the socket information into the symbolic link files*/
void storesocketinfo(int sockfd,char * lan_name);
/*This function will store the local ip address in symbolic link file*/
void storelocalIp(char * lan_name);
/*It will add the client in client list if there is any empty position otherwise it will return  0*/
int acceptclient(int clients[],int recv_sockfd);
/*This function will intialise the bridge table to NULL value*/
void initialisebrigetable();
/*Check if there is an entry for the mac address in the bridge table*/
int presentInbridgeTable(unsigned char * mac);
/*Add to the existing bridge table entry*/
void addTobridgeTable(unsigned char * mac,int sockfd);
/*This function will receive the packet and forward it accordingly*/
void receiveforwardpkt(int sockfd,int index,int clients[],fd_set *allset);
/*Return the iface to forward the packet.return -1 if the entry is missing in bridge table*/
int getIface(unsigned char * mac);
/*This function will forward the packet to the partcular socket*/
void forwardpacket(int sockfd,EtherPkt etherpkt,IP_PKT ippkt,ARP_PKT arppkt);
/*Remove stale entries from the list*/
/*void removestaleentries(BridgeEntry** bridgetable);*/
void * removestaleentries(void * arg);
/*This function will parse the command line into arguments*/
char** get_commandarguments(char* commandline);
/*This function will read the command that has been entered by the user*/
char* read_command();
/*This function will display bridge table information*/
void displaytableinfo();

/*This variable stores the number of ports of the bridge*/
int no_ports;
/*The lock for accessing bridge table.Because otherwise it may produce inconsistent read*/
pthread_mutex_t bridge_obj=PTHREAD_MUTEX_INITIALIZER;
/*Array for storing the bridge table entries*/
BridgeEntry** bridgetable;

int main(int argc, char *argv[])
{
  struct stat buf;
  char addr[100];
  /*The number of arguments is too less to start the bridge*/
  if (argc<3)
  {
   fprintf(stderr,"Insufficient number of arguments to start a bridge\n");
   exit(0);
  }
  /*Too many arguments to start a bridge*/
  if (argc>3)
  {
   fprintf(stderr,"Too many arguments to start a bridge\n");
   exit(0);
  }

  /*Check if LAN with same name already exist*/
  sprintf(addr,".%s.addr",argv[1]);
  if (lstat(addr,&buf)!=-1)
  {
   fprintf(stderr,"The LAN with same name already exist\n");
   exit(0);
  }
  no_ports=atoi(argv[2]);
  createmonitorSocket(argv[1]);
  return 0;
}

/*This function will create and monitor the socket*/
void createmonitorSocket(char *lan_name)
{
 int sockfd;
 int recv_sockfd;
 int clients[no_ports];
 int counter;
 int th_return;
 bridgetable=malloc(no_ports*sizeof(BridgeEntry *));
 pthread_t bridgetable_thread;
 struct sockaddr_in addr;
 struct sockaddr_in recv_addr;
 fd_set allset;
 fd_set resset;
 int maxfd;
 int len;
 char* token_accept="accept";
 char* token_reject="reject";
 char* token_table="table";
 char* read_line;
 char** command_arguments;
 sockfd=socket(AF_INET,SOCK_STREAM,0);
 if (sockfd<0)
 {
  printf("Unable to create socket\n");
  exit(0);
 }
 addr.sin_addr.s_addr=INADDR_ANY;
 addr.sin_port=htons(0);
 addr.sin_family=AF_INET;
 if (bind(sockfd,(struct sockaddr *)&addr,sizeof(addr))<0)
 {
  perror("Bind:");
  exit(0);
 }
 storesocketinfo(sockfd,lan_name);
 maxfd=sockfd;
 FD_ZERO(&allset);
 FD_SET(sockfd,&allset);
 FD_SET(STDIN_FILENO,&allset);
 listen(sockfd,5);  
 initialisebrigetable(bridgetable);
 intialiseClientlist(clients);
 fprintf(stderr,"bridge:");
 if ((th_return=pthread_create(&bridgetable_thread, NULL, removestaleentries, (void*)NULL)) != 0) {
        printf("thread creation failed. %d\n", th_return);
  }

 for(;;)
 {
  resset=allset;
  select(maxfd+1,&resset,NULL,NULL,NULL);
  /*If the listening socket is active then accept the client*/
  if (FD_ISSET(sockfd,&resset))
  {
    memset(&recv_addr,'0',sizeof(recv_addr));
    len=sizeof(recv_addr);
    recv_sockfd=accept(sockfd,(struct sockaddr *)&recv_addr,(socklen_t *)&len);
    if (recv_sockfd<0)
    {
     perror("Accept:");
     continue;
    }
    if (acceptclient(clients,recv_sockfd))
    {
      if (recv_sockfd>maxfd)
      {
       maxfd=recv_sockfd;
      }
     /*Debug Information*/
     fprintf(stderr,"Station or router connected\n");
     FD_SET(recv_sockfd,&allset);
     write(recv_sockfd,token_accept,sizeof(token_accept));
    }
    else
    {
     fprintf(stderr,"No free ports are available\n");
     write(recv_sockfd,token_reject,sizeof(token_reject));
     close(recv_sockfd);
    }
    
  }
 
  /*If the client socket is active*/
  for(counter=0;counter<no_ports;counter++)
  {
   pthread_mutex_lock(&bridge_obj);
   if (FD_ISSET(clients[counter],&resset))
   {    
    receiveforwardpkt(clients[counter],counter,clients,&allset);   
   }
   pthread_mutex_unlock(&bridge_obj);
  }

  /*If stdin is active*/
  if (FD_ISSET(STDIN_FILENO,&resset))
  {
   /*Every time make it Null for each line in command prompt*/
    read_line=NULL;
    command_arguments=NULL;
    read_line=read_command();
    if (read_line!=NULL)
    {
      command_arguments=get_commandarguments(read_line);
      if (strcmp(command_arguments[0],token_table)==0)
      {
       pthread_mutex_lock(&bridge_obj);
       displaytableinfo();
       pthread_mutex_unlock(&bridge_obj);
      }
    }
    /*Debug Information*/
    /*fprintf(stderr,"Tring to free the memory for command prompt\n");*/
    if (read_line!=NULL)
    {
     free(read_line);
     free(command_arguments);
    }
    /* fprintf(stderr,"Able to free the memory for command prompt\n");*/
  }
   /*removestaleentries(bridgetable);*/
 }


}

/*This function will read the command that has been entered by the user*/
char* read_command()
{
 int buffersize=75;
 int position=0;
 int character;
 char* buffer=malloc(sizeof(char *)*buffersize);
 do
 {
  character=getchar();
  /*If character is a new line character make it a null terminated string string and return it*/
  if (character=='\n'&& position!=0)
  {
    buffer[position]='\0';
    return buffer;
  }
  else
  {
    buffer[position]=character;
    position++;
  }
 }while(position<74);
 if (position==74)
 {
   buffer[position]='\0';
 }
 return buffer;
}

/*This function will parse the command line into arguments*/
char** get_commandarguments(char* commandline)
{
 int buffer_size=75;
 int position=0;
 char** arguments=malloc(sizeof(char*)*buffer_size);
 char* token;
 char* token_delimiter=" \t\r";
 if (arguments==NULL)
 {
  fprintf(stderr,"Unable to tokenize commands\n");
  return arguments;
 }
 token=strtok(commandline,token_delimiter);
 while(token!=NULL)
 {
   arguments[position]=token;
   position++;
   token=strtok(NULL,token_delimiter);
 }
 arguments[position]=NULL;
 return arguments;
}

void displaytableinfo()
{
 int i;
 fprintf(stderr,"Mac address             ");
 fprintf(stderr,"Interface     ");
 fprintf(stderr,"Last access time\n");
 for(i=0;i<no_ports;i++)
 {
  if (bridgetable[i]!=NULL)
  {
   fprintf(stderr,"%s                ",bridgetable[i]->host);
   fprintf(stderr,"%d                ",bridgetable[i]->iface);
   fprintf(stderr,"%s\n",ctime(&bridgetable[i]->last_accesstime));
  }
 }
}

/*This function will intialise the bridge table to NULL value*/
void initialisebrigetable()
{
 int i; 
 for(i=0;i<no_ports;i++)
 {
  bridgetable[i]=NULL;
 }
}

/*This function will receive the packet and forward it accordingly*/
void receiveforwardpkt(int sockfd,int index,int clients[],fd_set* allset)
{
 int num;
 EtherPkt receivedpkt;
 ARP_PKT arppkt;
 IP_PKT ippkt;
 int type;
 int length;
 int forward_index;
 int counter;
 memset(&receivedpkt,'0',sizeof(receivedpkt));
 memset(&arppkt,'0',sizeof(arppkt));
 memset(&ippkt,'0',sizeof(ippkt));
 /*Debug Information*/
 /*fprintf(stderr,"Started reading the packet\n");*/
 num=read(sockfd,&receivedpkt.dst,18);
 /*If any of the clients closed then do the cleaning operation*/
 if (num==0)
 {
  fprintf(stderr,"Station or router closed\n");
  clients[index]=-1;
  FD_CLR(sockfd,allset);
  return;
 }

 read(sockfd,&receivedpkt.src,18);
 /*fprintf(stderr,"Destination Mac:%s ,Source Mac:%s\n",receivedpkt.dst,receivedpkt.src);*/
 read(sockfd,(char *)&receivedpkt.type,sizeof(short));
 read(sockfd,(char *)&receivedpkt.size,sizeof(short));
 type=ntohs(receivedpkt.type);
 if (!presentInbridgeTable(receivedpkt.src))
 {
  addTobridgeTable(receivedpkt.src,index);
 }
 /*Debug Information*/
 /*displaytableinfo();*/
 if (type==TYPE_IP_PKT)
 {
  /*Debug Information*/
  /*fprintf(stderr,"Ip packet received\n");*/
  read(sockfd,(char *)&ippkt.dstip,sizeof(IPAddr));
  read(sockfd,(char *)&ippkt.srcip,sizeof(IPAddr));
  read(sockfd,(char *)&ippkt.protocol,sizeof(short));
  read(sockfd,(char *)&ippkt.sequenceno,sizeof(unsigned long));
  read(sockfd,(char *)&ippkt.length,sizeof(short));
  length=ntohs(ippkt.length);
  memset(&ippkt.data,'\0',sizeof(ippkt.data));
  read(sockfd,ippkt.data,length);
 }
 else if (type==TYPE_ARP_PKT)
 {
  /*Debug Information*/
  /*fprintf(stderr,"ARP packet received\n");*/
  read(sockfd,(char *)&arppkt.op,sizeof(short));
  read(sockfd,(char *)&arppkt.srcip,sizeof(IPAddr));
  read(sockfd,&arppkt.srcmac,18);
  read(sockfd,(char *)&arppkt.dstip,sizeof(IPAddr));
  read(sockfd,&arppkt.dstmac,18);
 }
 else
 {
  fprintf(stderr,"Unknown type packet received.Something wrong with reading\n");
  return;
 }
 /*Debug Information*/
 /*fprintf(stderr,"Finished reading the packet\n");*/
 forward_index=getIface(receivedpkt.dst);
 if (forward_index>-1)
 {
  forwardpacket(clients[forward_index],receivedpkt,ippkt,arppkt);
 }
 else
 {
  for(counter=0;counter<no_ports;counter++)
  {
   if (counter==index)
   {
    continue;
   }
   if (clients[counter]>=0)
   {
    forwardpacket(clients[counter],receivedpkt,ippkt,arppkt);
   }
  }
 }
}

/*This function will forward the packet to the partcular socket*/
void forwardpacket(int sockfd,EtherPkt etherpkt,IP_PKT ippkt,ARP_PKT arppkt)
{
 int type;
 int length;
 /*Debug Information*/
 /*fprintf(stderr,"Forwarding the packet\n");*/
 write(sockfd,&etherpkt.dst,18);
 write(sockfd,&etherpkt.src,18);
 write(sockfd,(char *)&etherpkt.type,sizeof(etherpkt.type));
 write(sockfd,(char *)&etherpkt.size,sizeof(etherpkt.size));
 type=ntohs(etherpkt.type);
 if (type==TYPE_IP_PKT)
 {
  /*Debug Information*/
  /*fprintf(stderr,"Forwarding Ip packet\n");*/
  write(sockfd,(char *)&ippkt.dstip,sizeof(IPAddr));
  write(sockfd,(char *)&ippkt.srcip,sizeof(IPAddr));
  write(sockfd,(char *)&ippkt.protocol,sizeof(short));
  write(sockfd,(char *)&ippkt.sequenceno,sizeof(unsigned long));
  write(sockfd,(char *)&ippkt.length,sizeof(short));
  length=ntohs(ippkt.length);
  /*memset(&ippkt.data,'\0',sizeof(ippkt.data));*/
  write(sockfd,ippkt.data,length);
 }
 else if (type==TYPE_ARP_PKT)
 {
  /*Debug Information*/
  /*fprintf(stderr,"Forwarding ARP packet\n");*/
  write(sockfd,(char *)&arppkt.op,sizeof(short));
  write(sockfd,(char *)&arppkt.srcip,sizeof(IPAddr));
  write(sockfd,arppkt.srcmac,18);
  write(sockfd,(char *)&arppkt.dstip,sizeof(IPAddr));
  write(sockfd,arppkt.dstmac,18);
 }
 /*fprintf(stderr,"Finished forwarding the packet\n");*/
}

/*Return the iface to forward the packet.return -1 if the entry is missing in bridge table*/
int getIface(unsigned char * mac)
{
 int retval=-1;
 int i;
 BridgeEntry entry;
 for(i=0;i<no_ports;i++)
 {
  if (bridgetable[i]!=NULL)
  {
   entry=*bridgetable[i];
   if (strcmp((char *)entry.host,(char *)mac)==0)
   {
    /*Debug Information*/
    /*fprintf(stderr,"Entry already exist in bridge table\n");*/
    return entry.iface;
   }
  }
 }
 return retval;
}

/*Remove stale entries from the list*/
void * removestaleentries(void * arg)
{
 int i;
 BridgeEntry entry;
 time_t nowtime;
 double diff_t;
 while(1)
 {
  pthread_mutex_lock(&bridge_obj);
  time(&nowtime);
  for(i=0;i<no_ports;i++)
  {
   if (bridgetable[i]!=NULL)
   {
    entry=*bridgetable[i];
    diff_t=difftime(nowtime,entry.last_accesstime);
    /*If a host has not sent any message for more than 10 minutes remove it from bridge table*/
    if (diff_t>600)
    {
     /*Debug Information*/
     /*fprintf(stderr,"Removing stale entry from bridge table\n");*/
     free(bridgetable[i]);
     bridgetable[i]=NULL;
    }
   }
  }
  pthread_mutex_unlock(&bridge_obj);
  sleep(60);
 }
}

/*Add to the existing bridge table entry*/
void addTobridgeTable(unsigned char * mac,int index)
{
 int i;
 BridgeEntry* entry=malloc(sizeof(BridgeEntry));
 strncpy((char *)entry->host,(char *)mac,18);
 /*Debug Information*/
 /*fprintf(stderr,"Mac trying to add:%s ,Mac adding :%s\n",mac,entry->host);*/
 entry->iface=index;
 time(&entry->last_accesstime);
 for(i=0;i<no_ports;i++)
 {
  if (bridgetable[i]==NULL)
  {
   /*Debug Information*/
   /*fprintf(stderr,"Adding to the bridge table\n");*/
   bridgetable[i]=entry;
   break;
  }
 }
 /*Debug Information*/
 /*fprintf(stderr,"After adding entry the contents of the bridge table are:\n");
 for(i=0;i<no_ports;i++)
 {
  if (bridgetable[i]!=NULL)
  {
   fprintf(stderr,"%s\n",bridgetable[i]->host);
  }
 }*/
}

/*Check if there is an entry for the mac address in the bridge table*/
int presentInbridgeTable(unsigned char * mac)
{
 int retval=0;
 int i;
 /*Debug Information*/
 /*int j;*/
 BridgeEntry* entry;
 for(i=0;i<no_ports;i++)
 {
  if (bridgetable[i]!=NULL)
  {
   entry=bridgetable[i];
   if (strcmp((char *)entry->host,(char *)mac)==0)
   {
    /*Debug Information*/
    /*fprintf(stderr,"Entry already exist.Will just update the time stamp\n");
    fprintf(stderr,"Previous time value %s\n",ctime(&entry->last_accesstime));*/
    time(&entry->last_accesstime);
    /*fprintf(stderr,"Present access time value is %s\n",ctime(&entry->last_accesstime));*/
    bridgetable[i]=entry;
    /*fprintf(stderr,"The bridge table contents after updating\n");
    for(j=0;j<no_ports;j++)
    {
     if (bridgetable[j]!=NULL)
     {
      fprintf(stderr,"%s\n",bridgetable[j]->host);
     }
    }*/
    return 1;
   }
  }
 }
 return retval;
}

/*It will add the client in client list if there is any empty position otherwise it will return  0*/
int acceptclient(int clients[],int recv_sockfd)
{
 int i;
 int retval=0;
 for(i=0;i<no_ports;i++)
 {
  if (clients[i]<0)
  {
   clients[i]=recv_sockfd;
   return 1;
  }
 }
 return retval;
}

/*This function will intialise the client list to empty value*/
void intialiseClientlist(int clients[])
{
 int i;
 for(i=0;i<no_ports;i++)
 {
  clients[i]=-1;
 }
}

/*This function will store the socket information into the symbolic link files*/
void storesocketinfo(int sockfd,char * lan_name)
{
 struct sockaddr_in addr;
 char fileName[100];
 char port[10];
 int len;
 storelocalIp(lan_name);
 len=sizeof(addr);
 if (getsockname(sockfd,(struct sockaddr *)&addr,(socklen_t *)&len)<0)
 {
   perror("Can not get socket information:");
   exit(0);
 }
 sprintf(port,"%d",ntohs(addr.sin_port));
 sprintf(fileName,".%s.port",lan_name);
 if (symlink(port,fileName)<0)
 {
  printf("Symbolic link file for port number could not be created.Exiting application\n");
  exit(0);
 }

}

/*This function will store the local ip address in symbolic link file*/
void storelocalIp(char * lan_name)
{
 char hostname[255];
 char fileName[100];
 int i=0;
 struct hostent* hostentry;
 char * szLocalIP;
 gethostname(hostname,255);
 hostentry=gethostbyname(hostname);
 while(hostentry->h_addr_list[i]!=NULL)
 {
  szLocalIP = inet_ntoa (*(struct in_addr *)hostentry->h_addr_list[i]);
  /*fprintf(stderr,"%s\n",szLocalIP);*/
  i++;
 }
 sprintf(fileName,".%s.addr",lan_name);
 if (symlink(szLocalIP,fileName)<0)
 {
  printf("Symbolic link file could not be created.Exiting application\n");
  exit(0);
 }
}



