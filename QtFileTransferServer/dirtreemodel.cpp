#include "dirtreemodel.h"
#include <QDir>
#include <QDebug>

DirTreeModel::DirTreeModel(QObject *parent) :
	QAbstractItemModel(parent)
{
	rootItem = new FileInfo(0, 0, true, -1, tr("Root"));
}

DirTreeModel::~DirTreeModel()
{
	delete rootItem;
}

QModelIndex DirTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	FileInfo *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<FileInfo*>(parent.internalPointer());

	FileInfo *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);

	return QModelIndex();
}

QModelIndex DirTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	FileInfo *childItem = static_cast<FileInfo*>(index.internalPointer());
	FileInfo *parentItem = childItem->parent();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->childIndex(), 0, parentItem);
}

int DirTreeModel::rowCount(const QModelIndex &parent) const
{
	FileInfo *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<FileInfo*>(parent.internalPointer());

	return parentItem->childCount();
}

int DirTreeModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 4;
}

QVariant DirTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	FileInfo *item = static_cast<FileInfo*>(index.internalPointer());

	if(index.column() == 0)
	{
		if(item->getParentId() == 0)
			return item->getPath();
		else
			return item->getName();
	}
	else if(index.column() == 1)
		return item->getSize();
	else if(index.column() == 2)
		return item->getId();
	else if(index.column() == 3)
	{
		return QString(item->getHash().toHex());
	}

	return QVariant();
}
bool DirTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::EditRole)
	{
		FileInfo *item = NULL;
		if(index.isValid())
			item = static_cast<FileInfo*>(index.internalPointer());

		if(!item)
			item = this->rootItem;

		switch(index.column())
		{
		case 0:
			item->setName(value.toString());
			break;
		case 1:
			item->setSize(value.toLongLong());
			break;
		case 2:
			item->setId(value.toInt());
			break;
		default:
			return false;
		}

		emit dataChanged(index, index);
		return true;
	}

	return false;
}

QVariant DirTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();

	if(orientation == Qt::Horizontal)
	{
		switch(section)
		{
		case 0:
			return tr("Name");

		case 1:
			return tr("Size");

		case 2:
			return tr("ID");

		case 3:
			return tr("Hash");

		default:
			return QVariant();
		}
	}

	return QVariant();
}

bool DirTreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
	(void)row;
	(void)count;

	if(!parent.isValid())
		return false;

	FileInfo *childItem = static_cast<FileInfo*>(parent.internalPointer());
	qDebug() << "Valid: " << childItem->getName();

	if(childItem->getParentId() == 0)
	{
		qDebug () << "Top Level Entry";
		int index = childItem->childIndex();
		beginRemoveRows(parent.parent(), index, index);
		removeFromHashRecursive(rootItem->child(index));
		rootItem->removeChild(index, 1);
		endRemoveRows();
	}

	return true;
}

Qt::ItemFlags DirTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool DirTreeModel::addDirectory(const QString &directory)
{
	return addDirectory(directory, this->rootItem, QModelIndex(), false);
}

bool DirTreeModel::addDirectory(const QString &directory, FileInfo *parentItem, QModelIndex &parent)
{

	return addDirectory(directory, parentItem, parent, false);
}

bool DirTreeModel::addDirectory(const QString &directory, FileInfo *parentItem, const QModelIndex &parent, bool addEmpty)
{
	QDir dir(directory);

	if(!dir.exists())
	{
		qWarning() << directory << " is not a directory.";
		return false;
	}

	if(dir.count() == 0)
	{
		qWarning() << directory << " is an empty directory.";
		if(!addEmpty)
			return false;
	}

	FileInfo *dirInfo = new FileInfo();
	dirInfo->setDir(1);
	dirInfo->setName(dir.dirName());
	dirInfo->setPath(dir.absolutePath());
	dirInfo->setParentId(parentItem->getId());
	dirInfo->setId(FileInfo::nextID());

	beginInsertRows(parent, parentItem->childCount(), parentItem->childCount());
	parentItem->appendChild(dirInfo);
	hash.insert(dirInfo->getHash(), dirInfo);
	endInsertRows();

	QFileInfoList fileList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst);
	QModelIndex dirIndex = index(parentItem->childCount()-1, 0, parent);

	for(int i = 0; i < fileList.count(); i++)
	{
		if(fileList.value(i).isDir())
		{
			QString path = fileList.at(i).absoluteFilePath();
			addDirectory(path, dirInfo, dirIndex);
			continue;
		}

		FileInfo *file = new FileInfo();

		file->setDir(0);
		file->setId(FileInfo::nextID());
		file->setName(fileList.at(i).fileName());
		file->setParentId(dirInfo->getId());
		file->setPath(fileList.at(i).absoluteFilePath());
		file->setSize(fileList.at(i).size());

		beginInsertRows(parent, parentItem->childCount(), parentItem->childCount());
		dirInfo->appendChild(file);
		hash.insert(file->getHash(), file);
		endInsertRows();
	}
	return true;
}

QList<FileInfo*> DirTreeModel::getSharedDirList()
{
	return rootItem->getChildList();
}

QList<FileInfo*> DirTreeModel::getSerializedList(FileInfo* root)
{
	if(root == NULL)
		root = this->rootItem;
	QList<FileInfo*> serializedList;

	QList<FileInfo*> childList = root->getChildList();

	for(int i = 0; i < childList.count(); i++)
	{
		serializedList.append(childList.value(i));

		if(childList.at(i)->isDir())
			serializedList.append(getSerializedList(childList.value(i)));
	}
	return serializedList;
}


void DirTreeModel::removeFromHashRecursive(FileInfo* item)
{
	if(item == NULL)
		return;

	for(int i = 0; i < item->childCount(); i++)
	{
		if(item->child(i)->isDir())
			removeFromHashRecursive(item->child(i));
		hash.remove(item->getHash());
	}
	hash.remove(item->getHash());
}
