// lwmon.h
//
// LiveWire Protocol monitor
//
//   (C) Copyright 2015 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef LWMON_H
#define LWMON_H

#include <QDir>
#include <QTcpSocket>
#include <QTextEdit>
#include <QMainWindow>

#include "astring.h"
#include "lineedit.h"

#define LWMON_DEFAULT_MODE MainWidget::Lwrp
#define LWMON_LWCP_DEFAULT_PORT 4010
#define LWMON_LWRP_DEFAULT_PORT 93
#define LWMON_SETTINGS_DIR ".lwcpmon"
#define LWMON_LWCP_HISTORY_FILE "lwcp_history"
#define LWMON_LWRP_HISTORY_FILE "lwrp_history"
#define LWMON_USAGE "[--mode=lwcp|lwrp] [--color=on|off] <hostname>[:<port-num>]\n"

class MainWidget : public QMainWindow
{
 Q_OBJECT;
 public:
  enum Mode {Lwrp=1,Lwcp=2};
  MainWidget(QWidget *parent=0);
  QSize sizeHint() const;

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);

 private slots:
  void editReturnPressedData();
  void tcpReadyReadData();
  void tcpConnectedData();
  void tcpDisconnectedData();
  void tcpErrorData(QAbstractSocket::SocketError err);

 private:
  QString Colorize(const QString &cmd) const;
  QString ColorizeLwcp(const AString &cmd) const;
  QString ColorizeLwrp(const AString &cmd) const;
  QString ColorString(const QString &str,const QColor &color) const;
  void ProcessCommand(const QString &cmd);
  QString FormatLwcp(const QString &str,bool local);
  bool CheckSettingsDirectory();
  MainWidget::Mode SetMode() const;
  QTextEdit *lw_text;
  LineEdit *lw_edit;

  QTcpSocket *lw_tcp_socket;
  QString lw_accum;
  QDir *lw_settings_dir;
  QString lw_history_path;
  MainWidget::Mode lw_mode;
  bool lw_colorize;
};


#endif  // LWMON_H
