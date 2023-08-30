// udpmon.cpp
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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <QApplication>
#include <QHostAddress>
#include <QMessageBox>
#include <QNetworkDatagram>
#include <QStringList>

#include "cmdswitch.h"
#include "udpmon.h"

MainWidget::MainWidget(QWidget *parent)
  : QMainWindow(parent)
{
  int receive_port=0;
  int send_port=0;
  int send_to_port=0;
  QHostAddress send_to_address;
  bool ok=false;

  d_recv_socket=NULL;
  d_send_socket=NULL;

  //
  // Process Arguments
  //
  CmdSwitch *cmd=new CmdSwitch("udpmon",UDPMON_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--send-port") {
      send_port=cmd->value(i).toInt(&ok);
      if((!ok)||(send_port<0)||(send_port>65536)) {
	QMessageBox::critical(this,"UDPMon - "+tr("Argument Error"),
			      tr("Invalid --send-port value."));
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--send-to-address") {
      if(!send_to_address.setAddress(cmd->value(i))) {
	QMessageBox::critical(this,"UDPMon - "+tr("Argument Error"),
			      tr("Invalid --send-to-address value."));
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--send-to-port") {
      send_to_port=cmd->value(i).toInt(&ok);
      if((!ok)||(send_to_port<0)||(send_to_port>65536)) {
	QMessageBox::critical(this,"UDPMon - "+tr("Argument Error"),
			      tr("Invalid --send-to-port value."));
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--receive-port") {
      receive_port=cmd->value(i).toInt(&ok);
      if((!ok)||(receive_port<0)||(receive_port>65536)) {
	QMessageBox::critical(this,"UDPMon - "+tr("Argument Error"),
			      tr("Invalid --receive-port value."));
	exit(1);
      }
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"udpmon: invalid argument \"%s\"\n",
	      (const char *)cmd->key(i).toUtf8());
      exit(256);
    }
  }

  setWindowTitle(tr("UDP Monitor"));
  if(CheckSettingsDirectory()) {
    d_history_path=d_settings_dir->path()+"/"+UDPMON_HISTORY_FILE;
  }

  //
  // UI Elements
  //
  d_text=new QTextEdit(this);
  d_text->setReadOnly(true);
  d_text->setFocusPolicy(Qt::NoFocus);

  d_edit=new LineEdit(this);
  connect(d_edit,SIGNAL(returnPressed()),this,SLOT(editReturnPressedData()));
  d_edit->loadHistory(d_history_path);

  d_button=new QPushButton(tr("&Clear"),this);
  d_button->setFocusPolicy(Qt::NoFocus);
  connect(d_button,SIGNAL(clicked()),d_text,SLOT(clear()));

  //
  // Receive Port
  //
  d_recvport_label=new QLabel(tr("Receive Port")+":",this);
  d_recvport_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_recvport_spin=new QSpinBox(this);
  d_recvport_spin->setRange(0,65536);
  d_recvport_spin->setSpecialValueText(tr("None"));
  connect(d_recvport_spin,SIGNAL(valueChanged(int)),
	  this,SLOT(recvPortChangedData(int)));
  d_recvport_bind_button=new QPushButton(tr("Bind"),this);
  d_recvport_bind_button->setDisabled(true);
  connect(d_recvport_bind_button,SIGNAL(clicked()),
	  this,SLOT(recvPortBindData()));

  //
  // Send Port
  //
  d_sendport_label=new QLabel(tr("Send Port")+":",this);
  d_sendport_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_sendport_spin=new QSpinBox(this);
  d_sendport_spin->setRange(0,65536);
  d_sendport_spin->setSpecialValueText(tr("Auto"));
  connect(d_sendport_spin,SIGNAL(valueChanged(int)),
	  this,SLOT(sendPortChangedData(int)));
  d_sendport_bind_button=new QPushButton(tr("Bind"),this);
  d_sendport_bind_button->setDisabled(true);
  connect(d_sendport_bind_button,SIGNAL(clicked()),
	  this,SLOT(sendPortBindData()));
  d_sendtoaddr_label=new QLabel(tr("To")+":",this);
  d_sendtoaddr_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  d_sendtoaddr_edit=new QLineEdit(this);
  d_sendtoport_label=new QLabel(":",this);
  d_sendtoport_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  d_sendtoport_spin=new QSpinBox(this);
  d_sendtoport_spin->setRange(1,65536);

  //
  // Initialize
  //
  d_recvport_spin->setValue(receive_port);
  recvPortBindData();

  d_sendport_spin->setValue(send_port);
  sendPortBindData();

  d_sendtoport_spin->setValue(send_to_port);
  d_sendtoaddr_edit->setText(send_to_address.toString());

  setMinimumSize(sizeHint());
}


QSize MainWidget::sizeHint() const
{
  return QSize(720+105,480);
}


void MainWidget::closeEvent(QCloseEvent *e)
{
  if(CheckSettingsDirectory()) {
    d_edit->saveHistory(d_history_path);
  }
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  d_text->setGeometry(0,0,size().width(),size().height()-48);
  d_edit->setGeometry(0,size().height()-48,size().width()-65,24);
  d_button->setGeometry(size().width()-60,size().height()-48,60,24);

  d_recvport_label->setGeometry(0,size().height()-24,100,24);
  d_recvport_spin->setGeometry(105,size().height()-23,80,22);
  d_recvport_bind_button->setGeometry(190,size().height()-24,50,24);

  d_sendport_label->setGeometry(240,size().height()-24,100,24);
  d_sendport_spin->setGeometry(350,size().height()-23,80,22);
  d_sendport_bind_button->setGeometry(430+5,size().height()-24,50,24);

  d_sendtoaddr_label->setGeometry(480+20,size().height()-24,30,24);
  d_sendtoaddr_edit->setGeometry(515+20,size().height()-22,140,20);
  d_sendtoport_label->setGeometry(655+20,size().height()-24,5,24);
  d_sendtoport_spin->setGeometry(660+20,size().height()-23,80,22);
}


void MainWidget::editReturnPressedData()
{
  QHostAddress addr;

  if(!addr.setAddress(d_sendtoaddr_edit->text())) {
    QMessageBox::warning(this,"UDPMon - "+tr("Error"),
			 tr("The send address is invalid."));
    return;
  }
  d_text->append(d_edit->text());
  d_send_socket->writeDatagram(d_edit->text().toUtf8(),
			       addr,d_sendtoport_spin->value());
  d_edit->setText("");
}


void MainWidget::recvPortChangedData(int port)
{
  d_recvport_bind_button->
    setEnabled((d_recv_socket==NULL)||
	       (d_recv_socket->localPort()!=d_recvport_spin->value()));
}


void MainWidget::recvPortBindData()
{
  if(d_recv_socket!=NULL) {
    delete d_recv_socket;
    d_recv_socket=NULL;
  }
  if(d_recvport_spin->value()>0) {
    d_recv_socket=new QUdpSocket(this);
    connect(d_recv_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
    connect(d_recv_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	    this,SLOT(errorData(QAbstractSocket::SocketError)));
    if(!d_recv_socket->bind(d_recvport_spin->value())) {
      QMessageBox::warning(this,"UDPMon - "+tr("Error"),
			   tr("Unable to bind receive port."));
      return;
    }
  }
  d_recvport_bind_button->setDisabled(true);
}


void MainWidget::readyReadData()
{
  QNetworkDatagram dgram=d_recv_socket->receiveDatagram(1500);
  d_text->append("<strong>"+QString::fromUtf8(dgram.data())+"</strong>\n");
}


void MainWidget::sendPortChangedData(int port)
{
  d_sendport_bind_button->
    setEnabled((d_send_socket==NULL)||
	       (d_send_socket->localPort()!=d_sendport_spin->value()));
}


void MainWidget::sendPortBindData()
{
  if((d_send_socket!=NULL)&&(d_send_socket!=d_recv_socket)) {
    delete d_send_socket;
    d_send_socket=NULL;
  }
  if((d_sendport_spin->value()>0)&&
     (d_sendport_spin->value()==d_recvport_spin->value())) {
    d_send_socket=d_recv_socket;
  }
  else {
    d_send_socket=new QUdpSocket(this);
    if(d_sendport_spin->value()>0) {
      if(!d_send_socket->bind(d_sendport_spin->value())) {
	QMessageBox::warning(this,"UDPMon - "+tr("Error"),
			     tr("Unable to bind send port."));
	return;
      }
    }
  }
  d_sendport_bind_button->setDisabled(true);
}


void MainWidget::errorData(QAbstractSocket::SocketError err)
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

  case QAbstractSocket::DatagramTooLargeError:
    err_text=tr("Datagram too large");
    break;

  case QAbstractSocket::NetworkError:
    err_text=tr("Network error");
    break;

  case QAbstractSocket::AddressInUseError:
    err_text=tr("Address in use");
    break;

  case QAbstractSocket::SocketAddressNotAvailableError:
    err_text=tr("Socket address not available");
    break;

  case QAbstractSocket::UnsupportedSocketOperationError:
    err_text=tr("Unsupported socket operation");
    break;

  default:
    err_text=tr("Network error")+QString::asprintf(" %u ",err)+tr("received");
    break;
  }
  QMessageBox::critical(this,"UDP Monitor - "+tr("Network Error"),err_text);
  //  d_status_widget->setStatus(StatusWidget::Failed);
}


void MainWidget::ProcessCommand(const QString &cmd)
{
  //  d_text->append(FormatLwcp(Colorize(cmd),false));
}


bool MainWidget::CheckSettingsDirectory()
{
#ifndef WIN32
  QString path=QString("/")+UDPMON_SETTINGS_DIR;

  if(getenv("HOME")!=NULL) {
    path=QString(getenv("HOME"))+"/"+UDPMON_SETTINGS_DIR;
  }
  d_settings_dir=new QDir(path);
  if(!d_settings_dir->exists()) {
    mkdir(path.toUtf8(),
    	  S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    if(!d_settings_dir->exists()) {
      return false;
    }
  }
#endif  // WIN32
  return true;
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
