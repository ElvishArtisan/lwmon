// lwcap.h
//
// Capture a LiveWire RTP stream to a file.
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef LWCAP_H
#define LWCAP_H

#include <QObject>
#include <QTimer>
#include <QUdpSocket>

#include <sndfile.h>

#define LWCAP_USAGE "--filename=<outfile> --multicast-address=<ip-addr> --interface-address=<ip-addr>\n"

class MainObject : public QObject
{
  Q_OBJECT
 public:
  MainObject(QObject *parent=0);

 private slots:
  void readyReadData();
  void errorData(QAbstractSocket::SocketError err);
  void durationData();
  void exitData();

 private:
  bool Subscribe(const QHostAddress &addr,const QHostAddress &if_addr);
  void WritePcm24(const char *data,int bytes);
  QUdpSocket *main_rtp_socket;
  SNDFILE *main_sndfile;
  QTimer *main_duration_timer;
  QTimer *main_exit_timer;
};


#endif  // LWCAP_H
