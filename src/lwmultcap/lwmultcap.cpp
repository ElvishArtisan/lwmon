// lwmultcap.cpp
//
// Print multicast messages from a specified address and port
//
//   (C) Copyright 2020-2024 Fred Gleason <fredg@paravelsystems.com>
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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <unistd.h>
#include <stdint.h>

#include <QCoreApplication>
#include <QStringList>

#include "cmdswitch.h"
#include "lwmultcap.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  unsigned port=0;
  QString err_msg;
  bool ok=false;

  c_port=0;
  c_show_ruler=true;
  c_first_offset=-1;
  c_last_offset=-1;
  c_packet_limit=0;

  //
  // Process command-line switches
  //
  CmdSwitch *cmd=new CmdSwitch("lwmultcap",LWMULTCAP_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--filter-source-address") {
      QHostAddress addr;
      if(!addr.setAddress(cmd->value(i))) {
	fprintf(stderr,
		"lwmultcap: invalid \"--filter-source-address\" value\n");
	exit(1);
      }
      c_filter_source_addresses.push_back(addr);
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--filter-byte") {
      QStringList f0=cmd->value(i).split(":",QString::KeepEmptyParts);
      if(f0.size()!=2) {
	fprintf(stderr,"lwmultcap: invalid \"--filter-byte\" argument\n");
	exit(1);
      }
      unsigned offset=ReadIntegerArg(f0.at(0),&ok);
      if(!ok) {
	fprintf(stderr,"lwmultcap: invalid \"--filter-byte\" offset\n");
	exit(1);
      }
      unsigned val=ReadIntegerArg(f0.at(1),&ok);
      if((!ok)||(val>0xFF)) {
	fprintf(stderr,"lwmultcap: invalid \"--filter-byte\" value\n");
	exit(1);
      }
      c_filter_bytes[offset]=(char)val;
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--filter-string") {
      QStringList f0=cmd->value(i).split(":",QString::KeepEmptyParts);
      if(f0.size()<2) {
	fprintf(stderr,"lwmultcap: invalid \"--filter-string\" argument\n");
	exit(1);
      }
      unsigned offset=ReadIntegerArg(f0.at(0),&ok);
      if(!ok) {
	fprintf(stderr,"lwmultcap: invalid \"--filter-string\" offset\n");
	exit(1);
      }
      f0.removeFirst();
      c_filter_strings[offset]=f0.join(":");
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--first-offset") {
      c_first_offset=ReadIntegerArg(cmd->value(i),&ok);
      if(!ok) {
	fprintf(stderr,"lwmultcap: invalid \"--first-offset\" value\n");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--iface-address") {
      if(!c_iface_address.setAddress(cmd->value(i))) {
	fprintf(stderr,"lwmultcap: invalid interface address\n");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--last-offset") {
      c_last_offset=ReadIntegerArg(cmd->value(i),&ok);
      if(!ok) {
	fprintf(stderr,"lwmultcap: invalid \"--last-offset\" value\n");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--port") {
      port=ReadIntegerArg(cmd->value(i),&ok);
      if((!ok)||(port>0xFFFF)) {
	fprintf(stderr,"lwmultcap: invalid port value\n");
	exit(1);
      }
      c_port=port;
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--mcast-address") {
      if(!c_mcast_address.setAddress(cmd->value(i))) {
	fprintf(stderr,"lwmultcap: invalid multicast address\n");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--no-ruler") {
      c_show_ruler=false;
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--packet-limit") {
      c_packet_limit=ReadIntegerArg(cmd->value(i),&ok);
      if(!ok) {
	fprintf(stderr,"lwmultcap: invalid \"--packet_limit\" value\n");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--show-ruler") {
      c_show_ruler=true;
      cmd->setProcessed(i,true);
    }

    if(!cmd->processed(i)) {
      fprintf(stderr,"lwmultcap: unknown option \"%s\"\n",
	      cmd->key(i).toUtf8().constData());
      exit(1);
    }
  }

  //
  // Sanity Checks
  //
  if(c_iface_address.isNull()) {
    fprintf(stderr,"lwmultcap: you must specify \"--iface-address\"\n");
    exit(1);
  }
  if(c_mcast_address.isNull()) {
    fprintf(stderr,"lwmultcap: you must specify \"--mcast-address\"\n");
    exit(1);
  }
  if(c_port==0) {
    fprintf(stderr,"lwmultcap: you must specify \"--port\"\n");
    exit(1);
  }

  //
  // Receive Socket
  //
  int sock;
  if((sock=socket(AF_INET,SOCK_DGRAM,0))<0) {
    fprintf(stderr,"lwmultcap: unable to create socket [%s]\n",strerror(errno));
    exit(1);
  }

  //
  // So we can get the actual delivery address
  //
  unsigned optval=1;
  if(setsockopt(sock,IPPROTO_IP,IP_PKTINFO,&optval,sizeof(unsigned))!=0) {
    fprintf(stderr,"lwmultcap: unable to set IP_PKTINFO [%s] on socket\n",
	    strerror(errno));
    exit(1);
  }
  sockaddr_in sa;
  memset(&sa,0,sizeof(sa));
  sa.sin_port=htons(c_port);
  if(bind(sock,(struct sockaddr *)(&sa),sizeof(sa))<0) {
    fprintf(stderr,"lwmultcap: unable to bind socket [%s]\n",strerror(errno));
    exit(1);
  }
  if(!Subscribe(sock,c_mcast_address,c_iface_address,&err_msg)) {
    fprintf(stderr,"lwmultcap: unable to subscribe to %s [%s]\n",
	    c_mcast_address.toString().toUtf8().constData(),strerror(errno));
    exit(1);
  }

  MainLoop(sock);
}


void MainObject::MainLoop(int sock)
{
  QHostAddress dst_addr;
  uint16_t dst_port=0;
  QHostAddress src_addr;
  uint16_t src_port=0;
  ssize_t n;
  struct msghdr msg;
  memset(&msg,0,sizeof(msg));

  char name[sizeof(struct sockaddr_in)];
  memset(&name,0,sizeof(struct sockaddr_in));
  msg.msg_name=name;
  msg.msg_namelen=sizeof(struct sockaddr_in);
  
  char data[1500];
  struct iovec iov;
  memset(&iov,0,sizeof(iov));
  iov.iov_base=data;
  iov.iov_len=1500;
  msg.msg_iov=&iov;
  msg.msg_iovlen=1;

  char cmsgs[1024];
  memset(cmsgs,0,1024);
  msg.msg_control=cmsgs;
  msg.msg_controllen=1024;
  
  while((n=recvmsg(sock,&msg,0))>0) {
    if(msg.msg_flags!=0) {
      fprintf(stderr,"lwmultcap: error flags received!\n");
      exit(1);
    }
    struct sockaddr_in sa;
    memcpy(&sa,msg.msg_name,sizeof(sa));
    src_addr.setAddress(ntohl(sa.sin_addr.s_addr));
    src_port=ntohs(sa.sin_port);
    
    struct cmsghdr *cmsg;
    cmsg=CMSG_FIRSTHDR(&msg);
    while(cmsg!=NULL) {
      if(cmsg->cmsg_type==8) {
	struct in_pktinfo pktinfo;
	memcpy(&pktinfo,CMSG_DATA(cmsg),sizeof(pktinfo));
	dst_addr.setAddress((ntohl(pktinfo.ipi_addr.s_addr)));
      }
      cmsg=CMSG_NXTHDR(&msg,cmsg);
    }
    
    ProcessPacket(dst_addr,src_addr,src_port,QByteArray(data,n));
  }

  fprintf(stderr,"lwmultcap: socket error [%s]\n",strerror(errno));
  exit(1);
}


void MainObject::ProcessPacket(const QHostAddress &dst_addr,
				const QHostAddress &src_addr,uint16_t src_port,
				QByteArray data)
{
  bool match=false;

  //
  // Process Offsets
  //
  if(c_first_offset>0) {
    data=data.right(data.size()-c_first_offset);
  }
  if(c_last_offset>=0) {
    data=data.left(c_last_offset);
  }
  
  //
  // Process Filter Bytes
  //
  match=false;
  for(QMap<unsigned,char>::const_iterator it=c_filter_bytes.begin();
      it!=c_filter_bytes.end();it++) {
    if((it.key()<(unsigned)data.size())&&(it.value()==data.at(it.key()))) {
      match=true;
      break;
    }
  }
  if((c_filter_bytes.size()>0)&&(!match)) {
    return;
  }

  //
  // Process Filter Strings
  //
  match=false;
  for(QMap<unsigned,QString>::const_iterator it=c_filter_strings.begin();
      it!=c_filter_strings.end();it++) {

    if((it.key()<(unsigned)data.size())&&
       (it.value().toUtf8()==data.mid(it.key(),it.value().size()))) {
      match=true;
      break;
    }
  }
  if((c_filter_strings.size()>0)&&(!match)) {
    return;
  }

  //
  // Process Filter Addresses
  //
  match=false;
  for(int i=0;i<c_filter_source_addresses.size();i++) {
    if(c_filter_source_addresses.at(i)==src_addr) {
      match=true;
      break;
    }
  }
  if((c_filter_source_addresses.size()>0)&&(!match)) {
    return;
  }

  PrintPacket(dst_addr,src_addr,src_port,data);
  if(c_packet_limit>0) {
    if(--c_packet_limit==0) {
      exit(0);
    }
  }
}


void MainObject::PrintPacket(const QHostAddress &dst_addr,
			     const QHostAddress &src_addr,uint16_t src_port,
			     const QByteArray &data)
{
  QString recv_from_str=
    src_addr.toString()+QString::asprintf(":%d",0xFFFF&src_port);

  QString recv_dst_str=dst_addr.toString()+QString::asprintf(":%d",0xFFFF&c_port);

  QString size_str=QString::asprintf("0x%04X",data.size());

  if(c_show_ruler) {
    printf("------------------------------------------------------------------------------\n");
    printf("| To: %-21s    From: %-21s     size: %-7s |\n",
	   recv_dst_str.toUtf8().constData(),
	   recv_from_str.toUtf8().constData(),
	   size_str.toUtf8().constData());
    printf("------------------------------------------------------------------------------\n");
    printf("| Offset  0- 1- 2- 3- 4- 5- 6- 7- 8- 9- A- B- C- D- E- F- | 0123456789ABCDEF |\n");
    printf("----------------------------------------------------------|------------------|\n");
  }
  for(int i=0;i<data.size();i+=16) {
    QString str="";
    printf("| 0x%04X: ",i);
    for(int j=0;j<16;j++) {
      if((i+j)<data.size()) {
	char c=0xFF&data[i+j];
	printf("%02X ",0xFF&c);
	if((c>=' ')&&(c<='~')) {
	  str+=c;
	}
	else {
	  str+='.';
	}
      }
      else {
	printf("   ");
	str+=' ';
      }
    }
    printf("| %s |\n",str.toUtf8().constData());
  }
  if(c_show_ruler) {
    printf("------------------------------------------------------------------------------\n");
  }
}


bool MainObject::Subscribe(int sock,const QHostAddress &addr,
			   const QHostAddress &if_addr,QString *err_msg)
{
  struct ip_mreqn mreq;

  memset(&mreq,0,sizeof(mreq));
  mreq.imr_multiaddr.s_addr=htonl(addr.toIPv4Address());
  mreq.imr_address.s_addr=htonl(if_addr.toIPv4Address());
  mreq.imr_ifindex=0;
  if(setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq))<0) {
    *err_msg=QString("Unable to subscribe to multicast address")+
      " \""+addr.toString()+"\" ["+strerror(errno)+"]";
    return false;
  }

  return true;
}


unsigned MainObject::ReadIntegerArg(const QString &arg,bool *ok) const
{
  unsigned val=0;

  if(arg.left(2).toLower()=="0x") {
    val=arg.right(arg.length()-2).toUInt(ok,16);
  }
  else {
    val=arg.toUInt(ok,10);
  }

  return val;
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();
  return a.exec();
}
