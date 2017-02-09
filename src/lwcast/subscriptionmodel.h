// subscriptionmodel.h
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

#ifndef SUBSCRIPTIONMODEL_H
#define SUBSCRIPTIONMODEL_H

#include <QAbstractTableModel>
#include <QHostAddress>
#include <QStringList>

class SubscriptionModel : public QAbstractTableModel
{
 Q_OBJECT;
 public:
  SubscriptionModel(QObject *parent=0);
  ~SubscriptionModel();
  int rowCount(const QModelIndex &index) const;
  int columnCount(const QModelIndex &index) const;
  QVariant headerData(int section,Qt::Orientation orient,
		      int role=Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const;
  void addAddress(const QHostAddress &addr,const QHostAddress &if_addr,
		  const QString &if_name);
  void removeAddress(const QModelIndex &index);

 signals:
  void error(const QString &msg);

 private:
  bool Subscribe(const QHostAddress &addr,const QHostAddress &if_addr);
  bool Unsubscribe(const QHostAddress &addr,const QHostAddress &if_addr);
  QList<QHostAddress> model_addresses;
  QList<QHostAddress> model_iface_addresses;
  QStringList model_iface_names;
  int model_socket;
};


#endif  // SUBSCRIPTIONMODEL_H
