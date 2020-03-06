// lwmastermon.h
//
// Monitor and display the location of the LiveWire master node
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

#ifndef LWMASTERMON_H
#define LWMASTERMON_H

#include <stdint.h>

#include <QHostAddress>
#include <QLabel>
#include <QMainWindow>
#include <QSocketNotifier>
#include <QTimer>

#define LWMASTERMON_MASTER_ADDR "239.192.255.2"
#define LWMASTERMON_MASTER_PORT 7000
#define LWMASTERMON_WATCHDOG_INTERVAL 2000

#define LWMASTERMON_USAGE "\n"

class MainWidget : public QMainWindow
{
 Q_OBJECT;
 public:
  MainWidget(QWidget *parent=0);
  QSize sizeHint() const;

 private slots:
  void activatedData(int sock);
  void watchdogData();

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  void UpdateWatchdog(const QHostAddress &addr);
  void Subscribe();
  void Subscribe(int sock,int index);
  bool IsAddress(const QHostAddress &addr,const char *data,int offset) const;
  QHostAddress IpAddress(const char *data,int offset) const;
  void DumpIpAddress(const char *data,int offset) const;
  QLabel *mon_label;
  QLabel *mon_value_label;
  QSocketNotifier *mon_notifier;
  QTimer *mon_watchdog_timer;
};


#endif  // LWMASTERMON_H
