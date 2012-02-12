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

ProgressBarBundleClient::ProgressBarBundleClient()
{
	bar = NULL;
	label = NULL;
	file = NULL;
	client = NULL;
}

ProgressBarBundleClient::ProgressBarBundleClient(FileInfo* file, DownloadClient *clientObj,
												 QWidget *parent)
{
	this->file = file;
	client = clientObj;
	label = new QLabel(file->getName(), parent);
	bar = new QProgressBar(parent);
	bar->setMaximum(file->getSize());
	bar->setValue(0);
	bar->setTextVisible(true);
}

ProgressBarBundleClient::~ProgressBarBundleClient()
{
	if(bar) delete bar;
	if(label) delete label;
}

void ProgressBarBundleClient::insertIntoLayout(int reverse_index, QVBoxLayout *layout)
{
	layout->insertWidget(layout->count()-reverse_index, label);
	layout->insertWidget(layout->count()-reverse_index, bar);
}

void ProgressBarBundleClient::removeFromLayout(QVBoxLayout *layout)
{
	layout->removeWidget(label);
	layout->removeWidget(bar);

	delete bar; bar = NULL;
	delete label; label = NULL;
}

void ProgressBarBundleClient::update(qint64 value)
{
	bar->setValue(value);
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

void MainWindow::downloadFileList()
{
	QHostAddress serverAddress;
	if(!getServerAddress(&serverAddress))
		return;

	settings->setValue("client/ip", ui->leServerIP->text());

	m_socket = new QTcpSocket(this);
	m_socket->connectToHost(serverAddress, DEFAULT_SERVER_LISTEN_PORT);

	if(!m_socket->isValid())
	{
		QMessageBox::warning(this, tr("Connection Failed"), tr("Could not connect to server. Please ensure the IP address is correct."), QMessageBox::Ok, QMessageBox::Ok);
		m_socket->deleteLater();
		return;
	}
	list_ack_receieved = false;

	connControlMsg msg;
	msg.message = REQUEST_FILE_LIST;

	m_socket->write((char*)&msg, sizeof(msg));

	connect(m_socket, SIGNAL(readyRead()), this, SLOT(onListReceiveData()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(sock_disconn()));
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
	if(!file)
		return;

	DownloadClient *client = new DownloadClient(file);

	if(!getServerAddress(&serverAddress))
		return;

	client->setServerAddress(serverAddress, DEFAULT_SERVER_LISTEN_PORT);
	client->setSaveDirectory(downloadDir);

	QThread *thread = new QThread(this);

	client->moveToThread(thread);

	connect(client, SIGNAL(fileTransferBeginning(FileInfo*,DownloadClient*)),
			this, SLOT(fileTransferStarted(FileInfo*,DownloadClient*)));
	connect(client, SIGNAL(fileTransferUpdate(qint64,DownloadClient*)),
			this, SLOT(fileTransferUpdated(qint64,DownloadClient*)));
	connect(client, SIGNAL(fileTransferComplete(DownloadClient*)),
			this, SLOT(fileTransferCompleted(DownloadClient*)));

	connect(thread, SIGNAL(started()), client, SLOT(beginDownload()));
	connect(client, SIGNAL(finished()), thread, SLOT(quit()));

	thread->start();
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

void MainWindow::sock_disconn()
{
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
	ProgressBarBundleClient *pbb = new ProgressBarBundleClient(file, dc, this);

	pbb->insertIntoLayout(1, ui->vlProgressBars);

	activeDownloads.insert(dc, pbb);
}

void MainWindow::fileTransferUpdated(qint64 bytes, DownloadClient *dc)
{
	if(!activeDownloads.contains(dc))
		return;

	activeDownloads.value(dc)->update(bytes);
}

void MainWindow::fileTransferCompleted(DownloadClient *dc)
{
	toRemove.enqueue(dc);
	QTimer::singleShot(10000, this, SLOT(removeProgresBar()));
}

void MainWindow::removeProgresBar()
{
	DownloadClient *obj = toRemove.dequeue();
	activeDownloads.value(obj)->removeFromLayout(ui->vlProgressBars);
	delete activeDownloads.value(obj);
	activeDownloads.remove(obj);
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
