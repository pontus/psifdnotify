TEMPLATE = subdirs

include(conf.pri)
windows:include(conf_windows.pri)

# configure iris
unix:system("echo \"include(../src/conf_iris.pri)\" > iris/conf.pri")
windows:system("echo include(../src/conf_iris.pri) > iris\\conf.pri")

qca-static {
	SUBDIRS += third-party/qca
}

SUBDIRS += \
	iris \
	src
