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

  //
  // Process command-line switches
  //
  CmdSwitch *cmd=
    new CmdSwitch(qApp->argc(),qApp->argv(),"lwmultcap",LWMULTCAP_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--first-offset") {
      if(cmd->value(i).left(2).toLower()=="0x") {
	c_first_offset=
	  cmd->value(i).right(cmd->value(i).length()-2).toInt(&ok,16);
      }
      else {
	c_first_offset=cmd->value(i).toInt(&ok);
      }
      if(!ok) {
	fprintf(stderr,"lwmultcap: invalid offset\n");
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
      if(cmd->value(i).left(2).toLower()=="0x") {
	c_last_offset=
	  cmd->value(i).right(cmd->value(i).length()-2).toInt(&ok,16);
      }
      else {
	c_last_offset=cmd->value(i).toInt(&ok);
      }
      if(!ok) {
	fprintf(stderr,"lwmultcap: invalid offset\n");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }

    if(cmd->key(i)=="--port") {
      port=cmd->value(i).toUInt(&ok);
      if((!ok)||(port>0xFFFF)) {
	fprintf(stderr,"lwmultcap: invalid port\n");
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
  //  printf("received %d bytes from %s:%u\n",data.size(),
  //	 src_addr.toString().toUtf8().constData(),0xFFFF&src_port);

  dumpToHex(data);
  /*
  //
  // Display Input Meters
  //
  for(int i=0;i<16;i++) {
    unsigned lvl=((0xFF&data[2*i+72])<<8)+(0xFF&data[2*i+73]);
    double dbfs=-100.0;
    if(lvl>3598) {
      dbfs=0.000981*(double)lvl-64.45;
    }
    printf("Input Channel %2d: %4.1lf\n",i+1,dbfs);
  }
  printf("------------------------\n");

  //
  // Display Output Meters
  //
  for(int i=0;i<16;i++) {
    unsigned lvl=((0xFF&data[2*i+112])<<8)+(0xFF&data[2*i+113]);
    double dbfs=-100.0;
    if(lvl>3598) {
      dbfs=0.000981*(double)lvl-64.45;
    }
    printf("Output Channel %2d: %4.1lf\n",i+1,dbfs);
  }
  printf("------------------------\n");
  */
}


void MainObject::dumpToHex(const QByteArray &data)
{
  if(c_show_ruler) {
    printf("        0- 1- 2- 3- 4- 5- 6- 7- 8- 9- A- B- C- D- E- F-\n");
  }
  for(int i=0;i<data.size();i+=16) {
    QString str="";
    if(((c_first_offset<0)||(c_first_offset<=i))&&
       ((c_last_offset<0)||(c_last_offset>=i))) {
      printf("0x%04X: ",i);
      for(int j=0;j<16;j++) {
	if((i+j)<data.size()) {
	  char c=0xFF&data[i+j];
	  printf("%02X ",0xFF&c);
	  if((c>='!')&&(c<='~')) {
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
      printf(" | %s |\n",str.toUtf8().constData());
    }
  }
  if(c_show_ruler) {
    printf("\n");
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


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();
  return a.exec();
}
