// lwrpdump.cpp
//
// Dump LWRP settings from a Livewire device
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <stdio.h>
#include <stdlib.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QHostAddress>

#include "astring.h"
#include "cmdswitch.h"
#include "lwrpdump.h"

MainObject::MainObject()
  : QObject(NULL)
{
  QString hostname;
  unsigned port=93;
  QString password;
  bool ok=false;
  d_dump_src=false;
  d_dump_dst=false;
  d_dump_gpio=false;
  d_dump_ip=false;
  d_dump_ver=false;

  //
  // Process Arguments
  //
  CmdSwitch *cmd=new CmdSwitch("lwrpdump",VERSION,LWRPDUMP_USAGE);
  for(int i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--dump-all") {
      d_dump_src=true;
      d_dump_dst=true;
      d_dump_gpio=true;
      d_dump_ip=true;
      d_dump_ver=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--dump-dst") {
      d_dump_dst=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--dump-gpio") {
      d_dump_gpio=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--dump-ip") {
      d_dump_ip=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--dump-src") {
      d_dump_src=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--dump-ver") {
      d_dump_ver=true;
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--hostname") {
      hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--password") {
      password=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--port") {
      port=cmd->value(i).toUInt(&ok);
      if((!ok)||(port>0xFFFF)) {
	fprintf(stderr,"lwrpdump: invalid \"--port\" value\n");
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    
    if(!cmd->processed(i)) {
      fprintf(stderr,"lwrpdump: unrecognized option \"%s\"\n",
	      cmd->key(i).toUtf8().constData());
      exit(1);
    }
  }
  if((!d_dump_dst)&&(!d_dump_ip)&&(!d_dump_src)&&(!d_dump_gpio)&&
     (!d_dump_ver)) {
      d_dump_src=true;
      d_dump_dst=true;
      d_dump_gpio=true;
      d_dump_ip=true;
      d_dump_ver=true;
  }

  d_socket=new QTcpSocket(this);
  connect(d_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(d_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
  connect(d_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(d_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
  d_socket->connectToHost(hostname,port);
}


void MainObject::connectedData()
{
  d_socket->write("DST\r\n");
  d_socket->write("SRC\r\n");
  d_socket->write("CFG GPO\r\n");
  d_socket->write("VER\r\n");
  d_socket->write("IP\r\n");

  printf("# Dump from livewire device %s:%d @ %s\n",
	 d_socket->peerAddress().toString().toUtf8().constData(),
	 d_socket->peerPort(),
	 QDateTime::currentDateTime().
	 toString("dd-MM-yyyyThh:mm:ss").toUtf8().constData());
  printf("\n");
}


void MainObject::disconnectedData()
{
  fprintf(stderr,"far end disconnected\n");
  exit(1);
}


void MainObject::errorData(QAbstractSocket::SocketError err)
{
}


void MainObject::readyReadData()
{
  QByteArray data=d_socket->readAll();

  for(int i=0;i<data.size();i++) {
    switch(data.at(i)) {
    case '\n':
      ProcessLwrp(QString::fromUtf8(d_accum).trimmed());
      d_accum.clear();
      break;

    case '\r':
      break;

    default:
      d_accum+=data.at(i);
      break;
    }
  }
}
  

void MainObject::ProcessLwrp(const QString &cmd)
{
  QStringList f0=AString(cmd).split(" ","\"");

  if(d_dump_dst&&(f0.first()=="DST")) {
    printf("%s\r\n",cmd.toUtf8().constData());
  }
  if(d_dump_src&&(f0.first()=="SRC")) {
    printf("%s\r\n",cmd.toUtf8().constData());
  }
  if(d_dump_gpio&&(f0.first()=="CFG")) {
    printf("%s\r\n",cmd.toUtf8().constData());
  }
  if(d_dump_ver&&(f0.first()=="VER")) {
    printf("%s\r\n",cmd.toUtf8().constData());
  }
  if(f0.first()=="IP") {
    if(d_dump_ip) {
      printf("%s\r\n",cmd.toUtf8().constData());
    }
    exit(0);
  }
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);

  new MainObject();

  return a.exec();
}
