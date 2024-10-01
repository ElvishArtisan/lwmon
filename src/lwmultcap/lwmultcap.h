// lwmultcap.h
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

#ifndef LWMULTCAP_H
#define LWMULTCAP_H

#include <stdint.h>

#include <QByteArray>
#include <QHostAddress>
#include <QObject>

#define LWMULTCAP_USAGE "--iface-address=<iface-addr> --mcast-address=<mcast-addr> --port=<port-num> [--show-ruler] [--no-ruler] [--first-offset=<offset>] [--last-offset=<offset>] [--filter-byte=<offset>:<value>] [--filter-string=<offset>:<string>\n\n"

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);

 protected:
  
 private:
  void MainLoop(int sock);
  void ProcessPacket(const QHostAddress &dst_addr,
		     const QHostAddress &src_addr,uint16_t src_port,
		     QByteArray data);
  void PrintPacket(const QHostAddress &dst_addr,
		   const QHostAddress &src_addr,uint16_t src_port,
		   const QByteArray &data);
  bool Subscribe(int sock,const QHostAddress &addr,const QHostAddress &if_addr,
  		 QString *err_msg);
  unsigned ReadIntegerArg(const QString &arg,bool *ok) const;
  QHostAddress c_mcast_address;
  QHostAddress c_iface_address;
  uint16_t c_port;
  bool c_show_ruler;
  int c_first_offset;
  int c_last_offset;
  QList<QHostAddress> c_filter_source_addresses;
  QMap<unsigned,char> c_filter_bytes;
  QMap<unsigned,QString> c_filter_strings;
  unsigned c_packet_limit;
};


#endif  // LWMULTCAP_H
