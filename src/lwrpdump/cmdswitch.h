// cmdswitch.h
//
// Process Command-Line Switches
//
//   (C) Copyright 2013-2024 Fred Gleason <fredg@paravelsystems.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of version 2.1 of the GNU Lesser General Public
//    License as published by the Free Software Foundation;
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, 
//    Boston, MA  02111-1307  USA
//
// EXEMPLAR_VERSION: 2.0.1
//

#ifndef CMDSWITCH_H
#define CMDSWITCH_H

#include <QList>
#include <QString>

class CmdSwitch
{
 public:
  CmdSwitch(const QString &modname,const QString &modver,const QString &usage);
  int keys() const;
  QString key(int n) const;
  QString value(int n) const;
  bool processed(int n) const;
  void setProcessed(int n,bool state);
  bool allProcessed() const;

 private:
  QList<QString> switch_keys;
  QList<QString> switch_values;
  QList<bool> switch_processed;
};


#endif  // CMDSWITCH_H
