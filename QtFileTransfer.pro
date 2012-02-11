TEMPLATE = \
	subdirs

SUBDIRS += \
	QtFileTransferServer \
	QtFileTransferClient

SOURCES = sharedstructures.cpp \
	fileinfo.cpp

HEADERS = sharedstructures.h \
	fileinfo.h
