// lwmon.cpp
//
// LiveWire Protocol monitor
//
//   (C) Copyright 2015-2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <QApplication>
#include <QCoreApplication>

#include "cmdswitch.h"
#include "lwmon.h"

int main(int argc,char *argv[])
{
  //
  // Process Arguments
  //
  MainWidget::Mode mode;
  bool colorize=true;
  QString write_from_file;
  mode=LWMON_DEFAULT_MODE;
  uint16_t port=93;
  QString password;
  bool ok=false;

  //
  // Get Mode
  //
  QStringList f1=QString::fromUtf8(argv[0]).split("/",Qt::SkipEmptyParts);
  if(f1.last()=="lwaddr") {
    mode=MainWidget::Lwaddr;
  }
  if(f1.last()=="lwcp") {
    mode=MainWidget::Lwcp;
  }
  if(f1.last()=="lwrp") {
    mode=MainWidget::Lwrp;
  }
  
  CmdSwitch *cmd=new CmdSwitch(argc,argv,"lwmon",LWMON_USAGE);
  if(cmd->keys()==0) {
    fprintf(stderr,"%s\n",LWMON_USAGE);
    exit(1);
  }
  for(unsigned i=0;i<cmd->keys()-1;i++) {
    if(cmd->key(i)=="--color") {
      if((cmd->value(i).toLower()=="off")||
	 (cmd->value(i).toLower()=="no")||
	 (cmd->value(i).toLower()=="false")) {
	colorize=false;
	cmd->setProcessed(i,true);
      }
      if((cmd->value(i).toLower()=="on")||
	 (cmd->value(i).toLower()=="yes")||
	 (cmd->value(i).toLower()=="true")) {
	colorize=true;
	cmd->setProcessed(i,true);
      }
      if(!cmd->processed(i)) {
	fprintf(stderr,"lwmon: invalid argument to --color switch\n");
	exit(1);
      }
    }
    if(cmd->key(i)=="--mode") {
      if(cmd->value(i).toLower()=="lwcp") {
	mode=MainWidget::Lwcp;
	cmd->setProcessed(i,true);
      }
      if(cmd->value(i).toLower()=="lwrp") {
	mode=MainWidget::Lwrp;
	cmd->setProcessed(i,true);
      }
      if(cmd->value(i).toLower()=="lwaddr") {
	mode=MainWidget::Lwaddr;
	cmd->setProcessed(i,true);
      }
      if(!cmd->processed(i)) {
	fprintf(stderr,"lwmon: invalid \"--mode\" argument");
	exit(1);
      }
    }
    if(cmd->key(i)=="--from") {
      if(cmd->value(i).isEmpty()) {
	write_from_file="-";
      }
      else {
	write_from_file=cmd->value(i);
      }
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--password") {
      password=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"lwmon: invalid argument \"%s\"\n",
	      (const char *)cmd->key(i).toUtf8());
      exit(256);
    }
  }
  QStringList f0=cmd->key(cmd->keys()-1).split(":",Qt::KeepEmptyParts);
  QString hostname=f0.first();
  if(f0.size()==2) {
    port=f0.last().toUInt(&ok);
    if((!ok)||(port>0xFFFF)) {
      fprintf(stderr,"lwrp: invalid port value\n");
    }
  }

  //
  // Launch appropriate type
  //
  if((mode==MainWidget::Lwrp)&&(!write_from_file.isEmpty())) {
    WriteFromFile(hostname,password,port,write_from_file);
    exit(0);
  }

  QApplication a(argc,argv);
  MainWidget *w=new MainWidget(mode,hostname,port,colorize);
  w->show();
  return a.exec();
}
