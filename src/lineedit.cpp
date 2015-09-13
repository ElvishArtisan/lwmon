// lineedit.cpp
//
// LineEdit widget with command history
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

#include <stdio.h>

#include <QKeyEvent>

#include "lineedit.h"

LineEdit::LineEdit(QWidget *parent)
  : QLineEdit(parent)
{
  edit_current_item=0;
}


void LineEdit::loadHistory(const QString &path)
{
  FILE *f=NULL;
  char line[1024];

  edit_history.clear();
  if((f=fopen(path.toUtf8(),"r"))!=NULL) {
    while(fgets(line,1024,f)!=NULL) {
      edit_history.push_back(QString(line).trimmed());
    }
    edit_current_item=edit_history.size();
    fclose(f);
  }
}


void LineEdit::saveHistory(const QString &path) const
{
  FILE *f=NULL;

  if((f=fopen((path+"-TMP").toUtf8(),"w"))!=NULL) {
    for(int i=0;i<edit_history.size();i++) {
      fprintf(f,"%s\n",(const char *)edit_history[i].toUtf8());
    }
    fclose(f);
  }
  rename((path+"-TMP").toUtf8(),path.toUtf8());
}


void LineEdit::keyPressEvent(QKeyEvent *e)
{
  switch(e->key()) {
  case Qt::Key_Enter:
  case Qt::Key_Return:
    if(!text().isEmpty()) {
      if((edit_history.size()==0)||(edit_history.back()!=text())) {
	edit_history.push_back(text());
      }
      edit_current_item=edit_history.size();
      edit_current_text="";
      QLineEdit::keyPressEvent(e);
    }
    break;

  case Qt::Key_Up:
    if(edit_current_item>0) {
      setText(edit_history[--edit_current_item]);
    }
    break;

  case Qt::Key_Down:
    if(edit_current_item<(edit_history.size()-1)) {
      setText(edit_history[++edit_current_item]);
    }
    else {
      setText(edit_current_text);
    }
    break;

  case Qt::Key_Backspace:
    edit_current_text=text().left(edit_current_text.length()-1);
    QLineEdit::keyPressEvent(e);
    break;

  case Qt::Key_A:
  case Qt::Key_B:
  case Qt::Key_C:
  case Qt::Key_D:
  case Qt::Key_E:
  case Qt::Key_F:
  case Qt::Key_G:
  case Qt::Key_H:
  case Qt::Key_I:
  case Qt::Key_J:
  case Qt::Key_K:
  case Qt::Key_L:
  case Qt::Key_M:
  case Qt::Key_N:
  case Qt::Key_O:
  case Qt::Key_P:
  case Qt::Key_Q:
  case Qt::Key_R:
  case Qt::Key_S:
  case Qt::Key_T:
  case Qt::Key_U:
  case Qt::Key_V:
  case Qt::Key_W:
  case Qt::Key_X:
  case Qt::Key_Y:
  case Qt::Key_Z:
  case Qt::Key_0:
  case Qt::Key_1:
  case Qt::Key_2:
  case Qt::Key_3:
  case Qt::Key_4:
  case Qt::Key_5:
  case Qt::Key_6:
  case Qt::Key_7:
  case Qt::Key_8:
  case Qt::Key_9:
  case Qt::Key_Space:
  case Qt::Key_Exclam:
  case Qt::Key_QuoteDbl:
  case Qt::Key_NumberSign:
  case Qt::Key_Dollar:
  case Qt::Key_Percent:
  case Qt::Key_Ampersand:
  case Qt::Key_Apostrophe:
  case Qt::Key_ParenLeft:
  case Qt::Key_ParenRight:
  case Qt::Key_Asterisk:
  case Qt::Key_Plus:
  case Qt::Key_Comma:
  case Qt::Key_Minus:
  case Qt::Key_Period:
  case Qt::Key_Slash:
  case Qt::Key_Colon:
  case Qt::Key_Semicolon:
  case Qt::Key_Less:
  case Qt::Key_Equal:
  case Qt::Key_Greater:
  case Qt::Key_Question:
  case Qt::Key_At:
  case Qt::Key_BracketLeft:
  case Qt::Key_Backslash:
  case Qt::Key_BracketRight:
  case Qt::Key_AsciiCircum:
  case Qt::Key_Underscore:
  case Qt::Key_QuoteLeft:
  case Qt::Key_BraceLeft:
  case Qt::Key_Bar:
  case Qt::Key_BraceRight:
  case Qt::Key_AsciiTilde:
    edit_current_text=text()+e->text();
    QLineEdit::keyPressEvent(e);
    break;

  default:
    QLineEdit::keyPressEvent(e);
    break;
  }
}
