// udpmon.h
//
// UDP communications monitor
//
//   (C) Copyright 2015-2023 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef UDPMON_H
#define UDPMON_H

#include <stdint.h>

#include <QDir>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QUdpSocket>

#include "lineedit.h"
#include "statuswidget.h"

#define UDPMON_SETTINGS_DIR ".udpmon"
#define UDPMON_HISTORY_FILE "history"
#define UDPMON_USAGE "[--receive-port=<udp-port>] [--send-port=<udp-port>] [--send-to-address=<ip-addr>] [--send-to-port=<udp-port>]\n"

class MainWidget : public QMainWindow
{
 Q_OBJECT;
 public:
  MainWidget(QWidget *parent=0);
  QSize sizeHint() const;

 protected:
  void closeEvent(QCloseEvent *e);
  void paintEvent(QPaintEvent *q);
  void resizeEvent(QResizeEvent *e);

 private slots:
  void editReturnPressedData();
  void recvPortChangedData(int port);
  void recvPortBindData();
  void readyReadData();
  void sendPortChangedData(int port);
  void sendPortBindData();
  void sendToAddressChangedData(const QString &str);
  void errorData(QAbstractSocket::SocketError err);

 private:
  bool CheckSettingsDirectory();
  QMessageBox::StandardButton
    DisplayMessageBox(QMessageBox::Icon icon,const QString &caption,
		      const QString &text,const QString &info_text="",
		      QMessageBox::StandardButtons buttons=QMessageBox::Ok,
		      QMessageBox::StandardButton def_button=QMessageBox::Ok);
  QTextEdit *d_text;
  LineEdit *d_edit;
  QPushButton *d_button;

  QString d_hostname;
  uint16_t d_port;
  QString d_accum;
  QDir *d_settings_dir;
  QString d_history_path;

  QLabel *d_recvport_label;
  QSpinBox *d_recvport_spin;
  QPushButton *d_recvport_bind_button;
  QUdpSocket *d_recv_socket;

  QLabel *d_sendport_label;
  QSpinBox *d_sendport_spin;
  QPushButton *d_sendport_bind_button;
  QLabel *d_sendtoaddr_label;
  QLineEdit *d_sendtoaddr_edit;
  QLabel *d_sendtoport_label;
  QSpinBox *d_sendtoport_spin;
  QUdpSocket *d_send_socket;

  QLabel *d_pid_label;
  QLineEdit *d_pid_edit;
};


#endif  // UDPMON_H
