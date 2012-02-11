#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serverobject.h"
#include "../sharedstructures.h"
#include <QDebug>
#include <QThread>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


	settings = new QSettings(tr("Martin Bakiev"), tr("QtFileTransfer"), this);

	model = new DirTreeModel(this);
	ui->tvDirList->setModel(model);
	ui->tvDirList->header()->setResizeMode(QHeaderView::ResizeToContents);

	int size = settings->beginReadArray("shared_directories");
	for(int i = 0; i < size; i++)
	{
		settings->setArrayIndex(i);
		model->addDirectory(settings->value("dir").toString());
	}
	settings->endArray();


	int portNumber = settings->value("server/portNumber", DEFAULT_SERVER_LISTEN_PORT).toInt();

	this->server = new MyTcpServer(portNumber, this);
	this->server->startServer();

	connect(this->server,SIGNAL(newConnectionDescriptor(int)), this, SLOT(newConnection(int)));
	connect(ui->pbToggleServer, SIGNAL(clicked()), this, SLOT(buttonTest()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	Q_UNUSED(event);

	qDebug() << "Close event received";
	server->close();

	settings->beginWriteArray("shared_directories");
	QList<FileInfo*> sharedDirList = model->getSharedDirList();
	for(int i = 0; i < sharedDirList.count(); i++)
	{
		settings->setArrayIndex(i);
		settings->setValue("dir", sharedDirList.at(i)->getPath());
	}
	settings->endArray();


	delete server;
	delete settings;

}
void MainWindow::buttonTest()
{
	QString path("/home/mrafcho001/tmp/qtransfer_test");
	model->addDirectory(path);
}

void MainWindow::newConnection(int socketDescriptor)
{
	//Handle New Connection

	qDebug() << "New Connection: " << socketDescriptor;
	QThread *thread = new QThread(this);


	ServerObject *serverObject ;//= new ServerObject(socketDescriptor, );

	serverObject->moveToThread(thread);
	connect(thread, SIGNAL(started()), serverObject, SLOT(handleConnection()));
	connect(serverObject, SIGNAL(finished()), thread, SLOT(quit()));

	thread->start();
}


MainWindow::~MainWindow()
{
    delete ui;
}
