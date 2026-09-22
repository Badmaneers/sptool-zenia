#ifndef PAYLOADWIDGET_H
#define PAYLOADWIDGET_H

#include "TabWidgetBase.h"
#include <QProcess>

namespace Ui
{
class PayloadWidget;
}

class MainWindow;

class PayloadWidget : public TabWidgetBase
{
    Q_OBJECT

public:
    PayloadWidget(QTabWidget *parent, MainWindow *window);
    ~PayloadWidget();

    DECLARE_TABWIDGET_VFUNCS()

private slots:
    void on_btnRun_clicked();
    void on_btnStop_clicked();
    void on_btnBrowseLoader_clicked();
    void processReadyReadStdOut();
    void processReadyReadStdErr();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);

private:
    MainWindow *main_window_;
    Ui::PayloadWidget *ui_;
    QProcess *process_;
    QString app_dir_;

    QString payloadBinaryPath() const;
    QStringList buildArgs() const;
    void setRunning(bool running);
    void appendLog(const QString &text, bool isStderr = false);
    void startPayload();
    void stopPayload();
};

#endif // PAYLOADWIDGET_H
