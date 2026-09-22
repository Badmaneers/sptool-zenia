#include "PayloadWidget.h"
#include "MainWindow.h"
#include "ui_PayloadWidget.h"
#include "../../Utility/FileUtils.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

PayloadWidget::PayloadWidget(QTabWidget *parent, MainWindow *window) :
    TabWidgetBase(3, tr("&Payload"), parent),
    main_window_(window),
    ui_(new Ui::PayloadWidget),
    process_(NULL)
{
    ui_->setupUi(this);

    app_dir_ = QString::fromStdString(FileUtils::GetAppDirectory());

    ui_->comboPtype->addItem("Auto (Kamakiri2)", "");
    ui_->comboPtype->addItem("Kamakiri", "kamakiri");
    ui_->comboPtype->addItem("Kamakiri2", "kamakiri2");
    ui_->comboPtype->addItem("Amonet", "amonet");
    ui_->comboPtype->addItem("Hashimoto", "hashimoto");
    ui_->comboPtype->addItem("Carbonara", "carbonara");

    setRunning(false);
}

PayloadWidget::~PayloadWidget()
{
    stopPayload();
    if(ui_)
    {
        delete ui_;
        ui_ = NULL;
    }
}

void PayloadWidget::LockOnUI()
{
}

void PayloadWidget::DoFinished()
{
}

void PayloadWidget::UpdateUI()
{
    ui_->retranslateUi(this);
}

void PayloadWidget::SetShortCut(int cmd, const QString &shortcut)
{
    (void)cmd; (void)shortcut;
}

void PayloadWidget::SetTabLabel(QTabWidget *tab_widget, int index)
{
    tab_widget->setTabText(index, tr("&Payload"));
}

QString PayloadWidget::payloadBinaryPath() const
{
    return app_dir_ + QDir::separator() + "payload" + QDir::separator() + "mtk_payload";
}

QStringList PayloadWidget::buildArgs() const
{
    QStringList args;
    args << "payload";

    int ptype_idx = ui_->comboPtype->currentIndex();
    QString ptype_val = ui_->comboPtype->itemData(ptype_idx).toString();
    if (!ptype_val.isEmpty())
        args << "--ptype" << ptype_val;

    QString loader = ui_->lineLoader->text().trimmed();
    if (!loader.isEmpty())
        args << "--loader" << loader;

    if (ui_->checkEnforceCrash->isChecked())
        args << "--crash";

    if (ui_->checkDebug->isChecked())
        args << "--debugmode";

    return args;
}

void PayloadWidget::setRunning(bool running)
{
    ui_->btnRun->setEnabled(!running);
    ui_->btnStop->setEnabled(running);
    ui_->groupOptions->setEnabled(!running);
    ui_->comboPtype->setEnabled(!running);
    ui_->btnBrowseLoader->setEnabled(!running);
}

void PayloadWidget::appendLog(const QString &text, bool isStderr)
{
    (void)isStderr;
    ui_->logOutput->appendPlainText(text);
}

void PayloadWidget::on_btnRun_clicked()
{
    startPayload();
}

void PayloadWidget::on_btnStop_clicked()
{
    stopPayload();
}

void PayloadWidget::on_btnBrowseLoader_clicked()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        tr("Select DA Loader"),
        app_dir_,
        tr("Binary files (*.bin);;All files (*)"));

    if (!file.isEmpty())
        ui_->lineLoader->setText(file);
}

void PayloadWidget::startPayload()
{
    QString binary = payloadBinaryPath();
    if (!QFileInfo::exists(binary)) {
        QMessageBox::warning(this, tr("Payload Tool"),
            tr("mtk_payload binary not found at:\n%1\n\nMake sure the payload tool is bundled in the dist/payload/ directory.").arg(binary));
        return;
    }

    if (process_ && process_->state() == QProcess::Running) {
        process_->terminate();
        process_->waitForFinished(3000);
    }

    process_ = new QProcess(this);
    connect(process_, SIGNAL(readyReadStandardOutput()), this, SLOT(processReadyReadStdOut()));
    connect(process_, SIGNAL(readyReadStandardError()), this, SLOT(processReadyReadStdErr()));
    connect(process_, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(process_, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));

    ui_->logOutput->clear();

    QStringList args = buildArgs();

    appendLog("$ " + binary + " " + args.join(" "));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // Remove LD_PRELOAD so libpatch_brom.so shim does not interfere with libusb
    env.remove("LD_PRELOAD");

    process_->setProcessEnvironment(env);
    process_->setWorkingDirectory(app_dir_ + QDir::separator() + "payload");

    process_->start(binary, args);
    if (!process_->waitForStarted(5000)) {
        appendLog("error: failed to start mtk_payload");
        setRunning(false);
        delete process_;
        process_ = NULL;
        return;
    }

    ui_->labelStatus->setText(tr("Running..."));
    setRunning(true);
}

void PayloadWidget::stopPayload()
{
    if (process_ && process_->state() == QProcess::Running) {
        process_->terminate();
        process_->waitForFinished(3000);
        if (process_->state() == QProcess::Running)
            process_->kill();
    }
}

void PayloadWidget::processReadyReadStdOut()
{
    if (!process_) return;
    QByteArray data = process_->readAllStandardOutput();
    QString text = QString::fromLocal8Bit(data);
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines)
        appendLog(line);
}

void PayloadWidget::processReadyReadStdErr()
{
    if (!process_) return;
    QByteArray data = process_->readAllStandardError();
    QString text = QString::fromLocal8Bit(data);
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines)
        appendLog(line, true);
}

void PayloadWidget::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    setRunning(false);

    if (exitStatus == QProcess::NormalExit && exitCode == 0)
        ui_->labelStatus->setText(tr("Done — device in BROM, switch to Download tab"));
    else
        ui_->labelStatus->setText(tr("Failed (exit code %1)").arg(exitCode));

    if (process_) {
        process_->deleteLater();
        process_ = NULL;
    }
}

void PayloadWidget::processError(QProcess::ProcessError error)
{
    setRunning(false);
    switch (error) {
        case QProcess::FailedToStart:
            appendLog("error: mtk_payload failed to start (binary missing or no permission)");
            break;
        case QProcess::Crashed:
            appendLog("warning: mtk_payload process crashed");
            break;
        case QProcess::Timedout:
            appendLog("error: mtk_payload timed out");
            break;
        case QProcess::WriteError:
        case QProcess::ReadError:
            appendLog("error: mtk_payload I/O error");
            break;
        default:
            appendLog("error: mtk_payload unknown error");
            break;
    }

    ui_->labelStatus->setText(tr("Error"));
    if (process_) {
        process_->deleteLater();
        process_ = NULL;
    }
}
