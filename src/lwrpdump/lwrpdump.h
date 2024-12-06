// lwrpdump.h
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

#ifndef LWRPDUMP_H
#define LWRPDUMP_H

#include <QObject>
#include <QTcpSocket>

#include <sndfile.h>

#define LWRPDUMP_USAGE "--hostname=<host-name> [--port=<port-num>] [--dump-all] [--dump-src] [--dump-dst] [--dump-gpio] [--dump-ip] [--dump-ver]\n\n"

class MainObject : public QObject
{
  Q_OBJECT
 public:
  MainObject();

 private slots:
  void connectedData();
  void disconnectedData();
  void errorData(QAbstractSocket::SocketError err);
  void readyReadData();
  
 private:
  void ProcessLwrp(const QString &cmd);
  QByteArray d_accum;
  QTcpSocket *d_socket;
  bool d_dump_src;
  bool d_dump_dst;
  bool d_dump_gpio;
  bool d_dump_ip;
  bool d_dump_ver;
};


#endif  // LWRPDUMP_H
