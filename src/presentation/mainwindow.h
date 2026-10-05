#pragma once

#include "presentation/UpdateModel.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(UpdateModel *model, QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void checkRequested();
    void updateRequested();
    void settingsEdited(const QString &appPath, const QString &repository, const QString &token, bool scheduleEnabled, int checkEvery, const QString &checkUnit);

private:
    void refresh();
    void appendLog(const QString &line);
    void publishSettings();
    void browseForProgram();

    Ui::MainWindow *ui;
    UpdateModel *m_model;
    bool m_refreshing = false;
};
