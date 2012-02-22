TEMPLATE = \
	subdirs

SUBDIRS += \
	QtFileTransferServer \
	QtFileTransferClient

SOURCES = sharedstructures.cpp \
	fileinfo.cpp \
	uibundle.cpp

HEADERS = sharedstructures.h \
	fileinfo.h \
	uibundle.h
