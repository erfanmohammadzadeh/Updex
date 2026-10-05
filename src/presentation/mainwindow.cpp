#include "presentation/mainwindow.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

MainWindow::MainWindow(UpdateModel *model, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(model)
{
    ui->setupUi(this);
    ui->textEditLog->setReadOnly(true);
    ui->textEditLog->setAcceptRichText(false);

    connect(m_model, &UpdateModel::changed, this, &MainWindow::refresh);
    connect(m_model, &UpdateModel::logAppended, this, &MainWindow::appendLog);
    connect(ui->pushButtonSetExePath, &QPushButton::clicked, this, &MainWindow::browseForProgram);
    connect(ui->lineEditAppExePath, &QLineEdit::editingFinished, this, &MainWindow::publishSettings);
    connect(ui->lineEditRepository, &QLineEdit::editingFinished, this, &MainWindow::publishSettings);
    connect(ui->lineEditToken, &QLineEdit::editingFinished, this, &MainWindow::publishSettings);
    connect(ui->pushButtonCheckForUpdate, &QPushButton::clicked, this, [this]() {
        publishSettings();
        emit checkRequested();
    });
    connect(ui->pushButtonUpdate, &QPushButton::clicked, this, [this]() {
        publishSettings();
        emit updateRequested();
    });

    refresh();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::refresh()
{
    m_refreshing = true;
    const bool busy = m_model->busy();
    auto show = [](QLineEdit *edit, const QString &value) {
        if (!edit->hasFocus() && edit->text() != value)
            edit->setText(value);
    };
    show(ui->lineEditAppExePath, m_model->appPath());
    show(ui->lineEditRepository, m_model->repository());
    show(ui->lineEditToken, m_model->token());

    ui->labelInstalledVersionValue->setText(m_model->currentVersion());
    ui->labelLatestVersionValue->setText(m_model->latestVersion());
    ui->progressBar->setValue(m_model->progress());
    statusBar()->showMessage(m_model->status());

    ui->lineEditAppExePath->setEnabled(!busy);
    ui->lineEditRepository->setEnabled(!busy);
    ui->lineEditToken->setEnabled(!busy);
    ui->pushButtonSetExePath->setEnabled(!busy);
    ui->pushButtonCheckForUpdate->setEnabled(!busy);
    ui->pushButtonUpdate->setEnabled(!busy);
    m_refreshing = false;
}

void MainWindow::appendLog(const QString &line)
{
    const QStringList rows = line.split(QLatin1Char('\n'));
    for (const QString &row : rows) {
        if (!row.trimmed().isEmpty())
            ui->textEditLog->append(row);
    }
}

void MainWindow::publishSettings()
{
    if (m_refreshing)
        return;
    emit settingsEdited(ui->lineEditAppExePath->text(),
                        ui->lineEditRepository->text(),
                        ui->lineEditToken->text());
}

void MainWindow::browseForProgram()
{
    QString start = QCoreApplication::applicationDirPath();
    const QString current = ui->lineEditAppExePath->text().trimmed();
    if (!current.isEmpty()) {
        QFileInfo info(current);
        if (info.isRelative())
            info.setFile(QDir(start).filePath(info.filePath()));
        if (!info.absolutePath().isEmpty())
            start = info.absolutePath();
    }

    const QString chosen = QFileDialog::getOpenFileName(this,
                                                        QStringLiteral("Select program"),
                                                        start,
                                                        QStringLiteral("Programs (*.exe)"));
    if (chosen.isEmpty())
        return;
    ui->lineEditAppExePath->setText(chosen);
    publishSettings();
}
