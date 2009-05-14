/*
 * psifdnotify.cpp: Psi's interface to notification daemon
 * Copyright (C) 2005  Remko Troncon
 *               2009  Pontus Freyhult
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QPixmap>
#include <QStringList>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QtDBus>
#include <QDBusArgument>
#include "common.h"
#include "psiaccount.h"
#include "avatars.h"
#include "psifdnotify.h"
#include "psievent.h"
#include "psipopup.h"
#include "userlist.h"


/**
 * (Private) constructor of the PsiFdnotify.
 * Initializes notifications and registers with Fdnotify through Fdnotify.
 */
PsiFdnotify::PsiFdnotify() : QObject(QCoreApplication::instance()), interface("org.freedesktop.Notifications",
									      "/org/freedesktop/Notifications",
									      "org.freedesktop.Notifications")
{
	setup();
}


bool PsiFdnotify::setup()
{

  serverLives = false;

  //if (!interface.isValid())
  //  interface=new QDBusInterface("org.freedesktop.Notifications",
  //				 "/org/freedesktop/Notifications",
  //				 "org.freedesktop.Notifications");


  
  QDBusMessage reply;

  reply = interface.call("GetCapabilities");

  if (reply.type() == QDBusMessage::MethodCallMessage)
    serverLives = true;

  qDBusRegisterMetaType<iconStruct>();
  return true;


}



QDBusArgument &operator<<(QDBusArgument &r, const iconStruct &mystruct)
{

  QImage i = mystruct.i.convertToFormat(QImage::Format_ARGB32);

  r.beginStructure();
  r << i.width();
  r << i.height();
  r << i.bytesPerLine();
  r << i.hasAlphaChannel();
  r << 8; // i.depth();
  r << 4;
  
  QByteArray l;
  
  for (int j = 0; j < i.height(); j++) {
    QRgb* line = (QRgb*) i.scanLine(j);
    
    for (int k = 0; k < i.width(); k++) {
      
      l.append(qRed(line[k]));
      l.append(qGreen(line[k]));
      l.append(qBlue(line[k]));
      l.append(qAlpha(line[k]));
    }
  }
  
  r << l;
  r.endStructure();
  
  return r;
}



 // Retrieve the MyStructure data from the D-BUS argument
 const QDBusArgument &operator>>(const QDBusArgument &argument, iconStruct &mystruct)
 {
   // Create a dummy image for mystruct
   mystruct.i = QImage();
     return argument;
 }








/**
 * Requests the global PsiFdnotify instance.
 * If PsiFdnotify wasn't initialized yet, it is initialized.
 *
 * \see Fdnotify()
 * \return A pointer to the PsiFdnotify instance
 */
PsiFdnotify* PsiFdnotify::instance() 
{
	if (!instance_) 
		instance_ = new PsiFdnotify();
	
	return instance_;
}


/**
 * Requests a popup to be sent to Fdnotify.
 *
 * \param account The requesting account.
 * \param type The type of popup to be sent.
 * \param jid The originating jid
 * \param uli The originating userlist item. Can be NULL.
 * \param event The originating event. Can be NULL.
 */
void PsiFdnotify::popup(PsiAccount* account, PsiPopup::PopupType type, const Jid& jid, const Resource& r, const UserListItem* uli, PsiEvent* event)
{
        if (!serverLives)
	  setup();

	QString category("im");

	QString name;
	QString title, desc, contact;
	QString statusTxt = status2txt(makeSTATUS(r.status()));
	QString statusMsg = r.status().status();
	QPixmap icon = account->avatarFactory()->getAvatar(jid.bare());

	if (icon.isNull())
	    icon = IconsetFactory::iconPixmap("psi/logo_128");

	if (uli) {
		contact = uli->name();
	}
	else if (event->type() == PsiEvent::Auth) {
		contact = ((AuthEvent*) event)->nick();
	}
	else if (event->type() == PsiEvent::Message) {
		contact = ((MessageEvent*) event)->nick();
	}

	if (contact.isEmpty())
		contact = jid.bare();

	// Default value for the title
	title = contact;

	switch(type) {
		case PsiPopup::AlertOnline:
		        category = QString("presence.online");
			name = QObject::tr("Contact becomes Available: %1 (%2)").arg(contact).arg(statusTxt);
			desc = statusMsg;
			break;
		case PsiPopup::AlertOffline:
		        category = QString("presence.offline");
			name = QObject::tr("Contact becomes Unavailable: %1 (%2)").arg(contact).arg(statusTxt);
			desc = statusMsg;
			break;
		case PsiPopup::AlertStatusChange:
		        category = QString("presence");
			name = QObject::tr("Contact changes Status: %1 (%2)").arg(contact).arg(statusTxt);
			desc = statusMsg;
			break;
	        case PsiPopup::AlertMessage: {
		        category = QString("im.received");
			name = QObject::tr("Incoming Message: %1 says:").arg(contact);
			const Message* jmessage = &((MessageEvent *)event)->message();
			desc = jmessage->body();
			break;
		}
		case PsiPopup::AlertChat: {
		        category = QString("im.received");
			name = QObject::tr("Incoming Message from %1").arg(contact);
			const Message* jmessage = &((MessageEvent *)event)->message();
			desc = jmessage->body();
			break;
		}
		case PsiPopup::AlertHeadline: {
			name = QObject::tr("Incoming Headline");
			const Message* jmessage = &((MessageEvent *)event)->message();
			if ( !jmessage->subject().isEmpty())
				title = jmessage->subject();
			desc = jmessage->body();
			break;
		}
		case PsiPopup::AlertFile:
			name = QObject::tr("Incoming File");
			desc = QObject::tr("[Incoming File]");

			break;
		default:
			break;
	}
	
	QMap<QString,QVariant> hints;
	QStringList actions;
	QDBusMessage reply;
	
	hints.insert("category", category);
	hints.insert("urgency", qVariantFromValue<uchar>(1));

	if (!icon.isNull()) {
	  iconStruct p;
	  p.i = icon.toImage(); 

	  QVariant v;
	  v.setValue(p);
	  hints.insert("icon_data", v); 
	}

	reply = interface.call("Notify",
			       QObject::tr("Psi"),
			       (uint) 0,
			       "",
			       name,
			       desc,
			       actions,
			       hints,
			       -1);


	if (reply.type() != QDBusMessage::ReplyMessage)
	  {
	    PsiPopup *popup = new PsiPopup(type, account);
	    popup->setData(jid, r, uli);
	  }	
	
}

void PsiFdnotify::notificationClicked(void* c)
{
  //	NotificationContext* context = (NotificationContext*) c;
  //	context->account()->actionDefault(context->jid());
  //	delete context;
}

PsiFdnotify* PsiFdnotify::instance_ = 0;
