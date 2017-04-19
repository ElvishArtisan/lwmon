// lwmastermon.cpp
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

#include <errno.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <QApplication>
#include <QMessageBox>

#include "lwmastermon.h"

MainWidget::MainWidget(QWidget *parent)
  : QMainWindow(parent,Qt::WindowStaysOnTopHint)
{
  setWindowTitle("Master");
  setMaximumSize(sizeHint());
  setMinimumSize(sizeHint());

  QFont bold_font(font().family(),font().pointSize(),QFont::Bold);

  mon_label=new QLabel(tr("LiveWire Master Node"),this);
  mon_label->setFont(bold_font);
  mon_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);

  mon_value_label=new QLabel(this);
  mon_value_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);

  mon_socket=new QUdpSocket(this);
  if(!mon_socket->bind(LWMASTERMON_MASTER_PORT)) {
    QMessageBox::warning(this,tr("Master"),
			 tr("Unable to bind UDP port 7000."));
    exit(256);
  }
  Subscribe();
  connect(mon_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));

  mon_watchdog_timer=new QTimer(this);
  mon_watchdog_timer->setSingleShot(true);
  connect(mon_watchdog_timer,SIGNAL(timeout()),this,SLOT(watchdogData()));
  mon_watchdog_timer->start(LWMASTERMON_WATCHDOG_INTERVAL);
}


QSize MainWidget::sizeHint() const
{
  return QSize(200,60);
}


void MainWidget::readyReadData()
{
  char data[1501];
  int n;
  QHostAddress addr;

  while((n=mon_socket->readDatagram(data,1500,&addr))>0) {
    mon_value_label->setText(addr.toString());
    if(mon_watchdog_timer->isActive()) {
      mon_watchdog_timer->stop();
    }
    else {
      mon_value_label->setFont(font());
      mon_value_label->setStyleSheet("");
    }
    mon_watchdog_timer->start(LWMASTERMON_WATCHDOG_INTERVAL);
  }
}


void MainWidget::watchdogData()
{
  QFont failed_font(font().family(),font().pointSize(),QFont::Bold);
  mon_value_label->setText(tr("No Master Found!"));
  mon_value_label->setStyleSheet("color: red;");
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  mon_label->setGeometry(10,10,size().width()-20,20);
  mon_value_label->setGeometry(10,32,size().width()-20,20);
}


void MainWidget::Subscribe()
{
  struct ifreq ifr;

  memset(&ifr,0,sizeof(ifr));
  ifr.ifr_ifindex=1;
  while(ioctl(mon_socket->socketDescriptor(),SIOCGIFNAME,&ifr)==0) {
    Subscribe(ifr.ifr_ifindex);
    ifr.ifr_ifindex++;
  }
}


void MainWidget::Subscribe(int index)
{
  struct ip_mreqn mreq;

  memset(&mreq,0,sizeof(mreq));
  mreq.imr_multiaddr.s_addr=
    htonl(QHostAddress(LWMASTERMON_MASTER_ADDR).toIPv4Address());
  mreq.imr_ifindex=index;
  if(setsockopt(mon_socket->socketDescriptor(),IPPROTO_IP,IP_ADD_MEMBERSHIP,
		&mreq,sizeof(mreq))<0) {
    QMessageBox::warning(this,tr("Master"),
			 tr("Unable to subscribe to multicast address")+
			 " \""+LWMASTERMON_MASTER_ADDR+"\" ["+
			 strerror(errno)+"]");
    exit(255);
  }
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);

  MainWidget *w=new MainWidget();
  w->show();

  return a.exec();
}
