// statuswidget.cpp
//
// Connection status widget
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

#include "statuswidget.h"

StatusWidget::StatusWidget(QWidget *parent)
  : QLabel(parent)
{
  setAlignment(Qt::AlignCenter);

  //
  // Create Stylesheets
  //
  stat_idle_style="";
  stat_connected_style="background-color: green;color: lightGray";
  stat_failed_style="background-color: red;color: lightGray";

  setStatus(StatusWidget::Idle);
}


StatusWidget::Status StatusWidget::status() const
{
  return stat_status;
}


bool StatusWidget::setStatus(StatusWidget::Status status)
{
  bool ret=false;

  switch(status) {
  case StatusWidget::Idle:
    setText(tr("IDLE"));
    setStyleSheet(stat_idle_style);
    ret=true;
    break;

  case StatusWidget::Connecting:
    setText(tr("CONNECTING..."));
    setStyleSheet(stat_idle_style);
    ret=true;
    break;

  case StatusWidget::Connected:
    setText(tr("CONNECTED"));
    setStyleSheet(stat_connected_style);
    ret=true;
    break;

  case StatusWidget::Failed:
    setText(tr("FAILED"));
    setStyleSheet(stat_failed_style);
    ret=true;
    break;
  }
  stat_status=status;

  return ret;
}
