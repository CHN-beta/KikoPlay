#ifndef SETTINGPAGE_H
#define SETTINGPAGE_H
#include <QWidget>
#include <QVariant>
#include <QHash>
class SettingPage : public QWidget
{
    Q_OBJECT
public:
    using QWidget::QWidget;
    virtual void onAccept() = 0;
    virtual void onClose() = 0;
    const QHash<QString, QVariant> &getChangedValues() const {return changedValues;}
signals:
    void showBusyState(bool);
    void showMessage(const QString &msg, int type=1);
protected:
    QHash<QString, QVariant> changedValues;
};
#endif // SETTINGPAGE_H
