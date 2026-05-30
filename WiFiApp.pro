QT       += core gui network widgets
QT       +=bluetooth androidextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SmartHome
TEMPLATE = app

# Qt 5.14.2 移动平台配置
CONFIG += c++11

# Android 平台
android {
    QT += androidextras
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}

# iOS 平台
ios {
    QMAKE_INFO_PLIST = $$PWD/ios/Info.plist
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    tcpclient.cpp

HEADERS += \
    mainwindow.h \
    tcpclient.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    android/res/values/strings.xml

android {
    ANDROID_ABIS = armeabi-v7a arm64-v8a x86 x86_64
}
