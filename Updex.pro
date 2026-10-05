QT += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += $$PWD/src

SOURCES += \
    main.cpp \
    src/application/ApplyUpdate.cpp \
    src/application/AssetSelector.cpp \
    src/application/CheckForUpdate.cpp \
    src/domain/Version.cpp \
    src/infrastructure/ExecutableDiscovery.cpp \
    src/infrastructure/GitHubReleaseRepository.cpp \
    src/infrastructure/GitHubRepoParser.cpp \
    src/infrastructure/JsonSettingsStore.cpp \
    src/infrastructure/WinHttpClient.cpp \
    src/infrastructure/WindowsApplicationProbe.cpp \
    src/infrastructure/ZipPackageInstaller.cpp \
    src/presentation/UpdateController.cpp \
    src/presentation/UpdateModel.cpp \
    src/presentation/mainwindow.cpp

HEADERS += \
    src/application/ApplyUpdate.h \
    src/application/AssetSelector.h \
    src/application/CheckForUpdate.h \
    src/application/Text.h \
    src/application/UpdateResults.h \
    src/application/UpdaterSettings.h \
    src/application/ports/IApplicationProbe.h \
    src/application/ports/IExecutableDiscovery.h \
    src/application/ports/IPackageInstaller.h \
    src/application/ports/IReleaseRepository.h \
    src/application/ports/ISettingsStore.h \
    src/domain/InstalledApplication.h \
    src/domain/Release.h \
    src/domain/Version.h \
    src/infrastructure/ExecutableDiscovery.h \
    src/infrastructure/GitHubReleaseRepository.h \
    src/infrastructure/GitHubRepoParser.h \
    src/infrastructure/JsonSettingsStore.h \
    src/infrastructure/TextConvert.h \
    src/infrastructure/WinHttpClient.h \
    src/infrastructure/WindowsApplicationProbe.h \
    src/infrastructure/ZipPackageInstaller.h \
    src/presentation/UpdateController.h \
    src/presentation/UpdateModel.h \
    src/presentation/mainwindow.h

FORMS += \
    src/presentation/mainwindow.ui

RESOURCES += \
    Resource.qrc

win32:RC_FILE += Updex.rc
win32:LIBS += -lwinhttp -lversion

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
