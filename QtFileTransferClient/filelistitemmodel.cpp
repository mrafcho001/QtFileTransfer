#include "filelistitemmodel.h"
#include <QDebug>

FileListItemModel::FileListItemModel(QObject *parent) :
	QAbstractItemModel(parent)
{
	rootItem = new FileInfo(0, 0, true, -1, tr("Root"));
}

FileListItemModel::~FileListItemModel()
{
	delete rootItem;
}

FileInfo *FileListItemModel::getItem(const QModelIndex &index) const
{
	if (index.isValid()) {
		FileInfo *item = static_cast<FileInfo*>(index.internalPointer());
		if (item) return item;
	}
	return rootItem;
}

QModelIndex FileListItemModel::index(int row, int column, const QModelIndex &parent) const
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

int FileListItemModel::rowCount(const QModelIndex &parent) const
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

int FileListItemModel::columnCount(const QModelIndex &parent) const
{
	return 3;
}

QModelIndex FileListItemModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	FileInfo *childItem = static_cast<FileInfo*>(index.internalPointer());
	FileInfo *parentItem = childItem->parent();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->childIndex(), 0, parentItem);
}

QVariant FileListItemModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	FileInfo *item = static_cast<FileInfo*>(index.internalPointer());

	if(index.column() == 0)
		return item->getName();
	else if(index.column() == 1)
		return item->getSize();
	else if(index.column() == 2)
		return item->getId();

	return QVariant();
}

QVariant FileListItemModel::headerData(int section, Qt::Orientation orientation, int role) const
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

		default:
			return QVariant();
		}
	}

	return QVariant();
}

/*
bool FileListTableModel::insertRows(int row, int count, const QModelIndex &parent)
{
	beginInsertRows(parent, row, count+row-1);

	for(int i = 0; i < count; i++)
	{
		FileInfo f1(-1, 0, tr("N/A"));
		fileList.insert(row, f1);
	}
	endInsertRows();
	return true;
}

bool FileListTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
	beginRemoveRows(parent, row, row+count-1);

	for(int i = 0; i < count; i++)
	{
		fileList.removeAt(i);
	}

	endRemoveRows();

	return true;
}
*/

bool FileListItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::EditRole)
	{
		FileInfo *item = getItem(index);

		switch(index.column())
		{
		case 0:
		{
			QString tmp = value.toString();
			item->setName(&tmp);
		}
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

bool FileListItemModel::appendRowWithData(FileInfo & fileInfo, QModelIndex &parent)
{
	FileInfo *parentItem;
	QModelIndex par;
	parent = this->index(0, 0, QModelIndex());
	if(fileInfo.getParentId() == 0)
	{
		parentItem = rootItem;
		par = parent;
	}
	else
	{
		if(parent.isValid())
		{
			FileInfo *p = (FileInfo*)parent.internalPointer();
			for(int i = 0; parent.sibling(i, 0).isValid(); i++)
			{
				if(((const FileInfo*)parent.sibling(i, 0).internalPointer())->getId() == fileInfo.getParentId())
				{
					par = parent.sibling(i,0);
					break;
				}
			}
		}
		parentItem = (FileInfo*)par.internalPointer();
	}

	int row = parentItem->childCount();

	beginInsertRows(par, row, row);
	FileInfo *file2 = new FileInfo();
	file2->setId(fileInfo.getId());
	QString name = fileInfo.getName();
	file2->setName(&name);
	file2->setSize(fileInfo.getSize());

	parentItem->appendChild(file2);
	endInsertRows();

	qDebug() << "Root now has " << rootItem->childCount() << "children";
	return true;
}

Qt::ItemFlags FileListItemModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

