// cmdswitch.cpp
//
// Process Rivendell Command-Line Switches
//
//   (C) Copyright 2002-2022 Fred Gleason <fredg@paravelsystems.com>
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
#include <syslog.h>
#include <stdlib.h>

#include <QCoreApplication>
#include <QMessageBox>

#include "cmdswitch.h"

CmdSwitch::CmdSwitch(const QString &modname,const QString &usage)
{
  switch_debug=false;

  QStringList args=qApp->arguments();

  for(int i=1;i<args.size();i++) {
    QString value=args.at(i);
    if(value=="--version") {
      printf("%s v%s \n",modname.toUtf8().constData(),VERSION);
      exit(0);
    }
    if(value=="--help") {
      printf("\n%s %s\n",modname.toUtf8().constData(),
	     usage.toUtf8().constData());
      exit(0);
    }
    if(value=="-d") {
      switch_debug=true;
    }
    QStringList f0=value.split("=",Qt::KeepEmptyParts);
    if(f0.size()>=2) {
      if(f0.at(0).left(1)=="-") {
	switch_keys.push_back(f0.at(0));
	for(int i=2;i<f0.size();i++) {
	  f0[1]+="="+f0.at(i);
	}
	if(f0.at(1).isEmpty()) {
	  switch_values.push_back("");
	}
	else {
	  switch_values.push_back(f0.at(1));
	}
      }
      else {
	switch_keys.push_back(f0.join("="));
	switch_values.push_back("");
      }
      switch_processed.push_back(false);
    }
    else {
      switch_keys.push_back(value);
      switch_values.push_back("");
      switch_processed.push_back(false);
    }
  }
}


CmdSwitch::CmdSwitch(int argc,char *argv[],const QString &modname,
			 const QString &usage)
{
  switch_debug=false;

  for(int i=1;i<argc;i++) {
    QString value=QString::fromUtf8(argv[i]);
    if(value=="--version") {
      printf("%s v%s\n",modname.toUtf8().constData(),VERSION);
      exit(0);
    }
    if(value=="--help") {
      printf("\n%s %s\n",modname.toUtf8().constData(),
	     usage.toUtf8().constData());
      exit(0);
    }
    if(value=="-d") {
      switch_debug=true;
    }
    QStringList f0=value.split("=",Qt::KeepEmptyParts);
    if(f0.size()>=2) {
      if(f0.at(0).left(1)=="-") {
	switch_keys.push_back(f0.at(0));
	for(int i=2;i<f0.size();i++) {
	  f0[1]+="="+f0.at(i);
	}
	if(f0.at(1).isEmpty()) {
	  switch_values.push_back("");
	}
	else {
	  switch_values.push_back(f0.at(1));
	}
      }
      else {
	switch_keys.push_back(f0.join("="));
	switch_values.push_back("");
      }
      switch_processed.push_back(false);
    }
    else {
      switch_keys.push_back(value);
      switch_values.push_back("");
      switch_processed.push_back(false);
    }
  }
}


unsigned CmdSwitch::keys() const
{
  return switch_keys.size();
}


QString CmdSwitch::key(unsigned n) const
{
  return switch_keys[n];
}


QString CmdSwitch::value(unsigned n) const
{
  return switch_values[n];
}


bool CmdSwitch::processed(unsigned n) const
{
  return switch_processed[n];
}


void CmdSwitch::setProcessed(unsigned n,bool state)
{
  switch_processed[n]=state;
}


bool CmdSwitch::allProcessed() const
{
  for(unsigned i=0;i<switch_processed.size();i++) {
    if(!switch_processed[i]) {
      return false;
    }
  }
  return true;
}


bool CmdSwitch::debugActive() const
{
  return switch_debug;
}
