// lwmaster.c
//
// Print the IPv4 address of the Livewire master node
//
//   (C) Copyright 2020 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <errno.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwmaster.h"

uint32_t global_master_mcast_address=0;

int GetAddress(int dl_sock)
{
  struct timeval tv;
  double deadline;
  double now;
  int timeout=TIMEOUT_INTERVAL;
  struct pollfd pfd[1];
  char data[1501];
  ssize_t n;
  uint32_t dst_addr=0;
  struct in_addr node_addr;
  char node_str[INET_ADDRSTRLEN];

  //
  // Calculate the Deadline
  //
  memset(&tv,0,sizeof(tv));
  if(gettimeofday(&tv,NULL)!=0) {
    fprintf(stderr,"lwmaster: unable to get system time [%s]\n",
	    strerror(errno));
    exit(1);
  }
  deadline=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0+
    (double)TIMEOUT_INTERVAL/1000;

  //
  // Main Loop
  //
  while(timeout>0) {
    //
    // Wait for input
    //
    memset(&pfd[0],0,sizeof(struct pollfd));
    pfd[0].fd=dl_sock;
    pfd[0].events=POLLIN;
    switch(poll(pfd,1,timeout)) {
    case -1:  // Error
      fprintf(stderr,"lwmaster: poll(3) returned error [%s]\n",strerror(errno));
      return 1;

    case 0:   // Timed out
      printf("0.0.0.0\n");
      return 0;
    }

    //
    // Process Input
    //
    if((pfd[0].revents&POLLIN)!=0) {
      if((n=recv(dl_sock,data,1500,0))>=0) {
	if(n==MASTER_MCAST_PACKET_LENGTH) {
	  dst_addr=((0xff&data[30])<<24)+((0xff&data[31])<<16)+
	    ((0xff&data[32])<<8)+(0xff&data[33]);
	  if(dst_addr==global_master_mcast_address) {
	    memset(&node_addr,0,sizeof(node_addr));
	    node_addr.s_addr=((0xff&data[29])<<24)+  // Network byte order!
	      ((0xff&data[28])<<16)+
	      ((0xff&data[27])<<8)+
	      (0xff&data[26]);
	    memset(node_str,0,INET_ADDRSTRLEN);
	    if(inet_ntop(AF_INET,&node_addr,node_str,INET_ADDRSTRLEN)==NULL) {
	      fprintf(stderr,"lwmaster: error converting node address [%s]\n",
		      strerror(errno));
	      return 1;
	    }
	    printf("%s\n",node_str);
	    return 0;
	  }
	}
      }
      else {
	fprintf(stderr,"lwmaster: error reading datalink [%s]\n",
		strerror(errno));
	return 1;
      }
      
    }

    //
    // Update timeout value
    //
    memset(&tv,0,sizeof(tv));
    if(gettimeofday(&tv,NULL)!=0) {
      fprintf(stderr,"lwmaster: unable to get system time [%s]\n",
	      strerror(errno));
      exit(1);
    }
    now=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
    timeout=(int)(1000.0*(deadline-now));
  }

  printf("0.0.0.0\n");
  return 0;
}


void SubscribeInterface(int dl_sock,int ip_sock,int index)
{
  struct ip_mreqn mreq;
  struct packet_mreq preq;

  //
  // IP Subscribe
  //
  memset(&mreq,0,sizeof(mreq));
  mreq.imr_multiaddr.s_addr=htonl(global_master_mcast_address);
  mreq.imr_ifindex=index;
  if(setsockopt(ip_sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq))<0) {
    fprintf(stderr,"lwmaster: unable to subscribe to \"%s\" [%s]\n",
	    MASTER_MCAST_ADDRESS,strerror(errno));
    exit(1);
  }

  //
  // Set Promiscuous Mode
  //
  memset(&preq,0,sizeof(preq));
  preq.mr_ifindex=index;
  preq.mr_type=PACKET_MR_PROMISC;
  if(setsockopt(dl_sock,SOL_PACKET,PACKET_ADD_MEMBERSHIP,&preq,
		sizeof(preq))<0) {
    fprintf(stderr,"lwmaster: unable to set promiscuous mode [%s]\n",
	    strerror(errno));
    exit(1);
  }
}


void Subscribe(int dl_sock)
{
  int ip_sock;
  struct ifreq ifr;

  if((ip_sock=socket(AF_INET,SOCK_DGRAM,0))<0) {
    fprintf(stderr,"lwmaster: unable to open subscription socket [%s]\n",
	    strerror(errno));
    exit(1);
  }

  memset(&ifr,0,sizeof(ifr));
  ifr.ifr_ifindex=1;
  while(ioctl(ip_sock,SIOCGIFNAME,&ifr)==0) {
    SubscribeInterface(dl_sock,ip_sock,ifr.ifr_ifindex);
    ifr.ifr_ifindex++;
  }
}


int main(int argc,char *argv[])
{
  int dl_sock=-1;
  struct in_addr addr;

  //
  // Initialize the Master Clock Multicast Address
  //
  if(inet_pton(AF_INET,MASTER_MCAST_ADDRESS,&addr)!=1) {
    fprintf(stderr,"lwmaster: invalid master clock multicast address [%s]\n",
	    strerror(errno));
    exit(1);
  }
  global_master_mcast_address=ntohl(addr.s_addr);

  //
  // Open the NetLink Interface
  //
  if((dl_sock=socket(AF_PACKET,SOCK_RAW,htons(ETH_P_IP)))<0) {
    fprintf(stderr,"lwmaster: unable to open netlink socket [%s]\n",
	    strerror(errno));
    exit(1);
  }

  //
  // Subscribe to the Master Clock
  //
  Subscribe(dl_sock);

  //
  // Look for Clock Packets
  //
  return GetAddress(dl_sock);
}
