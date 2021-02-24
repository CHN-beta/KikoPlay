#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include "framelessdialog.h"
class QPlainTextEdit;
class InputDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    InputDialog(const QString &title, const QString &tip, const QString &text="",
                         bool canEmpty=true, QWidget *parent = nullptr);

    InputDialog(const QString &title, const QString &tip, QWidget *parent = nullptr);
    InputDialog(const QByteArray &imgData, const QString &title, const QString &tip, const QString &text, QWidget *parent = nullptr);
    InputDialog(const QByteArray &imgData, const QString &title, const QString &tip, QWidget *parent = nullptr);
    QString text;
private:
    QPlainTextEdit *edit;
    bool canEmpty;
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};

#endif // INPUTDIALOG_H
