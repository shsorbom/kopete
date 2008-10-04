#ifndef JABBER_JINGLE_SESSION_H
#define JABBER_JINGLE_SESSION_H
#include <QObject>
#include <QDomElement>
#include <QTimer>
#include <QTime>

namespace XMPP
{
	class JingleSession;
	class JingleContent;
}
class JingleCallsManager;
class JabberJingleContent;
class MediaManager;
class MediaSession;
class JingleRtpSession;

class JabberJingleSession : public QObject
{
	Q_OBJECT
public:
	JabberJingleSession(JingleCallsManager*);
	~JabberJingleSession();
	
	void setJingleSession(XMPP::JingleSession*);
	XMPP::JingleSession *jingleSession() {return m_jingleSession;}
	
	JabberJingleContent *contentWithName(const QString&);
	XMPP::JingleSession *session() const {return m_jingleSession;} //FIXME:Use jingleSession()
	MediaManager *mediaManager() const;
	QList<JabberJingleContent*> contents() const {return jabberJingleContents;}
	QTime upTime();

public slots:
	void writeRtpData(XMPP::JingleContent*);
	void slotUptimeOut();
	void slotSessionTerminated();
	void slotStateChanged();


signals:
	void terminated();

private:
	XMPP::JingleSession *m_jingleSession;
	JingleCallsManager *m_callsManager;
	MediaManager *m_mediaManager;
	QList<JabberJingleContent*> jabberJingleContents;
	QTime m_timeUp;
//	JingleRtpSession *m_rtpSession;
};

#endif
