// lwmultcap.cpp
//
// Print multicast messages from a specified address and port
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
  c_show_ruler=false;
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
  c_socket=new QUdpSocket(this);
  if(!c_socket->bind(c_port)) {
    fprintf(stderr,"lwmultcap: unable to bind to port %d [%s]\n",
	    c_port,strerror(errno));
    exit(1);
  }
  connect(c_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  if(!Subscribe(c_mcast_address,c_iface_address,&err_msg)) {
    fprintf(stderr,"lwmultcap: unable to subscribe to %s [%s]\n",
	    c_mcast_address.toString().toUtf8().constData(),strerror(errno));
    exit(1);
  }
}


void MainObject::readyReadData()
{
  QHostAddress addr;
  uint16_t port=0;
  char data[1501];
  int64_t n;

  if((n=c_socket->readDatagram(data,1500,&addr,&port))<0) {
    fprintf(stderr,"lwmultcap: socket error [%s]\n",strerror(errno));
    exit(1);
  }

  packetReceived(addr,port,QByteArray(data,n));
}


void MainObject::packetReceived(const QHostAddress &src_addr,uint16_t src_port,
				const QByteArray &data)
{
  bool match=false;

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

  dumpToHex(src_addr,src_port,data);
  if(c_packet_limit>0) {
    if(--c_packet_limit==0) {
      exit(0);
    }
  }
}


void MainObject::dumpToHex(const QHostAddress &src_addr,uint16_t src_port,
			   const QByteArray &data)
{
  QHostAddress addr(src_addr.toIPv4Address());  // Strip out IPv6 attributes
  QString recv_from_str=
    addr.toString()+QString::asprintf(":%d",0xFFFF&src_port);
  QString size_str=QString::asprintf("%d [0x%04X]",data.size(),data.size());

  if(c_show_ruler) {
    printf("------------------------------------------------------------------------------\n");
    printf("| Received from: %-22s          Packet size: %-14s |\n",
	   recv_from_str.toUtf8().constData(),
	   size_str.toUtf8().constData());
    printf("| Offset  0- 1- 2- 3- 4- 5- 6- 7- 8- 9- A- B- C- D- E- F- | 0123456789ABCDEF |\n");
    printf("----------------------------------------------------------|------------------|\n");
  }
  for(int i=0;i<data.size();i+=16) {
    QString str="";
    if(((c_first_offset<0)||(c_first_offset<=i))&&
       ((c_last_offset<0)||(c_last_offset>=i))) {
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
  }
  if(c_show_ruler) {
    printf("------------------------------------------------------------------------------\n");
  }
}


bool MainObject::Subscribe(const QHostAddress &addr,const QHostAddress &if_addr,
			   QString *err_msg)
{
  struct ip_mreqn mreq;

  memset(&mreq,0,sizeof(mreq));
  mreq.imr_multiaddr.s_addr=htonl(addr.toIPv4Address());
  mreq.imr_address.s_addr=htonl(if_addr.toIPv4Address());
  mreq.imr_ifindex=0;
  if(setsockopt(c_socket->socketDescriptor(),IPPROTO_IP,IP_ADD_MEMBERSHIP,
		&mreq,sizeof(mreq))<0) {
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
