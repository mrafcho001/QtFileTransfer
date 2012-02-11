#include "fileinfo.h"
#include <QDataStream>
#include <QDebug>
#include <QCryptographicHash>

FileInfo::FileInfo():
	m_id(0), m_size(0), m_isDir(false), m_parentId(-1)
{
	parentItem = NULL;
}

FileInfo::FileInfo(const FileInfo &file)
{
	this->m_id = file.m_id;
	this->m_isDir = file.m_isDir;
	this->m_name = file.m_name;
	this->m_parentId = file.m_parentId;
	this->m_path = file.m_path;
	this->m_sha1_id = file.m_sha1_id;
	this->m_size = file.m_size;

	this->childItems = file.childItems;
	this->parentItem = file.parentItem;
}

FileInfo::FileInfo(int id, qint64 size, int isDir, int parentId, QString name) :
	m_id(id), m_size(size), m_isDir(isDir), m_parentId(parentId), m_name(name)
{
	parentItem = NULL;
}

FileInfo::~FileInfo()
{
	while(childItems.size() > 0)
	{
		delete childItems.takeAt(0);
	}
}

void FileInfo::setSize(qint64 newSize)
{
	m_size = newSize;
}

qint64 FileInfo::getSize() const
{
	return m_size;
}

int FileInfo::getId() const
{
	return m_id;
}

void FileInfo::setId(int newId)
{
	m_id = newId;
}


int FileInfo::isDir() const
{
	return m_isDir;
}
void FileInfo::setDir(int isDir)
{
	m_isDir = isDir;
}

int FileInfo::getParentId() const
{
	return m_parentId;
}

void FileInfo::setParentId(int newParentId)
{
	m_parentId = newParentId;
}

void FileInfo::setName(const QString &name)
{
	m_name = name;
}

QString FileInfo::getName() const
{
	return m_name;
}


QByteArray FileInfo::getHash()
{
	return m_sha1_id;
}

void FileInfo::setPath(const QString &path)
{
	m_path = path;
	m_sha1_id = QCryptographicHash::hash(m_path.toAscii(), QCryptographicHash::Sha1);
}

QString FileInfo::getPath() const
{
	return m_path;
}


QList<FileInfo*> FileInfo::getChildList()
{
	return this->childItems;
}

QByteArray FileInfo::getByteArray() const
{
	QByteArray array;

	array.append((char*)&m_id, sizeof(m_id));
	array.append((char*)&m_size, sizeof(m_size));
	array.append((char*)&m_isDir, sizeof(m_isDir));
	array.append((char*)&m_parentId, sizeof(m_parentId));
	array.append(m_sha1_id);
	array.append(m_name.toAscii());
	array.append('\0');
	int size = array.size();
	array.prepend((char*)&size, sizeof(size));

	return array;
}

bool FileInfo::setFromByteArray(char *buff)
{
	unsigned int pos = 0;
	memcpy((char*)&m_id, &buff[pos], sizeof(m_id));
	pos+= sizeof(m_id);

	memcpy((char*)&m_size, &buff[pos], sizeof(m_size));
	pos+=sizeof(m_size);

	memcpy((char*)&m_isDir, &buff[pos], sizeof(m_isDir));
	pos+=sizeof(m_isDir);

	memcpy((char*)&m_parentId, &buff[pos], sizeof(m_parentId));
	pos+=sizeof(m_parentId);

	//MAGIC
	m_sha1_id.append(&buff[pos], 20);
	pos+=20;

	m_name = QString(&buff[pos]);
	return 0;
}

FileInfo* FileInfo::child(unsigned int index)
{
	if(index >= (unsigned int)childItems.size())
		return NULL;

	return childItems.value(index);
}

int FileInfo::childIndex() const
{
	if(parentItem)
		return parentItem->childItems.indexOf(const_cast<FileInfo*>(this));

	return 0;
}

int FileInfo::childCount() const
{
	return childItems.size();
}

void FileInfo::appendChild(FileInfo* child)
{
	childItems.append(child);
	child->parentItem = this;
}

void FileInfo::insertChild(unsigned int pos, FileInfo* child)
{
	if(pos >= (unsigned int)childItems.size() && pos != 0)
		pos = childItems.size()-1;

	childItems.insert(pos, child);
	child->parentItem = this;
}

void FileInfo::insertBlank(unsigned int pos, unsigned int count = 1)
{

	if(pos >= (unsigned int)childItems.size() && pos != 0)
		pos = childItems.size()-1;

	for(unsigned int i = 0; i < count; i++)
		childItems.insert(pos+i, new FileInfo());
}

void FileInfo::removeChild(unsigned int pos, unsigned int count = 1)
{
	if(pos >= (unsigned int)childItems.size() && pos != 0)
		pos = childItems.size()-1;

	if(count > childItems.size() - pos)
		count = childItems.size() - pos;

	for(unsigned int i = 0; i < count; i++)
		delete childItems.takeAt(pos+i);
}


FileInfo *FileInfo::parent()
{
	return parentItem;
}



int FileInfo::m_nextID = 0;

int FileInfo::indexOfByID(int id)
{
	for(int i = 0; i< childItems.count(); i++)
	{
		if(childItems.value(i)->getId() == id)
			return i;
	}
	return -1;
}

int FileInfo::nextID()
{
	m_nextID++;
	return  m_nextID;
}

void FileInfo::resetID()
{
	m_nextID = 0;
}
