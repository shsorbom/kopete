//Licensed under the GNU General Public License

#include "logintest.h"
#include <QtNetwork/QTcpSocket>

LoginTest::LoginTest(int argc, char ** argv) : QApplication( argc, argv )
{
	// set up client stream
	myTestObject = new ClientStream( new QTcpSocket(), 0 );
	
	// notify when the transport layer is connected
	//connect( myTestObject, SIGNAL( connected() ), SLOT( slotConnected() ) );
	myClient = new Client();

	myConnection = new Connection( myTestObject, "AUTHORIZER" );
	myConnection->setClient( myClient );
	// do test once the event loop is running
	QTimer::singleShot( 0, this, SLOT( slotDoTest() ) );
	connected = false;
}

LoginTest::~LoginTest()
{
	delete myTestObject;
	delete myClient;
}

void LoginTest::slotDoTest()
{
	QString server = QString::fromLatin1("login.oscar.aol.com");
	// connect to server
	qDebug( "connecting to server ");

	myClient->setIsIcq( true );
	myClient->start( server, 5190, "userid", "password" );
	myClient->connectToServer( myConnection, server, 5190, true );
	connected = true;
}

void LoginTest::slotConnected()
{
	qDebug( "connection is up");
	connected = true;
}

int main(int argc, char ** argv)
{
	LoginTest a( argc, argv );
	a.exec();
	if ( !a.isConnected() )
		return 0;
}

#include "logintest.moc"
