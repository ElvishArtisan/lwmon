// lwmon_object.cpp
//
// LiveWire Protocol monitor
//
//   (C) Copyright 2015-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <QApplication>
#include <QHostAddress>
#include <QStringList>
#include <QTextStream>

#include "cmdswitch.h"
#include "lwmon.h"

MainObject::MainObject(const QString &hostname,uint16_t port,
		       const QString &filename,QObject *parent)
  : QObject(parent)
{
  d_hostname=hostname;
  d_port=port;

  //
  // Open Source File
  //
  d_write_from_file=new QFile(filename);
  if(!d_write_from_file->open(QIODevice::ReadOnly)) {
    fprintf(stderr,"lwrp: unable to open source file\n");
    exit(1);
  }

  //
  // Connect to Livewire Device
  //
  d_tcp_socket=new QTcpSocket(this);
  connect(d_tcp_socket,SIGNAL(connected()),this,SLOT(tcpConnectedData()));
  connect(d_tcp_socket,SIGNAL(disconnected()),
	  this,SLOT(tcpDisconnectedData()));
  connect(d_tcp_socket,SIGNAL(readyRead()),this,SLOT(tcpReadyReadData()));
  connect(d_tcp_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(tcpErrorData(QAbstractSocket::SocketError)));
  d_tcp_socket->connectToHost(hostname,port);
}


void MainObject::WriteFromFile(QFile *file)
{
  QTextStream in(file);
  QString line;
  while(in.readLineInto(&line)) {
    AString aline(line.trimmed());
    if(aline.left(1)!="#") {
      QStringList f0=aline.split(" ","\"");
      if((f0.first().toUpper()!="IP")&&(f0.first().toUpper()!="VER")) {
	printf("WOULD WRITE:\"%s\"\n",(line.trimmed()+"\r\n").toUtf8().constData());
	//d_tcp_socket->write((line.trimmed()+"\r\n").toUtf8());
      }
    }
  }
}


void MainObject::tcpReadyReadData()
{
  char data[1500];
  int n;

  while((n=d_tcp_socket->read(data,1500))>0) {
    for(int i=0;i<n;i++) {
      switch(data[i]) {
      case 10:
	break;

      case 13:
	ProcessCommand(d_accum);
	d_accum="";
	break;

      default:
	d_accum+=data[i];
	break;
      }
    }
  }
}


void MainObject::tcpConnectedData()
{
  d_tcp_socket->write("LOGIN\r\n",7);
  QTextStream in(d_write_from_file);
  QString line;
  while(in.readLineInto(&line)) {
    AString aline(line.trimmed());
    if(aline.left(1)!="#") {
      QStringList f0=aline.split(" ","\"");
      if((f0.first().toUpper()!="IP")&&(f0.first().toUpper()!="VER")) {
	//	printf("%s",(line.trimmed()+"\r\n").toUtf8().constData());
	d_tcp_socket->write((line.trimmed()+"\r\n").toUtf8());
      }
    }
  }
  d_tcp_socket->write("VER\r\n",5);
}


void MainObject::tcpDisconnectedData()
{
  fprintf(stderr,"lwrp: far end disconnected unexpectedly\n");
  exit(1);
}


void MainObject::tcpErrorData(QAbstractSocket::SocketError err)
{
  QString err_text;

  switch(err) {
  case QAbstractSocket::ConnectionRefusedError:
    err_text=tr("Connection refused");
    break;

  case QAbstractSocket::RemoteHostClosedError:
    err_text=tr("Remote host closed connection");
    break;

  case QAbstractSocket::HostNotFoundError:
    err_text=tr("Host not found");
    break;

  case QAbstractSocket::SocketTimeoutError:
    err_text=tr("Connection timed out");
    break;

  case QAbstractSocket::DatagramTooLargeError:
    err_text=tr("Datagram too large");
    break;

  case QAbstractSocket::NetworkError:
    err_text=tr("Network error");
    break;

  case QAbstractSocket::AddressInUseError:
    err_text=tr("Address in use");
    break;

  case QAbstractSocket::SocketAddressNotAvailableError:
    err_text=tr("Socket address not available");
    break;

  case QAbstractSocket::UnsupportedSocketOperationError:
    err_text=tr("Unsupported socket operation");
    break;

  default:
    err_text=tr("Network error")+QString::asprintf(" %u ",err)+tr("received");
    break;
  }

  fprintf(stderr,"lwrp: %s\n",err_text.toUtf8().constData());
  exit(1);
}


void MainObject::ProcessCommand(const QString &cmd)
{
  //  printf("RCVD: %s\n",cmd.toUtf8().constData());
  QStringList f0=AString(cmd).split(" ","\"");
  if(f0.first().trimmed()=="VER") {
    exit(0);
  }
}
