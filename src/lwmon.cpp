// lwmon.cpp
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

#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <QApplication>
#include <QMessageBox>
#include <QStringList>

#include "cmdswitch.h"
#include "lwmon.h"

MainWidget::MainWidget(QWidget *parent)
  : QMainWindow(parent)
{
  QString hostname;
  uint16_t port=LWMON_LWRP_DEFAULT_PORT;
  bool ok=false;

  //
  // Process Arguments
  //
  lw_mode=SetMode();
  CmdSwitch *cmd=new CmdSwitch(qApp->argc(),qApp->argv(),"lwcp",LWMON_USAGE);
  if(cmd->keys()==0) {
    fprintf(stderr,"%s\n",LWMON_USAGE);
    exit(256);
  }
  for(unsigned i=0;i<cmd->keys()-1;i++) {
    if(cmd->key(i)=="--mode") {
      if(cmd->value(i).toLower()=="lwcp") {
	lw_mode=MainWidget::Lwcp;
	cmd->setProcessed(i,true);
      }
      if(cmd->value(i).toLower()=="lwrp") {
	lw_mode=MainWidget::Lwrp;
	cmd->setProcessed(i,true);
      }
      if(!cmd->processed(i)) {
	fprintf(stderr,"lwmon: invalid \"--mode\" argument");
	exit(256);
      }
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"lwmon: invalid argument \"%s\"\n",
	      (const char *)cmd->key(i).toUtf8());
      exit(256);
    }
  }

  //
  // Set Mode Defaults
  //
  switch(lw_mode) {
  case MainWidget::Lwcp:
    port=LWMON_LWCP_DEFAULT_PORT;
    setWindowTitle(tr("LWCP Monitor"));
    if(CheckSettingsDirectory()) {
      lw_history_path=lw_settings_dir->path()+"/"+LWMON_LWCP_HISTORY_FILE;
    }
    break;

  case MainWidget::Lwrp:
    port=LWMON_LWRP_DEFAULT_PORT;
    setWindowTitle(tr("LWRP Monitor"));
    if(CheckSettingsDirectory()) {
      lw_history_path=lw_settings_dir->path()+"/"+LWMON_LWRP_HISTORY_FILE;
    }
    break;
  }

  //
  // Process Connection Arguments
  //
  QStringList f0=cmd->key(cmd->keys()-1).split(":");
  if(f0.size()>2) {
    fprintf(stderr,"lwcp: invalid argument\n");
    exit(256);
  }
  hostname=f0[0];
  if(f0.size()==2) {
    port=f0[1].toUInt(&ok);
    if((!ok)||(port==0)) {
      fprintf(stderr,"lwcp: invalid port value\n");
      exit(256);
    }
  }

  //
  // UI Elements
  //
  lw_text=new QTextEdit(this);
  lw_text->setReadOnly(true);
  lw_text->setFocusPolicy(Qt::NoFocus);

  lw_edit=new LineEdit(this);
  connect(lw_edit,SIGNAL(returnPressed()),this,SLOT(editReturnPressedData()));
  lw_edit->loadHistory(lw_history_path);

  //
  // Socket
  //
  lw_tcp_socket=new QTcpSocket(this);
  connect(lw_tcp_socket,SIGNAL(connected()),this,SLOT(tcpConnectedData()));
  connect(lw_tcp_socket,SIGNAL(disconnected()),
	  this,SLOT(tcpDisconnectedData()));
  connect(lw_tcp_socket,SIGNAL(readyRead()),this,SLOT(tcpReadyReadData()));
  connect(lw_tcp_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(tcpErrorData(QAbstractSocket::SocketError)));
  lw_tcp_socket->connectToHost(hostname,port);

  setMinimumSize(sizeHint());
}


QSize MainWidget::sizeHint() const
{
  return QSize(640,480);
}


void MainWidget::closeEvent(QCloseEvent *e)
{
  if(CheckSettingsDirectory()) {
    lw_edit->saveHistory(lw_history_path);
  }
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  lw_text->setGeometry(0,0,size().width(),size().height()-24);
  lw_edit->setGeometry(0,size().height()-24,size().width(),24);
}


void MainWidget::editReturnPressedData()
{
  lw_text->append(FormatLwcp(lw_edit->text(),true));
  lw_tcp_socket->write(lw_edit->text().toUtf8()+"\r\n",
			 lw_edit->text().length()+2);
  lw_edit->setText("");
}


void MainWidget::tcpReadyReadData()
{
  char data[1500];
  int n;

  while((n=lw_tcp_socket->read(data,1500))>0) {
    for(int i=0;i<n;i++) {
      switch(data[i]) {
      case 10:
	break;

      case 13:
	ProcessCommand(lw_accum);
	lw_accum="";
	break;

      default:
	lw_accum+=data[i];
	break;
      }
    }
  }
}


void MainWidget::tcpConnectedData()
{
}


void MainWidget::tcpDisconnectedData()
{
  QMessageBox::information(this,"LWCP - "+tr("Network Event"),
			   tr("Remote host disconnected."));
  exit(0);
}


void MainWidget::tcpErrorData(QAbstractSocket::SocketError err)
{
  QString err_text;

  switch(err) {
  case QAbstractSocket::ConnectionRefusedError:
    err_text=tr("Connection refused");
    break;

  case QAbstractSocket::RemoteHostClosedError:
    err_text=tr("Remote host closed connection");
    break;

  case QAbstractSocket::HostNotFoundError:
    err_text=tr("Host not found");
    break;

  case QAbstractSocket::SocketTimeoutError:
    err_text=tr("Connection timed out");
    break;

  default:
    err_text=tr("Network error")+QString().sprintf(" %u ",err)+tr("received");
    break;
  }
  QMessageBox::critical(this,"LWCP - "+tr("Network Error"),err_text);
  exit(256);
}


void MainWidget::ProcessCommand(const QString &cmd)
{
  lw_text->append(FormatLwcp(cmd,false));
}


QString MainWidget::FormatLwcp(const QString &str,bool local)
{
  QString ret;

  if(local) {
    ret+="<strong>";
  }
  ret+=str;
  if(local) {
    ret+="</strong>";
  }

  return ret;
}


bool MainWidget::CheckSettingsDirectory()
{
  QString path=QString("/")+LWMON_SETTINGS_DIR;

  if(getenv("HOME")!=NULL) {
    path=QString(getenv("HOME"))+"/"+LWMON_SETTINGS_DIR;
  }
  lw_settings_dir=new QDir(path);
  if(!lw_settings_dir->exists()) {
    mkdir(path.toUtf8(),
	  S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    if(!lw_settings_dir->exists()) {
      return false;
    }
  }
  return true;
}


MainWidget::Mode MainWidget::SetMode() const
{
  MainWidget::Mode mode=LWMON_DEFAULT_MODE;

  QStringList f0=QString(qApp->argv()[0]).split("/");
  if(f0.back()=="lwcp") {
    mode=MainWidget::Lwcp;
  }
  if(f0.back()=="lwrp") {
    mode=MainWidget::Lwrp;
  }

  return mode;
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
