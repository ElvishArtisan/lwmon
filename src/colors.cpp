// color.cpp
//
// Syntax colorization routines
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

#include "lwmon.h"

QString MainWidget::Colorize(const QString &cmd) const
{
  QString ret=cmd;

  if(lw_colorize) {
    switch(lw_mode) {
    case MainWidget::Lwcp:
      ret=ColorizeLwcp(cmd);
      break;
      
    case MainWidget::Lwrp:
      ret=ColorizeLwrp(cmd);
      break;

    case MainWidget::Lwaddr:
      ret=cmd;
      break;
    }
  }
  else {
    ret=cmd;
  }

  return ret;
}


QString MainWidget::ColorizeLwcp(const AString &cmd) const
{
  QStringList f0=AString(cmd.trimmed()).split(" ","\"");
  QString ret;
  
  if(f0[0]=="ERROR") {
    return ColorString(cmd,Qt::red);
  }

  ret+=ColorString(f0[0],Qt::blue);
  if(f0.size()>1) {
    ret+=" "+ColorString(f0[1],Qt::darkGreen)+" ";
    f0.erase(f0.begin());
    f0.erase(f0.begin());
    
    QStringList f1=AString(f0.join(" ")).split(",","\"");
    for(int i=0;i<f1.size();i++) {
      QStringList f2=AString(f1[i]).split("=","\"");
      ret+=ColorString(f2[0],Qt::magenta);
      if(f2.size()==2) {
	ret+="=";
	if(f2[1].left(1)=="\"") {
	  ret+=ColorString(f2[1],Qt::gray);
	}
	else {
	  ret+=f2[1];
	}
	ret+=",";
      }
    }
    ret=ret.left(ret.length()-1);
  }

  return ret;
}


QString MainWidget::ColorizeLwrp(const AString &cmd) const
{
  QStringList f1;
  QStringList f0=AString(cmd.trimmed()).split(" ","\"");
  QString ret;

  if(f0[0]=="ERROR") {
    return ColorString(cmd,Qt::red);
  }
  int istate=0;

  for(int i=0;i<f0.size();i++) {
    bool ok=false;
    switch(istate) {
    case 0:  // Command
      if(f0[i]=="IP") {
	ret+=ColorString(f0[i],Qt::blue)+" ";
	istate=3;
	ok=true;	
      }
      if(f0[i]=="LVL") {
	ret+=ColorString(f0[i],Qt::blue)+" ";
	istate=6;
	ok=true;
      }
      if(f0[i]=="MTR") {
	ret+=ColorString(f0[i],Qt::blue)+" ";
	istate=5;
	ok=true;	
      }
      if(f0[i]=="VER") {
	ret+=ColorString(f0[i],Qt::blue)+" ";
	istate=2;
	ok=true;
      }
      if((f0[i]=="DST")||
	 (f0[i]=="SRC")||
	 (f0[i]=="GPI")||
	 (f0[i]=="GPO")) {
	ret+=ColorString(f0[i],Qt::blue)+" ";
	istate=1;
	ok=true;
      }
      if((f0[i]=="ADD")||
	 (f0[i]=="DEL")||
	 (f0[i]=="CFG")) {
	ret+=ColorString(f0[i],Qt::blue)+" ";
	ok=true;
      }
      break;

    case 1:   // Slot number
      ret+=ColorString(f0[i],Qt::darkGreen)+" ";
      istate=2;
      ok=true;
      break;

    case 2:   // Value pairs
      f1=AString(f0[i]).split(":","\"");
      if(f1.size()<2) {
	ret+=f1.back()+" ";
      }
      else {
	ret+=ColorString(f1[0],Qt::magenta)+":";
	for(int j=1;j<(f1.size()-1);j++) {
	  if(f1[j].left(1)=="\"") {
	    ret+=ColorString(f1[j],Qt::gray)+":";
	  }
	  else {
	    ret+=f1[1]+":";
	  }
	}
	if(f1.back().left(1)=="\"") {
	  ret+=ColorString(f1.back(),Qt::gray)+" ";
	}
	else {
	  ret+=f1.back()+" ";
	}
      }
      ok=true;
      break;

    case 3:   // IP field names
      if((f0[i]=="address")||
	 (f0[i]=="netmask")||
	 (f0[i]=="gateway")||
	 (f0[i]=="hostname")) {
	ret+=ColorString(f0[i],Qt::magenta)+" ";
	istate=4;
	ok=true;
      }
      break;

    case 4:   // IP field values
      ret+=f0[i]+" ";
      istate=3;
      ok=true;
      break;

    case 5:   // Meter Type
      if((f0[i]=="ICH")||(f0[i]=="OCH")) {
	ret+=ColorString(f0[i],Qt::cyan)+" ";
	istate=1;
	ok=true;
      }
      break;

    case 6:   // Level Type
      if((f0[i]=="ICH")||(f0[i]=="OCH")) {
	ret+=ColorString(f0[i],Qt::cyan)+" ";
	istate=1;
	ok=true;
      }
      break;
      
    case 7:   // Plain Black
      ret+=f0[i]+" ";
      ok=true;
      break;
    }
    

    if(!ok) {  // Unrecognized element, bail out
      return cmd;
    }

  }

  return ret;
}


QString MainWidget::ColorString(const QString &str,const QColor &color) const
{
  return "<font color=\""+color.name()+"\">"+str+"</font>";
}
