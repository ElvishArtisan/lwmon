//   string.cpp
//
//   String with quote mode
//
//   (C) Copyright 2010-2015 Fred Gleason <fredg@paravelsystems.com>
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

#include <QtCore/QStringList>

#include "astring.h"

AString::AString()
  : QString()
{
}


AString::AString(const AString &lhs)
  : QString(lhs)
{
}


AString::AString(const QString &lhs)
  : QString(lhs)
{
}


QStringList AString::split(const QString &sep,const QString &esc) const
{
  if(esc.isEmpty()) {
    return QString::split(sep);
  }
  QStringList list;
  bool escape=false;
  QChar e=esc.at(0);
  list.push_back(QString());
  for(int i=0;i<length();i++) {
    if(at(i)==e) {
      escape=!escape;
      list.back()+=esc;
    }
    else {
      if((!escape)&&(mid(i,1)==sep)) {
	list.push_back(QString());
      }
      else {
	list.back()+=at(i);
      }
    }
  }
  return list;
}
