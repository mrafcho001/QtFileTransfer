#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serverobject.h"
#include "../sharedstructures.h"
#include <QDebug>
#include <QThread>
#include <QFileDialog>

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
	connect(ui->pbRemoveDir, SIGNAL(clicked()), this, SLOT(removeSelected()));
	connect(ui->pbAddDir, SIGNAL(clicked()), this, SLOT(addNewDirectory()));

	//Keep serializedList up to date
	m_serializedList = new QList<FileInfo*>(model->getSerializedList());
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

void MainWindow::removeSelected()
{
	QModelIndex index = ui->tvDirList->currentIndex();

	qDebug() << "Trying to remove";
	if(model->removeRows(0, 0, index))
		*m_serializedList = model->getSerializedList();
}

void MainWindow::addNewDirectory()
{
	QFileDialog dialog(this);

	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly, false);

	if(settings->contains("server/last_added_dir"))
		dialog.setDirectory(settings->value("server/last_added_dir").toString());

	QStringList selectedDir;
	if(dialog.exec())
		selectedDir = dialog.selectedFiles();

	for(int i = 0; i < selectedDir.count(); i++)
	{
		model->addDirectory(selectedDir.at(0));
	}

	if(selectedDir.count()>0)
	{
		settings->setValue("server/last_added_dir", selectedDir.value(0));
		*m_serializedList = model->getSerializedList();
	}
}

void MainWindow::newConnection(int socketDescriptor)
{
	//Handle New Connection

	qDebug() << "New Connection: " << socketDescriptor;
	QThread *thread = new QThread(this);

	ServerObject *serverObject = new ServerObject(socketDescriptor, m_serializedList);

	serverObject->moveToThread(thread);
	connect(thread, SIGNAL(started()), serverObject, SLOT(handleConnection()));
	connect(serverObject, SIGNAL(finished()), thread, SLOT(quit()));

	thread->start();
}


MainWindow::~MainWindow()
{
    delete ui;
}
