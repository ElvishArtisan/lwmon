// lwmastermon.cpp
//
// Monitor and display the location of the Livewire master node
//
//   (C) Copyright 2017-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <QApplication>
#include <QFontMetrics>
#include <QMessageBox>
#include <QProcess>
#include <QStringList>

#include "lwmastermon.h"

MainWidget::MainWidget(QWidget *parent)
  : QWidget(parent,(Qt::WindowFlags)(Qt::WindowStaysOnTopHint|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint))
{
  mon_lwrp_socket=NULL;
  mon_width=LWMASTERMON_MIN_WIDTH;

  setWindowTitle(" ");
  setMaximumSize(sizeHint());
  setMinimumSize(sizeHint());

  QFont bold_font(font().family(),font().pointSize(),QFont::Bold);

  mon_label=new QLabel(tr("Livewire Master Node"),this);
  mon_label->setFont(bold_font);
  mon_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  QFontMetrics fm(mon_label->font());
  mon_min_width=fm.horizontalAdvance(mon_label->text())+20;

  mon_value_label=new QLabel(this);
  mon_value_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);

  //
  // Watchdog
  //
  mon_update_timer=new QTimer(this);
  mon_update_timer->setSingleShot(true);
  connect(mon_update_timer,SIGNAL(timeout()),this,SLOT(updateData()));
  mon_update_timer->start(1);
}


QSize MainWidget::sizeHint() const
{
  return QSize(mon_width,60);
}


void MainWidget::updateData()
{
  QProcess *proc=new QProcess(this);
  proc->start("lwmaster",QStringList(),QIODevice::ReadOnly);
  proc->waitForFinished();
  if(proc->exitStatus()!=QProcess::NormalExit) {
    SetLabel(tr("*** ERROR ***"),true);
    fprintf(stderr,"lwmastermon: lwmaster(8) crashed\n");
    mon_current_result=QString();
  }
  else {
    if(proc->exitCode()!=0) {
      SetLabel(tr("*** ERROR ***"),true);
      QString err_msg=proc->readAllStandardError().constData();
      fprintf(stderr,"lwmastermon: lwmaster(8) returned error: \"%s\"\n",
	      err_msg.trimmed().toUtf8().constData());
      mon_current_result=QString();
    }
    else {
      QString result=proc->readAllStandardOutput().constData();
      result=result.trimmed();
      if(result!=mon_current_result) {
	if(result=="0.0.0.0") {
	  SetLabel(tr("No Master Found!"),true);
	}
	else {
	  SetLabel(result,false);

	  //
	  // Lookup Node Name
	  //
	  if(mon_lwrp_socket!=NULL) {
	    mon_lwrp_socket->disconnect();
	    mon_lwrp_socket->deleteLater();
	  }
	  mon_lwrp_socket=new QTcpSocket(this);
	  connect(mon_lwrp_socket,SIGNAL(connected()),
		  this,SLOT(lwrpConnectedData()));
	  connect(mon_lwrp_socket,SIGNAL(readyRead()),
		  this,SLOT(lwrpReadyReadData()));
	  connect(mon_lwrp_socket,SIGNAL(disconnected()),
		  this,SLOT(lwrpDisconnectedData()));
	  connect(mon_lwrp_socket,SIGNAL(error(QAbstractSocket::SocketError)),
		  this,SLOT(lwrpErrorData(QAbstractSocket::SocketError)));
	  mon_lwrp_socket->connectToHost(result,93);
	}

	mon_current_result=result;
      }
    }
  }
  delete proc;

  mon_update_timer->start(LWMASTERMON_UPDATE_INTERVAL);
}


void MainWidget::lwrpConnectedData()
{
  mon_lwrp_socket->write("IP\r\n");
}


void MainWidget::lwrpReadyReadData()
{
  QByteArray data=mon_lwrp_socket->readAll();
  QStringList f0;

  for(int i=0;i<data.length();i++) {
    switch(data.at(i)) {
    case 10:  // Linefeed
      break;

    case 13:  // Carriage return
      //
      // Process response
      //
      f0=QString::fromUtf8(mon_lwrp_accum).split(" ",Qt::SkipEmptyParts);
      for(int j=0;j<(f0.size()-1);j++) {
	if((f0.at(j).toLower()=="hostname")&&
	   (!f0.at(j+1).trimmed().isEmpty())) {
	  SetLabel(f0.at(j+1).trimmed()+" ["+mon_current_result+"]",false);
	}
      }
      mon_lwrp_accum.clear();
      mon_lwrp_socket->disconnectFromHost();
      break;

    default:
      mon_lwrp_accum+=data.at(i);
      break;
    }
  }
}


void MainWidget::lwrpDisconnectedData()
{
  if(mon_lwrp_socket!=NULL) {
    mon_lwrp_socket->disconnect();
    mon_lwrp_socket->deleteLater();
    mon_lwrp_socket=NULL;
  }
}


void MainWidget::lwrpErrorData(QAbstractSocket::SocketError err)
{
  lwrpDisconnectedData();
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  mon_label->setGeometry(10,10,size().width()-20,20);
  mon_value_label->setGeometry(10,32,size().width()-20,20);
}


void MainWidget::SetLabel(const QString &str,bool error)
{
  mon_value_label->setText(str);
  if(error) {
    QFont failed_font(font().family(),font().pointSize(),QFont::Bold);
    mon_value_label->setFont(failed_font);
    mon_value_label->setStyleSheet("color: red;");
  }
  else {
    mon_value_label->setFont(font());
    mon_value_label->setStyleSheet("");
  }

  QFontMetrics fm(mon_value_label->font());
  mon_width=fm.horizontalAdvance(mon_value_label->text())+20;
  if(mon_width<mon_min_width) {
    mon_width=mon_min_width;
  }
  setMaximumSize(sizeHint());
  setMinimumSize(sizeHint());
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);

  MainWidget *w=new MainWidget();
  w->show();

  return a.exec();
}
