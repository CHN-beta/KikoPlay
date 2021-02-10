#ifndef LIBRARYWINDOW_H
#define LIBRARYWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QTreeView>
class QListView;
class QToolButton;
class QPushButton;
class QGraphicsBlurEffect;
class QActionGroup;
class QButtonGroup;
class QSplitter;
class AnimeModel;
class AnimeItemDelegate;
class AnimeDetailInfoPage;
class LabelTreeView : public QTreeView
{
    Q_OBJECT
    Q_PROPERTY(QColor topLevelColor READ getTopLevelColor WRITE setTopLevelColor)
    Q_PROPERTY(QColor childLevelColor READ getChildLevelColor WRITE setChildLevelColor)
    Q_PROPERTY(QColor countFColor READ getCountFColor WRITE setCountFColor)
    Q_PROPERTY(QColor countBColor READ getCountBColor WRITE setCountBColor)
public:
    using QTreeView::QTreeView;

    QColor getTopLevelColor() const {return topLevelColor;}
    void setTopLevelColor(const QColor& color)
    {
        topLevelColor =  color;
        emit topLevelColorChanged(topLevelColor);
    }
    QColor getChildLevelColor() const {return childLevelColor;}
    void setChildLevelColor(const QColor& color)
    {
        childLevelColor = color;
        emit childLevelColorChanged(childLevelColor);
    }
    QColor getCountFColor() const {return countFColor;}
    void setCountFColor(const QColor& color)
    {
        countFColor = color;
        emit countFColorChanged(countFColor);
    }
    QColor getCountBColor() const {return countBColor;}
    void setCountBColor(const QColor& color)
    {
        countBColor = color;
        emit countBColorChanged(countBColor);
    }
signals:
    void topLevelColorChanged(const QColor &color);
    void childLevelColorChanged(const QColor &color);
    void countFColorChanged(const QColor &color);
    void countBColorChanged(const QColor &color);
private:
    QColor topLevelColor, childLevelColor, countFColor, countBColor;
};
class AnimeFilterBox : public QLineEdit
{
    Q_OBJECT
public:
    explicit AnimeFilterBox(QWidget *parent = nullptr);

signals:
    void filterChanged(int type,const QString &filterStr);
private:
    QActionGroup *filterTypeGroup;
};
class LibraryWindow : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool bgMode READ getBgMode WRITE setBgMode)
public:
    explicit LibraryWindow(QWidget *parent = nullptr);
    void beforeClose();

    bool getBgMode() const {return bgOn;}
    void setBgMode(bool on) {bgOn = on;}
private:
    AnimeModel *animeModel;
    QListView *animeListView;
    LabelTreeView *labelView;
    QSplitter *splitter;
    AnimeDetailInfoPage *detailPage;
    bool bgOn;
signals:
    void playFile(const QString &file);
    void switchBackground(const QPixmap &pixmap, bool setPixmap);
public slots:
    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
};

#endif // LIBRARYWINDOW_H
