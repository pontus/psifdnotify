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
#include "common.h"
#include "psiaccount.h"
#include "avatars.h"
#include "psifdnotify.h"
#include "psievent.h"
#include "userlist.h"


/**
 * (Private) constructor of the PsiFdnotify.
 * Initializes notifications and registers with Fdnotify through Fdnotify.
 */
PsiFdnotify::PsiFdnotify() : QObject(QCoreApplication::instance()), interface("org.freedesktop.Notifications",
									      "/org/freedesktop/Notifications",
									      "org.freedesktop.Notifications")
{
	// Initialize all notifications
	QStringList nots;
	nots << QObject::tr("Contact becomes Available");
	nots << QObject::tr("Contact becomes Unavailable");
	nots << QObject::tr("Contact changes Status");
	nots << QObject::tr("Incoming Message");
	nots << QObject::tr("Incoming Headline");
	nots << QObject::tr("Incoming File");

	// Initialize default notifications
	QStringList defaults;
	defaults << QObject::tr("Contact becomes Available");
	defaults << QObject::tr("Incoming Message");
	defaults << QObject::tr("Incoming Headline");
	defaults << QObject::tr("Incoming File");

	serverLives = false;
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
    

  return true;
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


	QString name;
	QString title, desc, contact;
	QString statusTxt = status2txt(makeSTATUS(r.status()));
	QString statusMsg = r.status().status();
	QPixmap icon = account->avatarFactory()->getAvatar(jid.bare());
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
			name = QObject::tr("Contact becomes Available");
			title = QString("%1 (%2)").arg(contact).arg(statusTxt);
			desc = statusMsg;
			//icon = PsiIconset::instance()->statusPQString(jid, r.status());
			break;
		case PsiPopup::AlertOffline:
			name = QObject::tr("Contact becomes Unavailable");
			title = QString("%1 (%2)").arg(contact).arg(statusTxt);
			desc = statusMsg;
			//icon = PsiIconset::instance()->statusPQString(jid, r.status());
			break;
		case PsiPopup::AlertStatusChange:
			name = QObject::tr("Contact changes Status");
			title = QString("%1 (%2)").arg(contact).arg(statusTxt);
			desc = statusMsg;
			//icon = PsiIconset::instance()->statusPQString(jid, r.status());
			break;
		case PsiPopup::AlertMessage: {
			name = QObject::tr("Incoming Message");
			title = QObject::tr("%1 says:").arg(contact);
			const Message* jmessage = &((MessageEvent *)event)->message();
			desc = jmessage->body();
			//icon = IconsetFactory::iconPQString("psi/message");
			break;
		}
		case PsiPopup::AlertChat: {
			name = QObject::tr("Incoming Message");
			const Message* jmessage = &((MessageEvent *)event)->message();
			desc = jmessage->body();
			//icon = IconsetFactory::iconPQString("psi/start-chat");
			break;
		}
		case PsiPopup::AlertHeadline: {
			name = QObject::tr("Incoming Headline");
			const Message* jmessage = &((MessageEvent *)event)->message();
			if ( !jmessage->subject().isEmpty())
				title = jmessage->subject();
			desc = jmessage->body();
			//icon = IconsetFactory::iconPQString("psi/headline");
			break;
		}
		case PsiPopup::AlertFile:
			name = QObject::tr("Incoming File");
			desc = QObject::tr("[Incoming File]");
			//icon = IconsetFactory::iconPQString("psi/file");
			break;
		default:
			break;
	}

	// Notify Fdnotify
	//NotificationContext* context = new NotificationContext(account, jid);
	// gn_->notify(name, title, desc, icon, false, this, SLOT(notificationClicked(void*)), SLOT(notificationTimedOut(void*)), context);

	
	QDBusMessage reply;

	reply = interface.call("Notify",
			       QObject::tr("Psi"),
			       0,
			       "",
			       name,
			       desc,
			       QVariant(),
			       QVariant(),
			       0);

}

void PsiFdnotify::notificationClicked(void* c)
{
  //	NotificationContext* context = (NotificationContext*) c;
  //	context->account()->actionDefault(context->jid());
  //	delete context;
}

void PsiFdnotify::notificationTimedOut(void* c)
{
  //	NotificationContext* context = (NotificationContext*) c;
  //	delete context;
}

PsiFdnotify* PsiFdnotify::instance_ = 0;
