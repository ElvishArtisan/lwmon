// lwmastermon.h
//
// Monitor and display the location of the LiveWire master node
//
//   (C) Copyright 2017-2020 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef LWMASTERMON_H
#define LWMASTERMON_H

#include <QHostAddress>
#include <QLabel>
#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>

#define LWMASTERMON_UPDATE_INTERVAL 1000
#define LWMASTERMON_MIN_WIDTH 200
#define LWMASTERMON_USAGE "\n"

class MainWidget : public QWidget
{
 Q_OBJECT;
 public:
  MainWidget(QWidget *parent=0);
  QSize sizeHint() const;

 private slots:
  void updateData();
  void lwrpConnectedData();
  void lwrpReadyReadData();
  void lwrpDisconnectedData();
  void lwrpErrorData(QAbstractSocket::SocketError err);

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  void SetLabel(const QString &str,bool error);
  QLabel *mon_label;
  QLabel *mon_value_label;
  int mon_min_width;
  int mon_width;
  QTimer *mon_update_timer;
  QTcpSocket *mon_lwrp_socket;
  QByteArray mon_lwrp_accum;
  QString mon_current_result;
};


#endif  // LWMASTERMON_H
