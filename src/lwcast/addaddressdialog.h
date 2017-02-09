// addaddressdialog.h
//
// Add a Multicast Address
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

#ifndef ADDADDRESSDIALOG_H
#define ADDADDRESSDIALOG_H

#include <stdint.h>

#include <QComboBox>
#include <QDialog>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class AddAddressDialog : public QDialog
{
 Q_OBJECT;
 public:
  AddAddressDialog(QWidget *parent=0);
  QSize sizeHint() const;

 public slots:
   int exec(QHostAddress *addr,QHostAddress *if_addr,QString *if_name);

 private slots:
  void okData();
  void cancelData();

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private:
  void PopulateInterfaces(QComboBox *box);
  unsigned GetInterfaceInfo(QList<uint64_t> *mac_addrs,
			    QList<QHostAddress> *ipv_addrs=NULL,
			    QList<QString> *if_names=NULL);
  QLabel *add_address_label;
  QLineEdit *add_address_edit;
  QLabel *add_interface_label;
  QComboBox *add_interface_box;
  QPushButton *add_ok_button;
  QPushButton *add_cancel_button;
  QHostAddress *add_mcast_address;
  QHostAddress *add_iface_address;
  QString *add_iface_name;
};


#endif  // ADDADDRESSDIALOG_H
