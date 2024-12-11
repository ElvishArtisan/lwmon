// writefromfile.h
//
// Write LWRP to a device
//
//   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
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

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <termios.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <QObject>

#include "astring.h"
#include "lwmon.h"

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif  // STDIN_FILENO

void WriteToDevice(int sock,const QByteArray &cmd)
{
  if(write(sock,cmd+"\r\n",2+cmd.size())!=2+cmd.size()) {
    fprintf(stderr,"dropped characters when writing to LWRP device\n");
  }
}


QString GetLine(const QString &prompt)
{
  char line[1024];
  struct termios orig_to;
  struct termios noecho_to;
  char *ret=NULL;
  
  //
  // Turn off character echo
  //
  tcgetattr(STDIN_FILENO,&orig_to);
  noecho_to=orig_to;
  noecho_to.c_lflag=noecho_to.c_lflag^ECHO;
  tcsetattr(STDIN_FILENO,TCSANOW,&noecho_to);

  //
  // Get string
  //
  printf("%s ",prompt.toUtf8().constData());
  fflush(stdout);
  ret=fgets(line,1024,stdin);
  
  //
  // Restore character echo
  //
  tcsetattr(STDIN_FILENO,TCSANOW,&orig_to);
  printf("\n");

  if(ret==NULL) {
    return QString();
  }
  return QString::fromUtf8(line).trimmed();
}


bool Login(int sock,const QString &password,QString *err_msg)
{
  int n;
  char data[1024];
  QString login="LOGIN";
  QByteArray accum;
  QStringList f0;
  
  if(!password.isEmpty()) {
    login+=" "+password;
  }
  WriteToDevice(sock,login.toUtf8());
  WriteToDevice(sock,"VER");
  
  while((n=read(sock,data,1024))>0) {
    data[n]=0;
    for(int i=0;i<n;i++) {
      switch(0xFF&data[i]) {
      case '\n':
	f0=AString(QString(accum).trimmed()).split(" ","\"");
	if(f0.first()=="VER") {
	  *err_msg="OK";
	  return true;
	}
	if(f0.first()=="ERROR") {
	  f0.removeFirst();
	  f0.removeFirst();
	  *err_msg=f0.join(" ");
	  return false;
	}
	accum.clear();
	break;

      default:
	accum+=data[i];
	break;
      }
    }
  }

  return false;
}

void WriteFromFile(const QString &hostname,QString password,
		   uint16_t port,const QString &filename)
{
  if(password.isEmpty()&&(!password.isNull())) {  // Prompt for password
    password=GetLine(QObject::tr("Password")+":");
  }

  int fd=0;
  FILE *f=NULL;
  int sock=-1;
  struct addrinfo ai_hints;
  struct addrinfo *ai_out=NULL;
  int err=0;
  char data[1024];
  QByteArray out;
  QString err_msg;
  
  //
  // Open source file
  //
  if(filename.isEmpty()||(filename=="-")) {
    fd=0;
  }
  else {
    if((fd=open(filename.toUtf8(),O_RDONLY))<0) {
      fprintf(stderr,"lwrp: %s\n",strerror(errno));
      exit(1);
    }
  }
  if((f=fdopen(fd,"r"))==NULL) {
    fprintf(stderr,"lwrp: %s\n",strerror(errno));
  }
  
  //
  // Resolve Hostname
  //
  memset(&ai_hints,0,sizeof(ai_hints));
  ai_hints.ai_socktype=SOCK_STREAM;
  if((err=getaddrinfo(hostname.toUtf8(),QString::asprintf("%d",port).toUtf8(),
		      &ai_hints,&ai_out))!=0) {
    fprintf(stderr,"lwrp: %s\n",gai_strerror(err));
    exit(1);
  }


  //
  // Open device connection
  //
  bool connected=false;
  for(struct addrinfo *ai=ai_out;ai!=NULL;ai=ai->ai_next) {
    if((sock=socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol))<0) {
      continue;
    }
    if(connect(sock,ai->ai_addr,ai->ai_addrlen)!=0) {
      continue;
    }
    else {
      connected=true;
      break;
    }
  }
  if(!connected) {
    fprintf(stderr,"lwrp: %s\n",strerror(errno));
    exit(1);
  }

  //
  // Transfer Data
  //
  if(!Login(sock,password,&err_msg)) {
    fprintf(stderr,"lwrp: %s\n",err_msg.toUtf8().constData());
    exit(1);
  }
  while(fgets(data,1024,f)!=NULL) {
    QString line=QString::fromUtf8(data).trimmed();
    if(!line.isEmpty()) {
      if((line.left(1)!="#")&&
	 (line.left(3).toUpper()!="VER")) {
	WriteToDevice(sock,line.toUtf8());
      }
    }
  }
}
