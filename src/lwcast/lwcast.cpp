// lwcast.h
//
// Multicast Subscription Monitor
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

#include <stdio.h>
#include <stdlib.h>

#include <QApplication>
#include <QMessageBox>

#include "cmdswitch.h"
#include "lwcast.h"

MainWidget::MainWidget(QWidget *parent)
  : QMainWindow(parent)
{
  CmdSwitch *cmd=new CmdSwitch("lwcast",LWCAST_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(!cmd->processed(i)) {
      fprintf(stderr,"lwcast: unknown option\n");
      exit(256);
    }
  }

  //
  // Fonts
  //
  QFont bold_font(font().family(),font().pointSize(),QFont::Bold);

  //
  // Dialogs
  //
  main_addaddress_dialog=new AddAddressDialog(this);

  //
  // Address List
  //
  main_address_model=new SubscriptionModel(this);
  connect(main_address_model,SIGNAL(error(const QString &)),
	  this,SLOT(errorData(const QString &)));
  main_address_list=new QTableView(this);
  main_address_list->setShowGrid(false);
  main_address_list->setWordWrap(false);
  main_address_list->setSelectionMode(QAbstractItemView::SingleSelection);
  main_address_list->setSelectionBehavior(QAbstractItemView::SelectRows);
  main_address_list->setModel(main_address_model);

  //
  // Add Button
  //
  main_add_button=new QPushButton(tr("Add"),this);
  main_add_button->setFont(bold_font);
  connect(main_add_button,SIGNAL(clicked()),this,SLOT(addData()));

  //
  // Remove Button
  //
  main_remove_button=new QPushButton(tr("Remove"),this);
  main_remove_button->setFont(bold_font);
  connect(main_remove_button,SIGNAL(clicked()),this,SLOT(removeData()));

  setWindowTitle(tr("Mcast Subscriptions"));
}


QSize MainWidget::sizeHint() const
{
  return QSize(300,150);
}


void MainWidget::addData()
{
  QHostAddress addr;
  QHostAddress if_addr;
  QString if_name;
  if(main_addaddress_dialog->exec(&addr,&if_addr,&if_name)) {
    main_address_model->addAddress(addr,if_addr,if_name);
    main_address_list->resizeColumnsToContents();
  }
}


void MainWidget::removeData()
{
  QItemSelectionModel *s=main_address_list->selectionModel();
  if(s->hasSelection()) {
    main_address_model->removeAddress(s->selectedRows()[0]);
  }
}


void MainWidget::errorData(const QString &msg)
{
  QMessageBox::critical(this,tr("Mcast Subscriptions"),msg);
}


void MainWidget::closeEvent(QCloseEvent *e)
{
  exit(0);
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  main_address_list->setGeometry(10,10,size().width()-20,size().height()-60);

  main_add_button->setGeometry(size().width()-160,size().height()-35,70,30);
  main_remove_button->setGeometry(size().width()-80,size().height()-35,70,30);
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *w=new MainWidget();
  w->show();
  return a.exec();
}
