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
  lw_port=0;
  bool ok=false;

  //
  // Process Arguments
  //
  lw_colorize=true;
  lw_mode=SetMode();
  CmdSwitch *cmd=new CmdSwitch(qApp->argc(),qApp->argv(),"lwmon",LWMON_USAGE);
  if(cmd->keys()==0) {
    fprintf(stderr,"%s\n",LWMON_USAGE);
    exit(256);
  }
  for(unsigned i=0;i<cmd->keys()-1;i++) {
    if(cmd->key(i)=="--color") {
      if((cmd->value(i).toLower()=="off")||
	 (cmd->value(i).toLower()=="no")||
	 (cmd->value(i).toLower()=="false")) {
	lw_colorize=false;
	cmd->setProcessed(i,true);
      }
      if((cmd->value(i).toLower()=="on")||
	 (cmd->value(i).toLower()=="yes")||
	 (cmd->value(i).toLower()=="true")) {
	lw_colorize=true;
	cmd->setProcessed(i,true);
      }
      if(!cmd->processed(i)) {
	fprintf(stderr,"lwmon: invalid argument to --color switch\n");
	exit(256);
      }
    }
    if(cmd->key(i)=="--mode") {
      if(cmd->value(i).toLower()=="lwcp") {
	lw_mode=MainWidget::Lwcp;
	cmd->setProcessed(i,true);
      }
      if(cmd->value(i).toLower()=="lwrp") {
	lw_mode=MainWidget::Lwrp;
	cmd->setProcessed(i,true);
      }
      if(cmd->value(i).toLower()=="lwaddr") {
	lw_mode=MainWidget::Lwaddr;
	cmd->setProcessed(i,true);
      }
      if(!cmd->processed(i)) {
	fprintf(stderr,"lwmon: invalid \"--mode\" argument");
	exit(256);
      }
    }
    if(cmd->key(i)=="--port") {
      lw_port=cmd->value(i).toUInt(&ok);
      if(!ok) {
	fprintf(stderr,"lwmon: invalid port argument\n");
	exit(256);
      }
      cmd->setProcessed(i,true);
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
    if(lw_port==0) {
      lw_port=LWMON_LWCP_DEFAULT_PORT;
    }
    setWindowTitle(tr("LWCP Monitor"));
    if(CheckSettingsDirectory()) {
      lw_history_path=lw_settings_dir->path()+"/"+LWMON_LWCP_HISTORY_FILE;
    }
    break;

  case MainWidget::Lwrp:
    if(lw_port==0) {
      lw_port=LWMON_LWRP_DEFAULT_PORT;
    }
    setWindowTitle(tr("LWRP Monitor"));
    if(CheckSettingsDirectory()) {
      lw_history_path=lw_settings_dir->path()+"/"+LWMON_LWRP_HISTORY_FILE;
    }
    break;

  case MainWidget::Lwaddr:
    PrintAddr(cmd->key(cmd->keys()-1));
    break;
  }

  //
  // Process Connection Arguments
  //
  lw_hostname=cmd->key(cmd->keys()-1);

  //
  // UI Elements
  //
  lw_text=new QTextEdit(this);
  lw_text->setReadOnly(true);
  lw_text->setFocusPolicy(Qt::NoFocus);

  lw_edit=new LineEdit(this);
  connect(lw_edit,SIGNAL(returnPressed()),this,SLOT(editReturnPressedData()));
  lw_edit->loadHistory(lw_history_path);

  lw_status_frame_widget=new QLabel(this);
  lw_status_frame_widget->setFrameStyle(QFrame::Box|QFrame::Raised);
  lw_status_widget=new StatusWidget(this);
  lw_status_widget->setStatus(StatusWidget::Connecting);

  lw_button=new QPushButton(tr("&Clear"),this);
  lw_button->setFocusPolicy(Qt::NoFocus);
  connect(lw_button,SIGNAL(clicked()),lw_text,SLOT(clear()));

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
  lw_tcp_socket->connectToHost(lw_hostname,lw_port);

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
  lw_edit->setGeometry(0,size().height()-24,size().width()-195,24);
  lw_status_frame_widget->
    setGeometry(size().width()-185,size().height()-24,125,24);
  lw_status_widget->setGeometry(size().width()-182,size().height()-21,119,18);
#ifdef OSX
  lw_button->setGeometry(size().width()-60,size().height()-28,60,35);
#else
  lw_button->setGeometry(size().width()-60,size().height()-24,60,24);
#endif  // OSX
}


void MainWidget::editReturnPressedData()
{
  lw_text->append(FormatLwcp(Colorize(lw_edit->text()),true));
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
  switch(lw_mode) {
  case MainWidget::Lwcp:
    if(lw_port==LWMON_LWCP_DEFAULT_PORT) {
      setWindowTitle(tr("LWCP Monitor")+" - "+lw_hostname);
    }
    else {
      setWindowTitle(tr("LWCP Monitor")+" - "+lw_hostname+
		     QString().sprintf(":%u",lw_port));
    }
    break;

  case MainWidget::Lwrp:
    if(lw_port==LWMON_LWRP_DEFAULT_PORT) {
      setWindowTitle(tr("LWRP Monitor")+" - "+lw_hostname);
    }
    else {
      setWindowTitle(tr("LWRP Monitor")+" - "+lw_hostname+
		     QString().sprintf(":%u",lw_port));
    }
    break;

  case MainWidget::Lwaddr:
    break;
  }
  lw_status_widget->setStatus(StatusWidget::Connected);
}


void MainWidget::tcpDisconnectedData()
{
  QMessageBox::information(this,"LWCP - "+tr("Network Event"),
			   tr("Remote host disconnected."));
  lw_status_widget->setStatus(StatusWidget::Failed);
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
    err_text=tr("Network error")+QString().sprintf(" %u ",err)+tr("received");
    break;
  }
  switch(lw_mode) {
  case MainWidget::Lwcp:
    QMessageBox::critical(this,"LWCP - "+tr("Network Error"),err_text);
    break;

  case MainWidget::Lwrp:
    QMessageBox::critical(this,"LWRP - "+tr("Network Error"),err_text);
    break;

  case MainWidget::Lwaddr:
    break;
  }
  lw_status_widget->setStatus(StatusWidget::Failed);
}


void MainWidget::ProcessCommand(const QString &cmd)
{
  lw_text->append(FormatLwcp(Colorize(cmd),false));
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
#ifndef WIN32
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
#endif  // WIN32
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
  if(f0.back()=="lwaddr") {
    mode=MainWidget::Lwaddr;
  }

  return mode;
}


void MainWidget::PrintMacAddr(const QString &arg) const
{
  bool ok=false;
  uint64_t mac;

  mac=arg.toULongLong(&ok,16);
  if(ok) {
    if((mac&0xFFFFFFFF0000ull)==0x01005e000000ull) {    // Stereo
      if((mac&0x00000000FF00ull)==0xFF00) {
	PrintSpecialChannel(mac&0xFF);
      }
      else {
	PrintAddr(mac&0xFFFF,MainWidget::Stereo);
      }
    }
    if((mac&0xFFFFFFFF0000ull)==0x01005e010000ull) {    // Backfeed
      PrintAddr(mac&0xFFFF,MainWidget::Backfeed);
    }
    if((mac&0xFFFFFFFF0000ull)==0x01005e040000ull) {   // Surround
      PrintAddr(mac&0x7FFF,MainWidget::Surround);
    }
  }
}


void MainWidget::PrintAddr(const QString &arg) const
{
  //
  // Address ranges taken from "Intro to LiveWire v2.1.1", pp 113-114
  //
  unsigned src;
  bool ok;
  QStringList f0;
  unsigned octets[4];
  uint32_t addr;

  //
  // Are we a stream ID?
  //
  if(arg.length()==8) {
    addr=arg.toUInt(&ok,16);
    if(ok) {
      if((addr&0xffff0000)==0xefc00000) {
	if((addr&0x0000ff00)==0xff00) {
	  PrintSpecialChannel(addr&0xff);
	}
	else {
	  PrintAddr(addr&0xffff,MainWidget::Stereo);
	}
      }
      if((addr&0xffff0000)==0xefc40000) {
	PrintAddr(addr&0xffff,MainWidget::Backfeed);
      }
      if((addr&0xffff0000)==0xefc10000) {
	PrintAddr(addr&0x7fff,MainWidget::Surround);
      }
    }
  }

  //
  // Are we a MAC address?
  //
  if(arg.length()==12) {
    PrintMacAddr(arg);
  }
  f0=arg.split(":");
  if(f0.size()==6) {
    PrintMacAddr(f0.join(""));
  }
  f0=arg.split("-");
  if(f0.size()==6) {
    PrintMacAddr(f0.join(""));
  }

  //
  // Are we a source number?
  //
  src=arg.toUInt(&ok);
  if((ok)&&(src>0)&&(src<32768)) {
    PrintAddr(src);
  }

  //
  // Are we an IP address?
  //
  f0=arg.split(".");
  if(f0.size()==4) {
    for(int i=0;i<4;i++) {
      octets[i]=f0[i].toUInt(&ok);
      if((!ok)||(octets[i]>255)) {
	fprintf(stderr,"lwaddr: invalid argument\n");
	exit(256);
      }
    }
    if(octets[0]==239) {
      if((octets[1]==192)&&(octets[2]<128)) {
	PrintAddr(256*octets[2]+octets[3],MainWidget::Stereo);
      }
      if((octets[1]==193)&&(octets[2]<128)) {
	PrintAddr(256*octets[2]+octets[3],MainWidget::Backfeed);
      }
      if((octets[1]==196)&&(octets[2]>=128)) {
	PrintAddr(256*(octets[2]-128)+octets[3],MainWidget::Surround);
      }
      if((octets[1]==192)&&(octets[2]=255)) {
	PrintSpecialChannel(octets[3]);
      }
    }
  }

  fprintf(stderr,"lwaddr: invalid argument\n");
  exit(256);
}


void MainWidget::PrintAddr(unsigned src_num,MainWidget::SignalType type) const
{
  int o3=src_num/256;
  int o4=src_num%256;
  char ipstr[8];
  char backipstr[8];

  snprintf(ipstr,7,"%d.%d",o3,o4);
  snprintf(backipstr,7,"%d.%d",128+o3,o4);
  printf("LiveWire Source # %u\n",src_num);
  if(type==MainWidget::Stereo) {
    printf("   *");
  }
  else {
    printf("    ");
  }
  printf("Stereo Address: 239.192.%-7s  01:00:5e:00:%02x:%02x\n",
	 ipstr,o3,o4);
  if(type==MainWidget::Surround) {
    printf(" *");
  }
  else {
    printf("  ");
  }
  printf("Surround Address: 239.196.%-7s  01:00:5e:04:%02x:%02x\n",
	 backipstr,128+o3,o4);
  if(type==MainWidget::Backfeed) {
    printf(" *");
  }
  else {
    printf("  ");
  }
  printf("Backfeed Address: 239.193.%-7s  01:00:5e:01:%02x:%02x\n",
	 ipstr,o3,o4);
  exit(0);
}


void MainWidget::PrintSpecialChannel(uint8_t last_octet) const
{
  switch(last_octet) {
  case 1:
    printf(" *Livestream clock: 239.192.255.1    01:00:5e:00:ff:01\n");
    exit(0);
    
  case 2:
    printf(" *Standard stream clock: 239.192.255.2    01:00:5e:00:ff:02\n");
    exit(0);
    
  case 3:
    printf(" *Advertisment channel: 239.192.255.3    01:00:5e:00:ff:03\n");
    exit(0);
    
  case 4:
    printf(" *GPIO channel: 239.192.255.4    01:00:5e:00:ff:04\n");
    exit(0);
  }
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
