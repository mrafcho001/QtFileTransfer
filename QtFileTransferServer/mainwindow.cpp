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

	this->server = new MyTcpServer(this);
	this->server->startServer();

	connect(this->server,SIGNAL(newConnectionDescriptor(int)), this, SLOT(newConnection(int)));

	m_file_list = new QList<FileInfo>();

	m_file_list->append(FileInfo(3, 64, false, 0, tr("Item 1")));
	m_file_list->append(FileInfo(4, 64, false, 0, tr("Item 2")));
	m_file_list->append(FileInfo(5, 64, false, 0, tr("Item 3")));
	m_file_list->append(FileInfo(6, 64, false, 0, tr("Item 4")));
	m_file_list->append(FileInfo(7, 64, false, 0, tr("Item 5")));
	m_file_list->append(FileInfo(8, 64, false, 4, tr("Item 6")));
	m_file_list->append(FileInfo(9, 64, false, 4, tr("Item 7")));
	m_file_list->append(FileInfo(10, 64, false, 4, tr("Item 8")));
	m_file_list->append(FileInfo(11, 64, false, 0, tr("Item 9")));
}

void MainWindow::newConnection(int socketDescriptor)
{
	//Handle New Connection

	qDebug() << "New Connection: " << socketDescriptor;
	QThread *thread = new QThread(this);

	ServerObject *serverObject = new ServerObject(socketDescriptor, m_file_list);

	serverObject->moveToThread(thread);
	connect(thread, SIGNAL(started()), serverObject, SLOT(handleConnection()));
	connect(serverObject, SIGNAL(finished()), thread, SLOT(quit()));

	thread->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}
