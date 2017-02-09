// lwcast.h
//
// Multicast Subscription Monitor
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

#ifndef LWCAST_H
#define LWCAST_H

#include <QTableView>
#include <QMainWindow>
#include <QPushButton>

#include "addaddressdialog.h"
#include "subscriptionmodel.h"

#define LWCAST_USAGE "\n"

class MainWidget : public QMainWindow
{
 Q_OBJECT;
 public:
  MainWidget(QWidget *parent=0);
  QSize sizeHint() const;

 private slots:
  void addData();
  void removeData();
  void errorData(const QString &msg);

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private:
  QTableView *main_address_list;
  SubscriptionModel *main_address_model;
  QPushButton *main_add_button;
  QPushButton *main_remove_button;
  AddAddressDialog *main_addaddress_dialog;
};


#endif  // LWCAST_H
