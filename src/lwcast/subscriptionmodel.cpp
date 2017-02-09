// subscriptionmodel.cpp
//
// Qt Model for Multicast Subscriptions
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#include "subscriptionmodel.h"

SubscriptionModel::SubscriptionModel(QObject *parent)
  : QAbstractTableModel(parent)
{
  model_socket=socket(AF_INET,SOCK_DGRAM,0);
}


SubscriptionModel::~SubscriptionModel()
{
  close(model_socket);
}


int SubscriptionModel::rowCount(const QModelIndex &index) const
{
  return model_addresses.size();
}


int SubscriptionModel::columnCount(const QModelIndex &index) const
{
  return 2;
}


QVariant SubscriptionModel::headerData(int section,Qt::Orientation orient,
				       int role) const
{
  if((orient==Qt::Horizontal)&&(role==Qt::DisplayRole)) {
    switch(section) {
    case 0:
      return QVariant(tr("Interface"));

    case 1:
      return QVariant(tr("Address"));
    }
  }

  return QVariant();
}


QVariant SubscriptionModel::data(const QModelIndex &index,int role) const
{
  switch((Qt::ItemDataRole)role) {
  case Qt::DisplayRole:
    switch(index.column()) {
    case 0:
      return QVariant(model_iface_names.at(index.row())+" - "+
		      model_iface_addresses.at(index.row()).toString());

    case 1:
      return QVariant(model_addresses.at(index.row()).toString());
    }
    break;

  default:
    break;
  }
  return QVariant();
}


void SubscriptionModel::addAddress(const QHostAddress &addr,
				   const QHostAddress &if_addr,
				   const QString &if_name)
{
  for(int i=0;i<model_addresses.size();i++) {
    if((model_addresses.at(i)==addr)&&(model_iface_names.at(i)==if_name)) {
      return;
    }
  }
  if(Subscribe(addr,if_addr)) {
    beginInsertRows(QModelIndex(),model_addresses.size(),model_addresses.size());
    model_addresses.append(addr);
    model_iface_addresses.append(if_addr);
    model_iface_names.append(if_name);
    endInsertRows();
  }
}


void SubscriptionModel::removeAddress(const QModelIndex &index)
{
  if(Unsubscribe(model_addresses.at(index.row()),model_iface_addresses.at(index.row()))) {
    beginRemoveRows(QModelIndex(),index.row(),index.row());
    model_addresses.removeAt(index.row());
    model_iface_addresses.removeAt(index.row());
    model_iface_names.removeAt(index.row());
    endRemoveRows();
  }
}


bool SubscriptionModel::Subscribe(const QHostAddress &addr,
				  const QHostAddress &if_addr)
{
  struct ip_mreqn mreq;

  memset(&mreq,0,sizeof(mreq));
  mreq.imr_multiaddr.s_addr=htonl(addr.toIPv4Address());
  mreq.imr_address.s_addr=htonl(if_addr.toIPv4Address());
  mreq.imr_ifindex=0;
  if(setsockopt(model_socket,IPPROTO_IP,IP_ADD_MEMBERSHIP,
		&mreq,sizeof(mreq))<0) {
    emit error(tr("Unable to subscribe to multicast address")+" \""+
	       addr.toString()+"\" ["+strerror(errno)+"]");
    return false;
  }
  return true;
}


bool SubscriptionModel::Unsubscribe(const QHostAddress &addr,
				    const QHostAddress &if_addr)
{
  struct ip_mreqn mreq;

  memset(&mreq,0,sizeof(mreq));
  mreq.imr_multiaddr.s_addr=htonl(addr.toIPv4Address());
  mreq.imr_address.s_addr=htonl(if_addr.toIPv4Address());
  mreq.imr_ifindex=0;
  if(setsockopt(model_socket,IPPROTO_IP,IP_DROP_MEMBERSHIP,
		&mreq,sizeof(mreq))<0) {
    emit error(tr("Unable to unsubscribe from multicast address")+" \""+
	       addr.toString()+"\" ["+strerror(errno)+"]");
    return false;
  }
  return true;
}
