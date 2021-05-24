// lwcap.cpp
//
// Capture a LiveWire RTS stream to a file.
//
//   (C) Copyright 2017-2021 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
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
#include <signal.h>
#include <stdint.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <unistd.h>

#include <QCoreApplication>

#include "cmdswitch.h"
#include "lwcap.h"

bool global_exiting=false;

void SigHandler(int signo)
{
  switch(signo) {
  case SIGTERM:
  case SIGINT:
    global_exiting=true;
    break;
  }
}


MainObject::MainObject(QObject *parent)
{
  QString filename;
  QHostAddress multicast_address;
  QHostAddress interface_address;
  unsigned duration=0;
  unsigned channels=2;
  bool ok=false;

  main_sndfile=NULL;

  CmdSwitch *cmd=new CmdSwitch("lwcap",LWCAP_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--channels") {
      channels=cmd->value(i).toUInt(&ok);
      if(!ok) {
	fprintf(stderr,"lwcap: invalid --channels\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--duration") {
      duration=cmd->value(i).toUInt(&ok);
      if(!ok) {
	fprintf(stderr,"lwcap: invalid --duration\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--filename") {
      filename=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--multicast-address") {
      multicast_address.setAddress(cmd->value(i));
      if(multicast_address.isNull()) {
	fprintf(stderr,"lwcap: invalid --multicast-address\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--interface-address") {
      interface_address.setAddress(cmd->value(i));
      if(interface_address.isNull()) {
	fprintf(stderr,"lwcap: invalid --interface-address\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"lwcap: unknown option\n");
      exit(256);
    }
  }
  if(multicast_address.isNull()) {
    fprintf(stderr,"lwcap: no --multicast-address specified\n");
    exit(256);
  }
  if(interface_address.isNull()) {
    fprintf(stderr,"lwcap: no --interface-address specified\n");
    exit(256);
  }

  //
  // Open Destination File
  //
  SF_INFO sf;
  memset(&sf,0,sizeof(sf));
  sf.samplerate=48000;
  sf.channels=channels;
  sf.format=SF_FORMAT_WAV|SF_FORMAT_PCM_24;

  if(!filename.isEmpty()) {
    if((main_sndfile=sf_open(filename.toUtf8(),SFM_WRITE,&sf))==NULL) {
      fprintf(stderr,"lwcap: unable to open output file [%s]\n",
	      sf_strerror(main_sndfile));
      exit(256);
    }
  }

  main_rtp_socket=new QUdpSocket(this);
  connect(main_rtp_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  if(!main_rtp_socket->bind(5004)) {
    fprintf(stderr,"unable to bind RTP port [%s]\n",strerror(errno));
    exit(256);
  }
  Subscribe(multicast_address,interface_address);

  //
  // Timers
  //
  main_duration_timer=new QTimer(this);
  main_duration_timer->setSingleShot(true);
  connect(main_duration_timer,SIGNAL(timeout()),this,SLOT(durationData()));
  if(duration>0) {
    main_duration_timer->start(duration*1000);
  }
  main_exit_timer=new QTimer(this);
  connect(main_exit_timer,SIGNAL(timeout()),this,SLOT(exitData()));
  main_exit_timer->start(100);

  ::signal(SIGINT,SigHandler);
  ::signal(SIGTERM,SigHandler);
}


void MainObject::readyReadData()
{
  QHostAddress addr;
  uint16_t port;
  char data[1501];
  int64_t n;

  while((n=main_rtp_socket->readDatagram(data,1500,&addr,&port))>0) {
    WritePcm24(data+12,n-12);
  }
  exitData();
}


void MainObject::errorData(QAbstractSocket::SocketError err)
{
  fprintf(stderr,"received socket error %d\n",err);
  exit(256);
}


void MainObject::durationData()
{
  sf_close(main_sndfile);
  exit(0);
}


void MainObject::exitData()
{
  if(global_exiting) {
    sf_close(main_sndfile);
    exit(0);
  }
}


bool MainObject::Subscribe(const QHostAddress &addr,const QHostAddress &if_addr)
{
  struct ip_mreqn mreq;

  memset(&mreq,0,sizeof(mreq));
  mreq.imr_multiaddr.s_addr=htonl(addr.toIPv4Address());
  mreq.imr_address.s_addr=htonl(if_addr.toIPv4Address());
  mreq.imr_ifindex=0;
  if(setsockopt(main_rtp_socket->socketDescriptor(),
		IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq))<0) {
    fprintf(stderr,
	    "lwcap: unable to subscribe to multicast address \"%s\" [%s]",
	    (const char *)addr.toString().toUtf8(),strerror(errno));
    exit(256);
  }
  return true;
}


void MainObject::WritePcm24(const char *data,int bytes)
{
  int8_t pcm[1440*4];

  if(main_sndfile==NULL) {
    if(write(1,data,bytes)!=1) {
      fprintf(stderr,"lwcap: write to stdout failed\n");
    }
  }
  else {
    for(int i=0;i<(bytes/3);i++) {
      pcm[4*i]=0;
      pcm[4*i+1]=data[3*i+2];
      pcm[4*i+2]=data[3*i+1];
      pcm[4*i+3]=data[3*i];
    }
    sf_writef_int(main_sndfile,(int32_t *)pcm,bytes/6);
  }
}


int main(int argv,char *argc[])
{
  QCoreApplication a(argv,argc);
  new MainObject();
  return a.exec();
}
