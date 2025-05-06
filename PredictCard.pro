QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp\
    image_select.cpp\
    tools.cpp\
    plot.cpp

HEADERS += \
    mainwindow.h\
    image_select.h\
    tools.h\
    plot.h\

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# —— vcpkg 安装路径 ——
VCPKG_ROOT = C:/Users/xiaoyu/vcpkg

VCPKG_LIB = $$VCPKG_ROOT/installed/x64-windows/lib

INCLUDEPATH += $$VCPKG_ROOT/installed/x64-windows/include/opencv4
INCLUDEPATH += $$VCPKG_ROOT/installed/x64-windows/include
LIBS += $$files("$${VCPKG_LIB}/opencv_*4.lib")

# 运行时加载 DLL
win32:CONFIG(release, debug|release): PATH += $$VCPKG_ROOT/installed/x64-windows/bin
win32:CONFIG(debug, debug|release):    PATH += $$VCPKG_ROOT/installed/x64-windows/bin



