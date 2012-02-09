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
	connect(ui->pbDownloadSelected, SIGNAL(clicked()), this, SLOT(testSlot()));

	tableModel = new FileListItemModel(this);

	ui->tvFileList->setModel(tableModel);
}


void MainWindow::testSlot()
{
	qDebug() << "Test Slot";

	sock = new QTcpSocket(this);
	sock->connectToHost(QHostAddress::LocalHost, SERVER_LISTEN_PORT);

	qDebug() << "State: " << sock->state();
	connControlMsg msg;
	msg.message = CONN_CONTROL_REQUEST_FILE_LIST;

	sock->write((char*)&msg, sizeof(msg));

	connect(sock, SIGNAL(readyRead()), this, SLOT(reply()));
	connect(ui->tvFileList, SIGNAL())

}
void MainWindow::reply()
{
	qDebug() << "avail data: " << sock->bytesAvailable();

	unsigned int size;

	while(sock->bytesAvailable() > 0)
	{
		sock->read((char*)&size, sizeof(size));


		char *buff = new char[size];
		sock->read(buff, size);

		FileInfo fi;

		fi.setFromByteArray(buff);
		qDebug() << "got item: " << fi.getName() << " of size " << fi.getSize() << " with id " << fi.getId() << "; parid: " << fi.getParentId();

		QModelIndex mi =  this->ui->tvFileList->rootIndex();
		this->tableModel->appendRowWithData(fi, mi);
		delete [] buff;
	}
}

MainWindow::~MainWindow()
{
    delete ui;
}
