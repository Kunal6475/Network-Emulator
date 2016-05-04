/*
 *Course:CNT5505 Data and Computer Communication
 *Semester:Spring 2016
 *Name:Kunal SinghaRoy
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include "ether.h"
#include "ip.h"

/*This function will read the host file and store the information*/
void readhostfile(char* fileName);
/*This function will read the interface file and store the information*/
void readifacefile(char* fileName);
/*This function will read the routing table file and store the information*/
void readrtablefile(char* fileName);
/*This function will connect to all the available LANs*/
void connectLan();
/*This is the main function which will monitor the socket*/
void monitorsocket();
/*This function will read the command that has been entered by the user*/
char* read_command();
/*This function will parse the command line into arguments*/
char** get_commandarguments(char* commandline);
/*This function will send the packet to the other station*/
void sendpacket(char* hostName,char * message);
/*This function will get the ip address corresponding to the host name*/
unsigned long getIpAddressofStation(char * hostName);
/*This function will get which Ip address to forward the packet based on the Ip Address of the packet*/
Rtable getIpAddressOfNextHop(unsigned long destIpAddr);
/*This function will send the ethernet frame*/
void sendetherframe(char * message,IPAddr destIp,IPAddr srcIp,MacAddr destMac,MacAddr srcMac,int sockfd,short pkt_type,short arp_type);
/*Get the socket descriptor*/
int getSocketFd(char * lanname);
/*Function to get the index of the iterface to which we need to send the data*/
int getIndexOftheIface(Rtable nextdest);
/*This function will get the message to be send from the arguments*/
char * getMessage(char * line);
/*This function will read from socket*/
void readsocket(int sockfd);
/*This function will check whether the Ip address is meant for the station.Return 1 if true else 0*/
int isPacketForStation(IPAddr destIp);
/*This function will try to find out if the ip packet is meant for the router.Return 1 if true else return false*/
int isPacketMeantforRouter(MacAddr dest);
/*This function routes the ip pkt based on the destination ip*/
void routeippkt(EtherPkt etherpkt,IP_PKT ippkt);
/*This function intialises the arp cache to null value*/
void intialisearpcache();
/*This function will return the index of the arp cache where there is entry with the corresponding mac address to destination IP address mapping.Otherwise it will 
 * return -1 if the mapping is absent*/
int getMacfromarpcache(IPAddr dest);
/*This function will intialise the pending packets to NULL value*/
void intialisependinglist();
/*This function stores the pending packet in Pending packet queue*/
void storependingpkt(PEND_PKT * pend_pkt);
/*This function will add entry into ARP cache*/
void addtoarpcache(IPAddr destIP,MacAddr destMac);
/*This function will get the index of the interface which contains the corresponding IP address*/
int getMacAddressIndex(IPAddr src);
/*This function will send the arp reply*/
void sendArpReply(int sockfd,ARP_PKT pkt);
/*This function will find out the index of the pending packets*/
int getIndexOfPendingPkts(IPAddr dest);
/*This function will display the routing table*/
void displayroutingtable();
/*This function will display the interface information*/
void displayinterfaceinfo();
/*This function will display the arp cahe information*/
void displayarpcache();
/*This function will get the index of the interface link which has that socket descriptor*/
int getIndexOfSocket(int sockfd);
/*This function will get the mac address index from the interface which has corresponding lan name*/
int getMacAddressFromLanname(char * lanName);
/*Variable to indicate whether the station is station or a router.If station value is 1 if router value is 0*/
int isStation;
/*Variable to store global all set of file descriptors to monitor*/
fd_set allset;
/*Variable to store the maximum file descriptor*/
int maxfd;
/*The array for storing arp cache*/
Arpc* arpCache[MAXHOSTS];
/*The array for storing pending packets*/
PEND_PKT* pendingPkts[MAXHOSTS];

int main(int argc,char * argv[])
{
 char * token_station="-no";
 char * token_router="-route";
 if (argc<5)
 {
  printf("Insufficient arguments to start the application\n");
  exit(0);
 }
 if (argc>5)
 {
  printf("Too many arguments to start the application\n");
  exit(0);
 }
 if (strcmp(argv[1],token_station)==0)
 {
  /*Debug Information*/
  /*fprintf(stderr,"This is a station\n");*/
  isStation=1;
 }
 else
 {
  if (strcmp(argv[1],token_router)==0)
  {
   /*fprintf(stderr,"This is a router\n");*/
   isStation=0;
  }
  else
  {
   fprintf(stderr,"Unknown type.Exiting the application\n");
   exit(0);
  }
 }
 readifacefile(argv[2]);
 readrtablefile(argv[3]);
 readhostfile(argv[4]);
 connectLan();
 intialisearpcache();
 intialisependinglist();
 monitorsocket(); 
}

/*This function will intialise the pending packets to NULL value*/
void intialisependinglist()
{
 int i;
 for(i=0;i<MAXHOSTS;i++)
 {
  pendingPkts[i]=NULL;
 }
}

/*This function intialises the arp cache to null value*/
void intialisearpcache()
{
 int i;
 for(i=0;i<MAXHOSTS;i++)
 {
  arpCache[i]=NULL;
 }
}

/*Function for reading and storing host file*/
void readhostfile(char * fileName)
{
 FILE* file;
 char * line=NULL;
 int i;
 int j=0;
 size_t len=0;
 ssize_t read;
 char *string[256];
 char delimit[]=" \t\r\n\v\f";
 struct in_addr addr;
 file = fopen(fileName, "r");
 if (file == NULL)
 {
  printf("Unable to open host file.Exiting application\n");
  exit(EXIT_FAILURE);
 }
 while ((read = getline(&line, &len, file)) != -1)
 {
   /*printf("Retrieved line of length %zu :\n", read);*/
   /*printf("%s", line);*/
   i=0;
   string[i]=strtok(line,delimit);
   strncpy(host[j].name,string[i],strlen(string[i])+1);
   i++;
   string[i]=strtok(NULL,delimit);
   inet_aton(string[i],&addr);
   host[j].addr=addr.s_addr;
   j++;
 }
 hostcnt=j;
 /*Debug Information*/
 /*Print information related to hosts*/
 /*for(i=0;i<j;i++)
 {
  addr.s_addr=host[i].addr;
  printf("%s, %s\n",host[i].name,inet_ntoa(addr));
 }
 printf("host count:%d\n",hostcnt);*/
 fclose(file);
 if (line!=NULL)
 {
  free(line);
 }

}

/*Function for reading and storing the interface information*/
void readifacefile(char* fileName)
{  
 FILE* file;
 char * line=NULL;
 int i;
 int j=0;
 size_t len=0;
 ssize_t read;
 char *string[256];
 char delimit[]=" \t\r\n\v\f";
 struct in_addr addr;
 file = fopen(fileName, "r");
 if (file == NULL)
 {
  printf("Unable to open host file.Exiting application\n");
  exit(EXIT_FAILURE);
 }
 while ((read = getline(&line, &len, file)) != -1)
 {  
   i=0;
   string[i]=strtok(line,delimit);
   strncpy(ifaces[j].ifacename,string[i],strlen(string[i])+1);
   i++;
   string[i]=strtok(NULL,delimit);
   inet_aton(string[i],&addr);
   ifaces[j].ipaddr=addr.s_addr;
   i++;
   string[i]=strtok(NULL,delimit);
   inet_aton(string[i],&addr);
   ifaces[j].netmask=addr.s_addr;
   i++;
   string[i]=strtok(NULL,delimit);
   strncpy((char *)ifaces[j].macaddr,string[i],strlen(string[i])+1);
   i++;
   string[i]=strtok(NULL,delimit);
   strncpy(ifaces[j].lanname,string[i],strlen(string[i])+1);
   j++;
 }
 ifacecnt=j;
 /*Debug Information*/
 /*Print information related to ifaces*/
 /*for(i=0;i<j;i++)
 {
  addr.s_addr=ifaces[i].ipaddr;
  fprintf(stderr,"%s, %s,",ifaces[i].ifacename,inet_ntoa(addr));
  addr.s_addr=ifaces[i].netmask;
  fprintf(stderr,"%s, %s, %s\n",inet_ntoa(addr),ifaces[i].macaddr,ifaces[i].lanname);
 }
 fprintf(stderr,"Interface count is:%d\n",ifacecnt);*/
}

/*This function will display the interface information*/
void displayinterfaceinfo()
{
 int i;
 struct in_addr addr;
 fprintf(stderr,"Interface Name    ");
 fprintf(stderr,"IP Address        ");
 fprintf(stderr,"Network Mask      ");
 fprintf(stderr,"Mac Address        ");
 fprintf(stderr,"LAN Name\n");
 for(i=0;i<ifacecnt;i++)
 {
  addr.s_addr=ifaces[i].ipaddr;
  fprintf(stderr,"%s              %s        ",ifaces[i].ifacename,inet_ntoa(addr));
  addr.s_addr=ifaces[i].netmask;
  fprintf(stderr,"%s       %s          %s\n",inet_ntoa(addr),ifaces[i].macaddr,ifaces[i].lanname); 
 }
}

/*This function will connect to all the LANS*/
void connectLan()
{
 char addressName[100]; 
 char portName[100];
 char server[20];
 char port[10];
 char buf[10];
 struct addrinfo hints,*res,*ressave;
 int i;
 int sockfd;
 int flag;
 int no_attempts=0;
 int skt_block_status;
 int num;
 int rv;
 bzero(&hints, sizeof(struct addrinfo));
 hints.ai_family=AF_UNSPEC;
 hints.ai_socktype=SOCK_STREAM;
 FD_ZERO(&allset);
 for(i=0;i<ifacecnt;i++)
 {
  /*Debug Information*/
  /*fprintf(stderr,"Trying to connect to the LAN\n");*/
  sprintf(addressName,".%s.addr",ifaces[i].lanname);
  sprintf(portName,".%s.port",ifaces[i].lanname);
  if (readlink(addressName,server,sizeof(server))<0)
  {
   printf("Unable to read address file.Exiting application\n");
   exit(0);
  }
  if (readlink(portName,port,sizeof(port))<0)
  {
   printf("Unable to read port file.Exiting application\n");
   exit(0);
  }
  if ((rv=getaddrinfo(server,port,&hints,&res))!=0)
  {
   fprintf(stderr,"getaddrinfo:%s\n",gai_strerror(rv));
  }

  ressave = res;
  flag = 0;
  do
  {
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
     continue;
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
    {
      /*Debug Information*/
      /*fprintf(stderr,"Connected to lan %s\n",ifaces[i].lanname);*/
      flag = 1;
      break;
    }
    close(sockfd);
  } while ((res = res->ai_next) != NULL);
  freeaddrinfo(ressave);
  if (flag == 0)
  {
    /*Debug Information*/
    /*fprintf(stderr, "cannot connect to lan name %s\n",ifaces[i].lanname);*/
   /* exit(1);*/
  }
  else
  {
   while(no_attempts<5)
   {
    no_attempts++;
    /*Debug Information*/
    /*fprintf(stderr,"Trying to read the connect message\n");*/
    skt_block_status=fcntl(sockfd,F_GETFL,0);
    /*fprintf(stderr,"Present socket block status is :%d\n",skt_block_status);*/
    fcntl(sockfd,F_SETFL,skt_block_status|O_NONBLOCK);
    /*fprintf(stderr,"Present socket block status after non blocking is:%d\n",fcntl(sockfd,F_GETFL,0));*/
    num=read(sockfd,buf,sizeof(buf));
    if (num>0)
    {
     if (strcmp(buf,"accept")==0)
     {
      FD_SET(sockfd,&allset);
      if (sockfd>maxfd)
      {
       maxfd=sockfd;
      }
      strncpy(link_socket[i].ifacename,ifaces[i].lanname,strlen(ifaces[i].lanname)+1);
      link_socket[i].sockfd=sockfd;
      /*fprintf(stderr,"Added to interface to socket link\n");*/
      fcntl(sockfd,F_SETFL,skt_block_status);
      /*fprintf(stderr,"Present socket block status after setting to  blocking is:%d\n",fcntl(sockfd,F_GETFL,0));*/
      break;
     }
    }
    else
    {
     fcntl(sockfd,F_SETFL,skt_block_status);
     sleep(2);
    }
   }
  }
 }

 /*Debug Information*/
 /*fprintf(stderr,"The conetents of Interface to link\n");
 for(i=0;i<ifacecnt;i++)
 {
  fprintf(stderr,"Lan name:%s , socket fd:%d\n",link_socket[i].ifacename,link_socket[i].sockfd);
 }*/
}

/*Function for reading and storing routing table information from routing file*/
void readrtablefile(char* fileName)
{ 
 FILE* file;
 char * line=NULL;
 int i;
 int j=0;
 size_t len=0;
 ssize_t read;
 char *string[256];
 char delimit[]=" \t\r\n\v\f";
 struct in_addr addr;
 file = fopen(fileName, "r");
 if (file == NULL)
 {
  printf("Unable to open host file.Exiting application\n");
  exit(EXIT_FAILURE);
 }
 while ((read = getline(&line, &len, file)) != -1)
 {  
   i=0;
   string[i]=strtok(line,delimit);
   inet_aton(string[i],&addr);
   rt_table[j].destsubnet=addr.s_addr;
   i++;
   string[i]=strtok(NULL,delimit);
   inet_aton(string[i],&addr);
   rt_table[j].nexthop=addr.s_addr;
   i++;
   string[i]=strtok(NULL,delimit);
   inet_aton(string[i],&addr);
   rt_table[j].mask=addr.s_addr;
   i++;
   string[i]=strtok(NULL,delimit);
   strncpy(rt_table[j].ifacename,string[i],strlen(string[i])+1);
   j++;
 }
 rt_cnt=j;
 /*Debug Information*/
 /*Print information related to routing table*/
 /*for(i=0;i<j;i++)
 {
  addr.s_addr=rt_table[i].destsubnet;
  fprintf(stderr,"%s,",inet_ntoa(addr));
  addr.s_addr=rt_table[i].nexthop;
  fprintf(stderr,"%s,",inet_ntoa(addr));
  addr.s_addr=rt_table[i].mask;
  fprintf(stderr,"%s,%s\n",inet_ntoa(addr),rt_table[i].ifacename);
 }
 fprintf(stderr,"Routing count is:%d\n",rt_cnt);*/
}

/*This is the main function that will keep monitoring the socket*/
void monitorsocket()
{
 fd_set resset;
 char* read_line;
 char** command_arguments;
 char* read_line_cp;
 char* message;
 int i;
 fprintf(stderr,"station:");
 FD_SET(STDIN_FILENO,&allset);
 for(;;)
 {
  resset=allset;
  select(maxfd+1,&resset,NULL,NULL,NULL);
  /*If stdin is active*/
   if (FD_ISSET(STDIN_FILENO,&resset))
   {
    /*Every time make it Null for each line in command prompt*/
     read_line=NULL;
     command_arguments=NULL;
     message=NULL;
     read_line=read_command();
     if (read_line!=NULL)
     {
      read_line_cp=malloc(125);
      strncpy(read_line_cp,read_line,strlen(read_line)+1);
      command_arguments=get_commandarguments(read_line);
      if (strcmp(command_arguments[0],"send")==0)
      {
        message=getMessage(read_line_cp);
	/*Debug Information*/
	/*fprintf(stderr,"The message trying to send:%s\n",message);*/
        sendpacket(command_arguments[1],message);
	fprintf(stderr,"station:");
      }
      else if (strcmp(command_arguments[0],"route")==0)
      {
       displayroutingtable();
       fprintf(stderr,"station:");
      }
      else if (strcmp(command_arguments[0],"interface")==0)
      {
       displayinterfaceinfo();
       fprintf(stderr,"station:");
      }
      else if (strcmp(command_arguments[0],"arp")==0)
      {
       displayarpcache();
       fprintf(stderr,"station:");
      }
     }
     /*Debug Information*/
     /*fprintf(stderr,"Trying to free the memory for command line\n");*/
     if (read_line!=NULL)
     {
      free(read_line);
      free(command_arguments);
      free(read_line_cp);
     }
     /*fprintf(stderr,"Able to free the memory for command line\n");*/
   }

   /*If any of the socket is active read from it*/
   for(i=0;i<ifacecnt;i++)
   {
    if (FD_ISSET(link_socket[i].sockfd,&resset))
    {
     readsocket(link_socket[i].sockfd);
    }
   }
 }

}

/*This function will display the routing table*/
void displayroutingtable()
{
 int i;
 struct in_addr addr;
 fprintf(stderr,"Destination Subnetwork        ");
 fprintf(stderr,"Next Hop        ");
 fprintf(stderr,"Mask              ");
 fprintf(stderr,"Interface Name\n");
 for(i=0;i<rt_cnt;i++)
 {
  addr.s_addr=rt_table[i].destsubnet;
  fprintf(stderr,"%s                     ",inet_ntoa(addr));
  addr.s_addr=rt_table[i].nexthop;
  fprintf(stderr,"%s         ",inet_ntoa(addr));
  addr.s_addr=rt_table[i].mask;
  fprintf(stderr,"%s           %s\n",inet_ntoa(addr),rt_table[i].ifacename); 
 }
}

/*This function will get the ip address corresponding to the host name*/
unsigned long getIpAddressofStation(char * hostName)
{
 int i;
 for(i=0;i<hostcnt;i++)
 {
  if (strcmp(host[i].name,hostName)==0)
  {
   /*Debug Information*/
   /*fprintf(stderr,"Found Ip address\n");*/
   return host[i].addr;
  }
 }
 return 0;
}

/*This function will send the packet to the other station*/
void sendpacket(char* hostName,char * message)
{
 int index;
 int sockfd;
 int index_arpcache;
 struct in_addr addr;
 MacAddr nexthop_mac;
 Rtable nextdest;
 IP_PKT ippkt;
 PEND_PKT* pendpkt;
 IPAddr destIpAddr=getIpAddressofStation(hostName);
 MacAddr dest_mac="11111111111111111";
 if (destIpAddr==0)
 {
  fprintf(stderr,"%s does not exist\n",hostName);
  return;
 }
 nextdest=getIpAddressOfNextHop(destIpAddr);
 index=getIndexOftheIface(nextdest);
 sockfd=link_socket[index].sockfd;
 /*Debug Information*/
 /*fprintf(stderr,"Trying to send to socket:%d\n",sockfd);*/
 inet_aton("0.0.0.0",&addr);
 if (addr.s_addr==nextdest.nexthop)
 {
  /*Debug Information*/
  /*fprintf(stderr,"The destination exist within the same Lan\n");*/
  if ((index_arpcache=getMacfromarpcache(destIpAddr))>-1)
  {
   strncpy((char *)nexthop_mac,(char *)arpCache[index_arpcache]->macaddr,18);
  }
  else
  {
   /*Debug Information*/
   /*fprintf(stderr,"Sending ARP Request packet\n");*/
   pendpkt=malloc(sizeof(PEND_PKT));
   sendetherframe(NULL,destIpAddr,ifaces[index].ipaddr,dest_mac,ifaces[index].macaddr,sockfd,TYPE_ARP_PKT,ARP_REQUEST);
   strncpy(ippkt.data,message,strlen(message)+1);
   ippkt.dstip=destIpAddr;
   ippkt.srcip=ifaces[index].ipaddr;
   ippkt.length=htons(strlen(message)+1);
   pendpkt->packet=ippkt;
   pendpkt->destination=destIpAddr;
   storependingpkt(pendpkt);
   /*Debug Information*/
   /*fprintf(stderr,"Able to store pending packets\n");*/
   return;
  }
 }
 else
 {
  if ((index_arpcache=getMacfromarpcache(nextdest.nexthop))>-1)
  {
   strncpy((char *)nexthop_mac,(char *)arpCache[index_arpcache]->macaddr,18);
  }
  else
  {
   /*Send arp pkt with router ip as destination*/
   /*Debug Information*/
   /*fprintf(stderr,"Sending ARP Request packet\n");*/
   pendpkt=malloc(sizeof(PEND_PKT));
   sendetherframe(NULL,nextdest.nexthop,ifaces[index].ipaddr,dest_mac,ifaces[index].macaddr,sockfd,TYPE_ARP_PKT,ARP_REQUEST);
   strncpy(ippkt.data,message,strlen(message)+1);
   ippkt.dstip=destIpAddr;
   ippkt.srcip=ifaces[index].ipaddr;
   ippkt.length=htons(strlen(message)+1);
   pendpkt->packet=ippkt;
   pendpkt->destination=nextdest.nexthop;
   storependingpkt(pendpkt);
   /*Debug Information*/
   /*fprintf(stderr,"Able to store pending packets\n");*/
   return;
  }
 }

 sendetherframe(message,destIpAddr,ifaces[index].ipaddr,nexthop_mac,ifaces[index].macaddr,sockfd,TYPE_IP_PKT,3);
}

/*This function stores the pending packet in Pending packet queue*/
void storependingpkt(PEND_PKT * pend_pkt)
{
 int i;
 for(i=0;i<MAXHOSTS;i++)
 {
  if (pendingPkts[i]==NULL)
  {
   pendingPkts[i]=pend_pkt;
   return;
  }
 }
}

/*Function to get the index of the iterface to which we need to send the data*/
int getIndexOftheIface(Rtable nextdest)
{
 int i;
 for(i=0;i<ifacecnt;i++)
 {
  if (strcmp(nextdest.ifacename,ifaces[i].ifacename)==0)
  {
   /*Debug Information*/
   /*fprintf(stderr,"Found the interface to forward to\n");*/
   return i;
  }
 }
 return -1;
}

/*Get the socket descriptor*/
int getSocketFd(char * lanname)
{
 int i;
 for(i=0;i<ifacecnt;i++)
 {
  if (strcmp(lanname,link_socket[i].ifacename)==0)
  {
   /*Debug Information*/
   /*fprintf(stderr,"Found socket descriptor to forward to\n");*/
   return link_socket[i].sockfd;
  }
 }
 return -1;
}

/*This function will read from the socket*/
void readsocket(int sockfd)
{
 short type;
 short length;
 EtherPkt etherpkt;
 IP_PKT ippkt;
 ARP_PKT arppkt;
 int num;
 int index_pendpkt;
 int index_sock;
 int index_mac;
 /*Debug Information*/
 /*fprintf(stderr,"Started reading from the socket\n");*/
 num=read(sockfd,&etherpkt.dst,18);
 if (num==0)
 {
  fprintf(stderr,"Connected bridge closed.\n");
  FD_CLR(sockfd,&allset);
  if (isStation==1)
  {
   fprintf(stderr,"Closing the station\n");
  }
  else
  {
   fprintf(stderr,"Closing the router\n");
  }
  exit(0);
 }
 /*Debug Information*/
 /*fprintf(stderr,"Dest Mac:%s\n",etherpkt.dst);*/
 read(sockfd,&etherpkt.src,18);
 /*fprintf(stderr,"Src Mac:%s\n",etherpkt.src);*/
 read(sockfd,(char *)&etherpkt.type,sizeof(short));
 type=ntohs(etherpkt.type);
 /*fprintf(stderr,"Type:%d\n",type);*/
 read(sockfd,(char *)&etherpkt.size,sizeof(short));
 if (type==TYPE_IP_PKT)
 {
  read(sockfd,(char *)&ippkt.dstip,sizeof(IPAddr));
  read(sockfd,(char *)&ippkt.srcip,sizeof(IPAddr));
  read(sockfd,(char *)&ippkt.protocol,sizeof(short));
  read(sockfd,(char *)&ippkt.sequenceno,sizeof(unsigned long));
  read(sockfd,(char *)&ippkt.length,sizeof(length));
  length=ntohs(ippkt.length);
  read(sockfd,ippkt.data,length);
  /*If the packet received is Ip packet and the station is of station type and the destination Ip is its Ip address display the message*/
  if (isStation==1)
  {
   /*fprintf(stderr,"Trying to check whether the packet is meant for the station\n");*/
   if (isPacketForStation(ippkt.dstip))
   {
    /*fprintf(stderr,"The IP packet is meant for the station\n");*/
    fprintf(stderr,"%s\n",ippkt.data);
    fprintf(stderr,"station:");
   }
  }
  else
  {
   if (isPacketMeantforRouter(etherpkt.dst))
   {
    routeippkt(etherpkt,ippkt);
   }
  }
 }
 else if (type==TYPE_ARP_PKT)
 {
  read(sockfd,(char *)&arppkt.op,sizeof(short));
  read(sockfd,(char *)&arppkt.srcip,sizeof(IPAddr));
  read(sockfd,arppkt.srcmac,18);
  read(sockfd,(char *)&arppkt.dstip,sizeof(IPAddr));
  read(sockfd,arppkt.dstmac,18);
  if (arppkt.op==ARP_REQUEST)
  {   
    /*If ARP request is meant for station send ARP reply*/
    if (isPacketForStation(arppkt.dstip))
    {
      /*add to the arp cache the source ip and mac address if absent in arp cache*/
      if (getMacfromarpcache(arppkt.srcip)==-1)
      {
       /*fprintf(stderr,"Adding to the ARP cache\n");*/
       addtoarpcache(arppkt.srcip,arppkt.srcmac);
      }
      /*fprintf(stderr,"The arp packet is meant for the station\n");*/
     /*send arp reply*/
     sendArpReply(sockfd,arppkt);
    }
   }
   else if (arppkt.op==ARP_RESPONSE)
   {
    if (isPacketForStation(arppkt.srcip))
    {
     addtoarpcache(arppkt.dstip,arppkt.dstmac);
     /*check if there is any pending packet with destionation IP*/
     index_pendpkt=getIndexOfPendingPkts(arppkt.dstip);
     /*If present resend the packet*/
     if (index_pendpkt>-1)
     {
      /*fprintf(stderr,"Trying to send the pending packet\n");*/
      index_sock=getIndexOfSocket(sockfd);
      index_mac=getMacAddressFromLanname(link_socket[index_sock].ifacename);
      /*fprintf(stderr,"Index for src Mac address %d\n",index_mac);*/
      sendetherframe(pendingPkts[index_pendpkt]->packet.data,pendingPkts[index_pendpkt]->packet.dstip,pendingPkts[index_pendpkt]->packet.srcip,arppkt.dstmac,ifaces[index_mac].macaddr,sockfd,TYPE_IP_PKT,3);
      /*fprintf(stderr,"Trying to free the pending packet queue\n");*/
      free(pendingPkts[index_pendpkt]);
      /*fprintf(stderr,"Able to free pending packet\n");*/
     }
    }
    else
    {
     fprintf(stderr,"Arp response packet not meant for station which should not happen\n");
    }
   }
  } 
}

/*This function will get the index of the interface link which has that socket descriptor*/
int getIndexOfSocket(int sockfd)
{
 int i;
 for(i=0;i<ifacecnt;i++)
 {
   if (link_socket[i].sockfd==sockfd)
   {
    return i;
   }
 }
 return -1;
}

/*This function will get the mac address index from the interface which has corresponding lan name*/
int getMacAddressFromLanname(char * lanName)
{
 int i;
 for(i=0;i<ifacecnt;i++)
 {
  if (strcmp(ifaces[i].lanname,lanName)==0)
  {
   return i;
  }
 }
 return -1;
}

/*This function will find out the index of the pending packets*/
int getIndexOfPendingPkts(IPAddr dest)
{
 int i;
 /*Debug Information*/
 /*fprintf(stderr,"Trying to find the index of the pending pkt\n");*/
 for(i=0;i<MAXHOSTS;i++)
 {
  if (pendingPkts[i]!=NULL)
  {
   if (pendingPkts[i]->destination==dest)
   {
     /*Debug Information*/
     /*fprintf(stderr,"Found the pending packet needed to be send\n");*/
     return i;
   }
  }
 }
 
 fprintf(stderr,"Did not find any pending packet something must be wrong\n");
 return -1;
}

/*This function will send the arp reply*/
void sendArpReply(int sockfd,ARP_PKT pkt)
{
 int index_iface;
 short size;
 short pkt_type=TYPE_ARP_PKT;
 pkt.op=ARP_RESPONSE;
 /*Debug Information*/
 /*fprintf(stderr,"Trying to send ARP reply\n");*/
 index_iface=getMacAddressIndex(pkt.dstip);
 if (index_iface>-1)
 {
  strncpy((char *)pkt.dstmac,(char *)ifaces[index_iface].macaddr,18);
  write(sockfd,pkt.srcmac,18);
  write(sockfd,ifaces[index_iface].macaddr,18);
  pkt_type=htons(pkt_type);
  write(sockfd,(char *)&pkt_type,sizeof(short));
  size=(sizeof(IPAddr)*2)+36+sizeof(short);
  write(sockfd,(char *)&size,sizeof(short));
  write(sockfd,(char *)&pkt.op,sizeof(short));
  write(sockfd,(char *)&pkt.srcip,sizeof(IPAddr));
  write(sockfd,pkt.srcmac,18);
  write(sockfd,(char *)&pkt.dstip,sizeof(IPAddr));
  write(sockfd,pkt.dstmac,18);
 }
 /*Debug Information*/
 /*fprintf(stderr,"Able to send ARP reply\n");*/
}

/*This function will get the index of the interface which contains the corresponding IP address*/
int getMacAddressIndex(IPAddr src)
{
 int i;
 for(i=0;i<ifacecnt;i++)
 {
  if (ifaces[i].ipaddr==src)
  {
   return i;
  }
 }
 return -1;
}

/*This function will add entry into ARP cache*/
void addtoarpcache(IPAddr destIP,MacAddr destMac)
{
 int i;
 Arpc* entry=malloc(sizeof(Arpc));
 entry->ipaddr=destIP;
 strncpy((char *)entry->macaddr,(char *)destMac,18);
 for(i=0;i<MAXHOSTS;i++)
 {
  if (arpCache[i]==NULL)
  {
   arpCache[i]=entry;
   return;
  }
 }
}

/*This function will display the arp cahe information*/
void displayarpcache()
{
 int i;
 struct in_addr addr;
 fprintf(stderr,"IP Address                    ");
 fprintf(stderr,"Mac Address\n");
 for(i=0;i<MAXHOSTS;i++)
 {
  if (arpCache[i]!=NULL)
  {
    addr.s_addr=arpCache[i]->ipaddr;
    fprintf(stderr,"%s               ",inet_ntoa(addr));
    fprintf(stderr,"%s\n",arpCache[i]->macaddr);
  }
 }
}

/*This function will return the index of the arp cache where there is entry with the corresponding mac address to destination IP address mapping.Otherwise it will 
 * return -1 if the mapping is absent*/
int getMacfromarpcache(IPAddr dest)
{
 int i;
 /*Debug Information*/
 /*fprintf(stderr,"Trying to find ip address if present in arp cache\n");*/
 for(i=0;i<MAXHOSTS;i++)
 {
  if (arpCache[i]!=NULL)
  {
   if (arpCache[i]->ipaddr==dest)
   {
    /*fprintf(stderr,"Present in the arp cache\n");*/
    return i;
   }
  }
 }
 /*fprintf(stderr,"Absent in arp cache\n");*/
 return -1;
}

/*This function will route the packet on the basis of the destination ip*/
void routeippkt(EtherPkt etherpkt,IP_PKT ippkt)
{
 MacAddr nexthop_mac;
 int index;
 int index_arpcache;
 int sockfd;
 MacAddr dest_mac="11111111111111111";
 struct in_addr addr;
 PEND_PKT* pendpkt;
 /*Debug Information*/
 /*fprintf(stderr,"I am a router I should forward the ip packet I received\n");*/

 Rtable nextdest=getIpAddressOfNextHop(ippkt.dstip); 
 index=getIndexOftheIface(nextdest);
 sockfd=link_socket[index].sockfd;
 inet_aton("0.0.0.0",&addr); 
 if (addr.s_addr==nextdest.nexthop)
 {
  /*Debug Information*/
  /*fprintf(stderr,"The destination exist within the same Lan\n");*/
  /*If Ip address present in the same LAN*/
  if ((index_arpcache=getMacfromarpcache(ippkt.dstip))>-1)
  {
   strncpy((char *)nexthop_mac,(char *)arpCache[index_arpcache]->macaddr,18);
  }
  else
  {
   /*Debug Information*/
   /*fprintf(stderr,"Sending ARP Request packet\n");*/
   pendpkt=malloc(sizeof(PEND_PKT));
   sendetherframe(NULL,ippkt.dstip,ifaces[index].ipaddr,dest_mac,ifaces[index].macaddr,sockfd,TYPE_ARP_PKT,ARP_REQUEST);
   pendpkt->packet=ippkt;
   pendpkt->destination=ippkt.dstip;
   storependingpkt(pendpkt);
   /*fprintf(stderr,"Able to store pending packets\n");*/
   return;
  }
  
 }
 else
 {
  if ((index_arpcache=getMacfromarpcache(nextdest.nexthop))>-1)
  {
   strncpy((char *)nexthop_mac,(char *)arpCache[index_arpcache]->macaddr,18);
  }
  else
  {
  /*Send arp pkt with router ip as destination*/
   /*fprintf(stderr,"Sending ARP Request packet\n");*/
   pendpkt=malloc(sizeof(PEND_PKT));
   sendetherframe(NULL,nextdest.nexthop,ifaces[index].ipaddr,dest_mac,ifaces[index].macaddr,sockfd,TYPE_ARP_PKT,ARP_REQUEST);
   pendpkt->packet=ippkt;
   pendpkt->destination=nextdest.nexthop;
   storependingpkt(pendpkt);
   /*fprintf(stderr,"Able to store pending packets\n");*/
   return;
  }
 }



 /*fprintf(stderr,"Trying to send to socket:%d\n",sockfd);*/
 sendetherframe(ippkt.data,ippkt.dstip,ippkt.srcip,nexthop_mac,ifaces[index].macaddr,sockfd,TYPE_IP_PKT,3);


}

/*This function will check whether the Ip address is meant for the station.Return 1 if true else 0*/
int isPacketForStation(IPAddr destIp)
{
 int i;
 for(i=0;i<ifacecnt;i++)
 {
  if (ifaces[i].ipaddr==destIp)
  {
   return 1;
  }
 }
 return 0;
}

/*This function will try to find out whether the IP packet is meant for the router.Return 1 if true else 0*/
int isPacketMeantforRouter(MacAddr dest)
{
  int i;
  for(i=0;i<ifacecnt;i++)
  {
   if (strcmp((char *)ifaces[i].macaddr,(char *)dest)==0)
   {
    /*Debug Information*/
    /*fprintf(stderr,"The Ip packet is meant for the router.So will forward it\n");*/
    return 1;
   }
  }
 return 0;
}


/*This function will send the ethernet frame*/
void sendetherframe(char * message,IPAddr destIp,IPAddr srcIp,MacAddr destMac,MacAddr srcMac,int sockfd,short typeOfFrame,short arp_type)
{
 int size;
 short packet_type;
 short length;
 short protocol=0;
 unsigned long seqNo=0; 
 if (typeOfFrame==TYPE_IP_PKT)
 {
  length=strlen(message)+1;
  size=(sizeof(IPAddr)*2)+(sizeof(short)*2)+sizeof(unsigned long)+length;
  length=htons(length);
 }
 else if (typeOfFrame==TYPE_ARP_PKT)
 {
  size=(sizeof(IPAddr)*2)+(sizeof(short))+(sizeof(MacAddr)*2);
 }
 size=htons(size); 
 packet_type=typeOfFrame;
 packet_type=htons(packet_type);
 /*Debug Information*/
 /*fprintf(stderr,"Trying to send ethernet frame\n");*/
 write(sockfd,destMac,18);
 write(sockfd,srcMac,18);
 write(sockfd,(char *)&packet_type,sizeof(short));
 write(sockfd,(char *)&size,sizeof(short));
 if (typeOfFrame==TYPE_IP_PKT)
 {
  /*fprintf(stderr,"Trying to send Ip packet\n");*/
  write(sockfd,(char *)&destIp,sizeof(destIp));
  write(sockfd,(char *)&srcIp,sizeof(srcIp));
  write(sockfd,(char *)&protocol,sizeof(protocol));
  write(sockfd,(char *)&seqNo,sizeof(seqNo));
  write(sockfd,(char *)&length,sizeof(length));
  write(sockfd,message,ntohs(length));
 }
 else if (typeOfFrame==TYPE_ARP_PKT)
 {
  /*fprintf(stderr,"Trying to send Arp packet\n");*/
  write(sockfd,(char *)&arp_type,sizeof(arp_type));
  write(sockfd,(char *)&srcIp,sizeof(srcIp));
  write(sockfd,srcMac,18);
  write(sockfd,(char *)&destIp,sizeof(destIp));
  write(sockfd,destMac,18);
 }
}

/*This function will get the message to be send from the arguments*/
char * getMessage(char * line)
{
 char* token;
 char* token_delimiter=" \t\r";
 char* message=malloc(sizeof(125));
 token=strtok(line,token_delimiter);
 token=strtok(NULL,token_delimiter);
 message=token+strlen(token)+1;
 return message;
}

/*This function will get which Ip address to forward the packet based on the destination Ip Address of the packet*/
Rtable getIpAddressOfNextHop(unsigned long destIpAddr)
{
 int i;
 unsigned long resultIpaddress;
 for(i=0;i<rt_cnt;i++)
 {
  resultIpaddress=destIpAddr & rt_table[i].mask;
  if (resultIpaddress==rt_table[i].destsubnet)
  {
   /*Debug Information*/
   /*fprintf(stderr,"Found the Ip address to foward to\n");*/
   return rt_table[i];
  }
 }
 /*fprintf(stderr,"Did not find the Ip address to forward to\n");*/
 return rt_table[i]; 
}

/*This function will read the command that has been entered by the user*/
char* read_command()
{
 int buffersize=125;
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
 }while(position<124);
 if (position==124)
 {
  buffer[position]='\0';
 }
 return buffer;
}

/*This function will parse the command line into arguments*/
char** get_commandarguments(char* commandline)
{
 int buffer_size=125;
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

