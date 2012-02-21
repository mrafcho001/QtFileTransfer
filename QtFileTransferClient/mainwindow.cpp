#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../sharedstructures.h"
#include "../fileinfo.h"
#include <QDebug>
#include <QHostAddress>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QThread>
#include <QFileDialog>
#include <QTimer>
#include <QToolButton>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QGridLayout>

ClientUIBundle::ClientUIBundle():
	layout(NULL), pbProgress(NULL), lblFilName(NULL), lblSpeed(NULL), hLine(NULL),
	pbAction(NULL), file(NULL), client(NULL)
{
}

ClientUIBundle::ClientUIBundle(FileInfo* file, DownloadClient *clientObj,
												 QWidget *parent)
{
	this->file = file;
	client = clientObj;

	layout = new QGridLayout(0);


	lblFilName = new QLabel(file->getName(), parent);
	layout->addWidget(lblFilName, 0, 0, 1, 2);

	pbProgress = new QProgressBar(parent);
	pbProgress->setMaximum(file->getSize());
	pbProgress->setValue(0);
	pbProgress->setTextVisible(true);
	layout->addWidget(pbProgress, 1, 0);

	pbAction = new QToolButton(parent);
	pbAction->setIcon(QIcon(QString(":/icons/stop.png")));
	layout->addWidget(pbAction, 1, 1);
	pbAction->connect(pbAction, SIGNAL(clicked()), clientObj, SLOT(abortFileTransfer()));

	lblSpeed = new QLabel("Speed: 0 Kb/s");
	layout->addWidget(lblSpeed, 2, 0, 1, 2);

	hLine = new QFrame(parent);
	hLine->setFrameShape(QFrame::HLine);
	layout->addWidget(hLine, 3, 0, 1, 2);
}

ClientUIBundle::~ClientUIBundle()
{
	clearLayout(layout);
	delete layout;
}

void ClientUIBundle::insertIntoLayout(int reverse_index, QVBoxLayout *parentLayout)
{
	parentLayout->insertLayout(parentLayout->count()-reverse_index, this->layout);//->insertLayout(layout->count()-reverse_index, this->layout);
}

void ClientUIBundle::removeFromLayout(QVBoxLayout *parentLayout)
{
	parentLayout->removeItem(this->layout);
}

void ClientUIBundle::update(qint64 value, double speed)
{
	pbProgress->setValue(value);
	lblSpeed->setText(QString("Speed: %1 Kb/s").arg(speed, 0, 'f', 2));
}

void ClientUIBundle::setFinished()
{
	layout->removeWidget(pbAction);
	delete pbAction;
	pbProgress->setValue(pbProgress->maximum());
}

void ClientUIBundle::setAborted()
{
	pbAction->setIcon(QIcon(":/icons/restart.png"));
	pbAction->disconnect();
	pbAction->connect(pbAction, SIGNAL(clicked()), client, SLOT(restartFileTransfer()));
	lblSpeed->setText(QString("Download Aborted"));
}


QToolButton* ClientUIBundle::getActionButton()
{
	return pbAction;
}

void ClientUIBundle::clearLayout(QLayout *layout)
{
	QLayoutItem *item;
	while((item = layout->takeAt(0)))
	{
		if(item->layout())
		{
			clearLayout(item->layout());
			delete item->layout();
		}
		else if(item->widget())
		{
			delete item->widget();
		}
		else if(item->spacerItem())
		{
			delete item->spacerItem();
		}
	}
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	connect(ui->pbGetFiles, SIGNAL(clicked()), this, SLOT(downloadFileList()));
	connect(ui->pbDownloadSelected, SIGNAL(clicked()), this, SLOT(requestFileDownload()));
	connect(ui->pbSelectDownloadDir, SIGNAL(clicked()), this, SLOT(selectNewSaveDirectory()));

	QString Octet = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
	ui->leServerIP->setValidator(new QRegExpValidator(QRegExp("^" + Octet
															  + "\\." + Octet
															  + "\\." + Octet
															  + "\\." + Octet + "$"), this));


	settings = new QSettings(tr("Martin Bakiev"), tr("QtFileTransfer"), this);
	if(settings->contains("client/ip"))
		ui->leServerIP->setText(settings->value("client/ip").toString());
	if(settings->contains("client/save_directory"))
		ui->leDownloadDir->setText(settings->value("client/save_directory").toString());

	tableModel = new FileListItemModel(this);

	ui->tvFileList->setModel(tableModel);
	ui->tvFileList->header()->setResizeMode(QHeaderView::ResizeToContents);
}

MainWindow::~MainWindow()
{
	delete settings;
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	emit cleanUpThreads();

	QHashIterator<DownloadClient*,DownloadWorkerBundle*> iter(workerHash);
	while(iter.hasNext())
	{
		iter.next();
		//This should wait for thread to quit properly
		delete iter.value();
	}

	event->setAccepted(true);
}

void MainWindow::downloadFileList()
{
	QHostAddress serverAddress;
	if(!getServerAddress(&serverAddress))
		return;

	settings->setValue("client/ip", ui->leServerIP->text());

	m_socket = new QTcpSocket(this);
	connect(m_socket, SIGNAL(connected()), this, SLOT(sock_connected()));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(sock_error(QAbstractSocket::SocketError)));

	m_socket->connectToHost(serverAddress, DEFAULT_SERVER_LISTEN_PORT);
}

void MainWindow::requestFileDownload()
{
	QHostAddress serverAddress;
	QString downloadDir = ui->leDownloadDir->text();

	if(!getServerAddress(&serverAddress))
		return;
	if(downloadDir.isEmpty())
	{
		QMessageBox::warning(this, tr("Please Select Download Location"), tr("Please select a directory to download the files to."), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	QModelIndex modelIndex = ui->tvFileList->currentIndex();

	if(!modelIndex.isValid())
		return;

	FileInfo *file = static_cast<FileInfo*>(modelIndex.internalPointer());
	if(!file || file->isDir())
		return;

	DownloadWorkerBundle *worker = new DownloadWorkerBundle();
	worker->client = new DownloadClient(file);

	worker->client->setServerAddress(serverAddress, DEFAULT_SERVER_LISTEN_PORT);
	worker->client->setSaveDirectory(downloadDir);

	worker->thread = new QThread(this);

	worker->client->moveToThread(worker->thread);

	connect(worker->client, SIGNAL(fileTransferBeginning(FileInfo*,DownloadClient*)),
			this, SLOT(fileTransferStarted(FileInfo*,DownloadClient*)));

	connect(worker->client, SIGNAL(fileTransferUpdate(qint64,double,DownloadClient*)),
			this, SLOT(fileTransferUpdated(qint64,double,DownloadClient*)));

	connect(worker->client, SIGNAL(fileTransferComplete(DownloadClient*)),
			this, SLOT(fileTransferCompleted(DownloadClient*)));

	connect(worker->client, SIGNAL(fileTransferAborted(qint64,DownloadClient*)),
			this, SLOT(fileTransferAborted(qint64,DownloadClient*)));

	connect(worker->thread, SIGNAL(started()), worker->client, SLOT(beginDownload()));

	connect(worker->client, SIGNAL(finished()), worker->thread, SLOT(quit()));

	worker->thread->start();

	workerHash.insert(worker->client, worker);
}


void MainWindow::selectNewSaveDirectory()
{
	QFileDialog dialog(this);

	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly, false);

	if(settings->contains("client/save_directory"))
		dialog.setDirectory(settings->value("client/save_directory").toString());

	QStringList selectedDir;
	if(dialog.exec())
		selectedDir = dialog.selectedFiles();

	if(selectedDir.count() > 0)
	{
		ui->leDownloadDir->setText(selectedDir.at(0));
		settings->setValue("client/save_directory", selectedDir.at(0));
	}
}

void MainWindow::sock_connected()
{
	if(!m_socket->isValid())
	{
		return;
	}
	list_ack_receieved = false;

	connControlMsg msg;
	msg.message = REQUEST_FILE_LIST;

	m_socket->write((char*)&msg, sizeof(msg));

	connect(m_socket, SIGNAL(readyRead()), this, SLOT(onListReceiveData()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(sock_disconn()));
}

void MainWindow::sock_error(QAbstractSocket::SocketError err)
{
	if(err == QAbstractSocket::ConnectionRefusedError)
	{
		QMessageBox::warning(this, tr("Connection Failed"),
							 tr("Could not connect to server. Please ensure the IP address is correct."),
							 QMessageBox::Ok, QMessageBox::Ok);
		m_socket->deleteLater();
	}
	else if(err != QAbstractSocket::RemoteHostClosedError)
	{
		QMessageBox::warning(this, tr("Network Error"),
							 tr("There was a network error, please try again later."),
							 QMessageBox::Ok, QMessageBox::Ok);
		m_socket->close();
	}
}

void MainWindow::sock_disconn()
{
	qDebug() << "No loose ends..";
	disconnect(m_socket, 0,0,0);
	m_socket->deleteLater();
}

void MainWindow::onListReceiveData()
{
	if(!list_ack_receieved)
	{
		connControlMsg msg;

		m_socket->read((char*)&msg, sizeof(msg));

		if(msg.message == LIST_REQUEST_REJECTED)
		{
			m_socket->close();
			return;
		}
		qDebug() << "Received ACK";
		list_ack_receieved = true;
	}

	unsigned int size;

	while(m_socket->bytesAvailable() > 0)
	{
		//Make sure we have enough data available to
		//read the entire message
		m_socket->peek((char*)&size, sizeof(size));
		if(m_socket->bytesAvailable() < size)
			return;

		m_socket->read((char*)&size, sizeof(size));

		char *buff = new char[size];
		m_socket->read(buff, size);

		FileInfo *fi = new FileInfo();

		fi->setFromByteArray(buff);

		this->tableModel->insertRowWithData(fi);
		delete [] buff;
	}
}


void MainWindow::fileTransferStarted(FileInfo* file, DownloadClient* dc)
{
	if(!workerHash.contains(dc))
		return;

	DownloadWorkerBundle *worker = workerHash.value(dc);

	worker->ui = new ClientUIBundle(file, dc, this);

	worker->ui->insertIntoLayout(1, ui->vlProgressBars);
}

void MainWindow::fileTransferUpdated(qint64 bytes, double speed, DownloadClient *dc)
{
	if(!workerHash.contains(dc))
		return;

	workerHash.value(dc)->ui->update(bytes, speed);
}

void MainWindow::fileTransferCompleted(DownloadClient *dc)
{
	if(!workerHash.contains(dc))
		return;

	workerHash.value(dc)->ui->setFinished();
	toRemove.enqueue(workerHash.value(dc));
	QTimer::singleShot(10000, this, SLOT(removeDownloadUI()));
}

void MainWindow::fileTransferAborted(qint64 bytes_recieved, DownloadClient *dc)
{
	if(!workerHash.contains(dc))
		return;

	workerHash.value(dc)->ui->update(bytes_recieved, 0.0);
	workerHash.value(dc)->ui->setAborted();
	//What to do now?
	// probably connect restart button to client slot that restarts the download where it left
	// off
}

void MainWindow::removeDownloadUI()
{
	DownloadWorkerBundle *worker = toRemove.dequeue();

	worker->ui->removeFromLayout(ui->vlProgressBars);
	delete worker;
}

bool MainWindow::getServerAddress(QHostAddress *addr)
{
	if(!addr->setAddress(ui->leServerIP->text()))
	{
		QMessageBox::warning(this, tr("Invalid Server Address"), tr("The entered server IP address is invalid, please correct it."), QMessageBox::Ok, QMessageBox::Ok);
		return false;
	}
	return true;
}
