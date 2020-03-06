// lwmastermon.cpp
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

#include <errno.h>
#include <unistd.h>

#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <QApplication>
#include <QMessageBox>

#include "lwmastermon.h"

QHostAddress master_address(LWMASTERMON_MASTER_ADDR);

MainWidget::MainWidget(QWidget *parent)
  : QMainWindow(parent,Qt::WindowStaysOnTopHint)
{
  int sock=-1;
  struct sockaddr_ll sl;

  mon_notifier=NULL;

  setWindowTitle("Master");
  setMaximumSize(sizeHint());
  setMinimumSize(sizeHint());

  QFont bold_font(font().family(),font().pointSize(),QFont::Bold);

  mon_label=new QLabel(tr("Livewire Master Node"),this);
  mon_label->setFont(bold_font);
  mon_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);

  mon_value_label=new QLabel(this);
  mon_value_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);

  //
  // Open the NetLink Interface
  //
  if((sock=socket(AF_PACKET,SOCK_RAW,htons(ETH_P_IP)))<0) {
    QMessageBox::warning(this,tr("Master"),
			 tr("Unable to open the NetLink interface")+"\n"+
			 "["+strerror(errno)+"]");
    exit(256);
  }

  memset(&sl,0,sizeof(sl));
  sl.sll_family=AF_PACKET;
  sl.sll_protocol=htons(ETH_P_IP);
  sl.sll_ifindex=3;
  if(bind(sock,(struct sockaddr *)(&sl),sizeof(sl))<0) {
    fprintf(stderr,"lwmastermon: bind failed [%s]\n",strerror(errno));
  }

  mon_notifier=new QSocketNotifier(sock,QSocketNotifier::Read,this);
  Subscribe();
  connect(mon_notifier,SIGNAL(activated(int)),this,SLOT(activatedData(int)));

  //
  // Watchdog
  //
  mon_watchdog_timer=new QTimer(this);
  mon_watchdog_timer->setSingleShot(true);
  connect(mon_watchdog_timer,SIGNAL(timeout()),this,SLOT(watchdogData()));
  mon_watchdog_timer->start(LWMASTERMON_WATCHDOG_INTERVAL);
}


QSize MainWidget::sizeHint() const
{
  return QSize(200,60);
}


void MainWidget::activatedData(int sock)
{
  char data[1501];
  QHostAddress addr;

  if(recv(sock,data,1500,0)>=0) {
    if(IsAddress(master_address,data,30)) {
      UpdateWatchdog(IpAddress(data,26));
    }
  }
}


void MainWidget::UpdateWatchdog(const QHostAddress &addr)
{
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


bool MainWidget::IsAddress(const QHostAddress &addr,const char *data,int offset)
  const
{
  uint32_t raw=((0xff&data[offset])<<24)+((0xff&data[offset+1])<<16)+
    ((0xff&data[offset+2])<<8)+(0xff&data[offset+3]);
  return addr.toIPv4Address()==raw;
}


QHostAddress MainWidget::IpAddress(const char *data,int offset) const
{
  QHostAddress addr;
  uint32_t raw=((0xff&data[offset])<<24)+((0xff&data[offset+1])<<16)+
    ((0xff&data[offset+2])<<8)+(0xff&data[offset+3]);

  addr.setAddress(raw);

  return addr;
}


void MainWidget::DumpIpAddress(const char *data,int offset) const
{
  printf("IPv4 Address: %s [%02X %02X %02X %02X]\n",
	 IpAddress(data,offset).toString().toUtf8().constData(),
	 0xff&data[offset],0xff&data[offset+1],
	 0xff&data[offset+2],0xff&data[offset+3]);
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
  int sock;
  struct ifreq ifr;

  if((sock=socket(AF_INET,SOCK_DGRAM,0))<0) {
    QMessageBox::warning(this,tr("Master"),
			 tr("Unable to create subscription socket")+"\n"+
			 "["+strerror(errno)+"]");
    exit(255);
  }

  memset(&ifr,0,sizeof(ifr));
  ifr.ifr_ifindex=1;
  while(ioctl(sock,SIOCGIFNAME,&ifr)==0) {
    Subscribe(sock,ifr.ifr_ifindex);
    ifr.ifr_ifindex++;
  }
}


void MainWidget::Subscribe(int sock,int index)
{
  struct ip_mreqn mreq;
  struct packet_mreq preq;

  //
  // IP Subscribe
  //
  memset(&mreq,0,sizeof(mreq));
  mreq.imr_multiaddr.s_addr=
    htonl(QHostAddress(LWMASTERMON_MASTER_ADDR).toIPv4Address());
  mreq.imr_ifindex=index;
  if(setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq))<0) {
    QMessageBox::warning(this,tr("Master"),
			 tr("Unable to subscribe to multicast address")+
			 " \""+LWMASTERMON_MASTER_ADDR+"\" ["+
			 strerror(errno)+"]");
    exit(255);
  }

  //
  // Set Promiscuous Mode
  //
  memset(&preq,0,sizeof(preq));
  preq.mr_ifindex=index;
  preq.mr_type=PACKET_MR_PROMISC;
  if(setsockopt(mon_notifier->socket(),SOL_PACKET,PACKET_ADD_MEMBERSHIP,
		&preq,sizeof(preq))<0) {
    QMessageBox::warning(this,tr("Master"),
			 tr("Unable to set promiscuous mode")+"\n"+
			 "["+strerror(errno)+"]");
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
