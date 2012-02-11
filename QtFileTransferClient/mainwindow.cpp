#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../sharedstructures.h"
#include "../fileinfo.h"
#include <QDebug>
#include <QHostAddress>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	connect(ui->pbGetFiles, SIGNAL(clicked()), this, SLOT(downloadFileList()));
	connect(ui->pbDownloadSelected, SIGNAL(clicked()), this, SLOT(requestFileDownload()));

	tableModel = new FileListItemModel(this);

	ui->tvFileList->setModel(tableModel);
	ui->tvFileList->header()->setResizeMode(QHeaderView::ResizeToContents);
}

void MainWindow::downloadFileList()
{
	sock = new QTcpSocket(this);
	sock->connectToHost(QHostAddress::LocalHost, DEFAULT_SERVER_LISTEN_PORT);

	qDebug() << "State: " << sock->state();
	connControlMsg msg;
	msg.message = CONN_CONTROL_REQUEST_FILE_LIST;

	sock->write((char*)&msg, sizeof(msg));

	connect(sock, SIGNAL(readyRead()), this, SLOT(onListReceiveData()));
	connect(sock, SIGNAL(disconnected()), this, SLOT(sock_disconn()));
}

void MainWindow::requestFileDownload()
{

	QModelIndex modelIndex = ui->tvFileList->currentIndex();

	if(modelIndex.isValid())
	{
		sock = new QTcpSocket(this);
		sock->connectToHost(QHostAddress::LocalHost, DEFAULT_SERVER_LISTEN_PORT);

		connControlMsg msg;
		msg.message = CONN_CONTROL_REQUEST_FILE_DOWNLOAD;
		rec_file = static_cast<FileInfo*>(modelIndex.internalPointer());

		//MORE MAGIC
		memcpy(msg.sha1_id, rec_file->getHash().constData(), 20);
		sock->write((char*)&msg, sizeof(msg));

		out_file = new QFile(rec_file->getName());

		out_file->open(QIODevice::WriteOnly);
		rec_bytes =0;


		connect(sock, SIGNAL(readyRead()), this, SLOT(onFileReceiveData()));
		connect(sock, SIGNAL(disconnected()), this, SLOT(sock_disconn()));
	}
}


void MainWindow::onListReceiveData()
{
	qDebug() << "onListReceiveData triggered";

	unsigned int size;

	while(sock->bytesAvailable() > 0)
	{
		sock->read((char*)&size, sizeof(size));

		char *buff = new char[size];
		sock->read(buff, size);

		FileInfo *fi = new FileInfo();

		fi->setFromByteArray(buff);
		qDebug() << "got item: " << fi->getName() << " of size " << fi->getSize()
				 << " with id " << fi->getId() << "; parid: " << fi->getParentId();

		this->tableModel->insertRowWithData(fi);
		delete [] buff;
	}
}

void MainWindow::onFileReceiveData()
{

	qDebug() << "onFileReceiveData triggered";


	QByteArray arr = sock->readAll();
	out_file->write(arr);
	rec_bytes+= arr.count();

	if(rec_bytes == rec_file->getSize())
	{
		delete sock;
		out_file->close();
		delete out_file;
		rec_file = NULL;
	}
}


void MainWindow::sock_disconn()
{
	qDebug() << "Sock disconnected";
}

MainWindow::~MainWindow()
{
    delete ui;
}
