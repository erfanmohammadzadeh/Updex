#include "application/ApplyUpdate.h"
#include "application/CheckForUpdate.h"
#include "infrastructure/ExecutableDiscovery.h"
#include "infrastructure/GitHubReleaseRepository.h"
#include "infrastructure/JsonSettingsStore.h"
#include "infrastructure/WindowsApplicationProbe.h"
#include "infrastructure/ZipPackageInstaller.h"
#include "presentation/UpdateController.h"
#include "presentation/UpdateModel.h"
#include "presentation/mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QFile styleFile(QStringLiteral(":/Resource/style.qss"));
    if (styleFile.open(QFile::ReadOnly | QFile::Text))
        application.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    application.setWindowIcon(QIcon(QStringLiteral(":/Resource/icon.ico")));

    GitHubReleaseRepository releases;
    WindowsApplicationProbe probe;
    ZipPackageInstaller installer;
    JsonSettingsStore settings(QCoreApplication::applicationDirPath());
    ExecutableDiscovery discovery;
    CheckForUpdate checkForUpdate(releases, probe);
    ApplyUpdate applyUpdate(installer);

    UpdateModel model;
    UpdateController controller(model, checkForUpdate, applyUpdate, settings, discovery, probe);
    MainWindow window(&model);
    controller.attach(&window);
    controller.start();
    window.show();
    return application.exec();
}
