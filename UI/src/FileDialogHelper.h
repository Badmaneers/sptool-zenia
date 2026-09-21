#ifndef FILEDIALOGHELPER_H
#define FILEDIALOGHELPER_H

#include <QFileDialog>
#include <QApplication>
#include <QStyleFactory>

class FileDialogHelper {
public:
    static QString getOpenFileName(QWidget *parent, const QString &caption,
                                   const QString &dir = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = nullptr,
                                   QFileDialog::Options options = QFileDialog::Options()) {
        return execDialog([&]() {
            return QFileDialog::getOpenFileName(parent, caption, dir, filter, selectedFilter, options);
        });
    }

    static QString getSaveFileName(QWidget *parent, const QString &caption,
                                    const QString &dir = QString(),
                                    const QString &filter = QString(),
                                    QString *selectedFilter = nullptr,
                                    QFileDialog::Options options = QFileDialog::Options()) {
        return execDialog([&]() {
            return QFileDialog::getSaveFileName(parent, caption, dir, filter, selectedFilter, options);
        });
    }

    static QString getExistingDirectory(QWidget *parent, const QString &caption,
                                         const QString &dir = QString(),
                                         QFileDialog::Options options = QFileDialog::Options()) {
        return execDialog([&]() {
            return QFileDialog::getExistingDirectory(parent, caption, dir, options);
        });
    }

private:
    template<typename Func>
    static QString execDialog(Func func) {
        QString currentStyle = qApp->style()->objectName();
        QString currentSheet = qApp->styleSheet();
        qApp->setStyle(QStyleFactory::create("windows"));
        qApp->setStyleSheet("");
        QString result = func();
        qApp->setStyle(QStyleFactory::create(currentStyle));
        qApp->setStyleSheet(currentSheet);
        return result;
    }
};

#endif // FILEDIALOGHELPER_H
