// addaddressdialog.cpp
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

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <unistd.h>

#include <QMessageBox>
#include <QStringList>

#include "addaddressdialog.h"

AddAddressDialog::AddAddressDialog(QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Mcast Subscriptions - Add"));

  //
  // Fonts
  //
  QFont bold_font(font().family(),font().pointSize(),QFont::Bold);

  //
  // Interface
  //
  add_interface_label=new QLabel(tr("Interface")+":",this);
  add_interface_label->setFont(bold_font);
  add_interface_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  add_interface_box=new QComboBox(this);

  //
  // Address
  //
  add_address_label=new QLabel(tr("Multicast Address")+":",this);
  add_address_label->setFont(bold_font);
  add_address_label->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  add_address_edit=new QLineEdit(this);
  add_address_edit->setMaxLength(15);

  //
  // Ok Button
  //
  add_ok_button=new QPushButton(tr("OK"),this);
  add_ok_button->setFont(bold_font);
  connect(add_ok_button,SIGNAL(clicked()),this,SLOT(okData()));

  //
  // Cancel Button
  //
  add_cancel_button=new QPushButton(tr("Cancel"),this);
  add_cancel_button->setFont(bold_font);
  connect(add_cancel_button,SIGNAL(clicked()),this,SLOT(cancelData()));
}


QSize AddAddressDialog::sizeHint() const
{
  return QSize(400,100);
}


int AddAddressDialog::exec(QHostAddress *addr,QHostAddress *if_addr,
			   QString *if_name)
{
  add_mcast_address=addr;
  add_iface_address=if_addr;
  add_iface_name=if_name;
  PopulateInterfaces(add_interface_box);
  add_address_edit->setText("");

  return QDialog::exec();
}


void AddAddressDialog::okData()
{
  QHostAddress addr(add_address_edit->text());

  if(addr.isNull()) {
    QMessageBox::critical(this,tr("MCast Subscriptions - Error"),
			  tr("That IP address is invalid."));
    return;
  }
  uint32_t octet=addr.toIPv4Address()>>24;
  if((octet<224)||(octet>239)) {
    QMessageBox::critical(this,tr("MCast Subscriptions - Error"),
			  tr("That is not a valid multicast IP address."));
    return;
  }
  *add_mcast_address=addr;
  QStringList f0=add_interface_box->currentText().split(" - ");
  *add_iface_name=f0.at(0);
  add_iface_address->setAddress(f0.at(1));
  done(true);
}


void AddAddressDialog::cancelData()
{
  done(false);
}


void AddAddressDialog::closeEvent(QCloseEvent *e)
{
  cancelData();
}


void AddAddressDialog::resizeEvent(QResizeEvent *e)
{
  add_interface_label->setGeometry(10,10,150,20);
  add_interface_box->setGeometry(165,10,size().width()-175,20);

  add_address_label->setGeometry(10,32,150,20);
  add_address_edit->setGeometry(165,32,size().width()-175,20);

  add_ok_button->setGeometry(size().width()-160,size().height()-35,70,30);
  add_cancel_button->setGeometry(size().width()-80,size().height()-35,70,30);
}


void AddAddressDialog::PopulateInterfaces(QComboBox *box)
{
  QList<uint64_t> mac_addrs;
  QList<QHostAddress> ip4_addrs;
  QList<QString> if_names;

  GetInterfaceInfo(&mac_addrs,&ip4_addrs,&if_names);
  add_interface_box->clear();
  for(int i=0;i<if_names.size();i++) {
    add_interface_box->insertItem(-1,if_names.at(i)+" - "+ip4_addrs.at(i).toString());
  }
}


unsigned AddAddressDialog::GetInterfaceInfo(QList<uint64_t> *mac_addrs,
					    QList<QHostAddress> *ip4_addrs,
					    QList<QString> *if_names)
{
  int fd;
  struct ifreq ifr;
  int index=0;
  unsigned n=0;
  uint64_t mac;
  uint64_t accum;
  sockaddr_in *sa=NULL;

  if((fd=socket(PF_INET,SOCK_DGRAM,IPPROTO_IP))<0) {
    return n;
  }

  memset(&ifr,0,sizeof(ifr));
  index=1;
  ifr.ifr_ifindex=index;
  while(ioctl(fd,SIOCGIFNAME,&ifr)==0) {
    if(ioctl(fd,SIOCGIFHWADDR,&ifr)==0) {
      mac=0;
      for(unsigned i=0;i<6;i++) {
	accum=((uint64_t)(0xFF&ifr.ifr_ifru.ifru_hwaddr.sa_data[i]));
	mac=mac|(accum<<(40-8*i));
      }
      if(mac!=0) {
	n++;
	mac_addrs->push_back(mac);
      }
      else {
	mac_addrs->push_back(0);
      }
      if(if_names!=NULL) {
	if_names->push_back(QString(ifr.ifr_name));
      }
      if(ip4_addrs!=NULL) {
	ip4_addrs->push_back(QHostAddress());
	if(ioctl(fd,SIOCGIFADDR,&ifr)==0) {
	  sa=(struct sockaddr_in *)(&(ifr.ifr_addr));
	  ip4_addrs->back().setAddress(ntohl(sa->sin_addr.s_addr));
	}
      }
    }
    ifr.ifr_ifindex=++index;
  }
  ::close(fd);

  return n;
}
