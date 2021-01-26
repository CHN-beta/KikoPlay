#include "animefilterproxymodel.h"
#include "animelibrary.h"
#include "animemodel.h"
#include "labelmodel.h"
#include "globalobjects.h"

AnimeFilterProxyModel::AnimeFilterProxyModel(QObject *parent):QSortFilterProxyModel(parent),filterType(0)
{
    QObject::connect(GlobalObjects::library->animeModel, &AnimeModel::animeCountInfo,
                     this, &AnimeFilterProxyModel::refreshAnimeCount);
}

void AnimeFilterProxyModel::setFilter(int type, const QString &str)
{
    filterType=type;
    setFilterRegExp(str);
    GlobalObjects::library->animeModel->showStatisMessage();
}
void AnimeFilterProxyModel::setTags(const QStringList &tagList)
{
    tagFilterList=tagList;
    invalidateFilter();
    GlobalObjects::library->animeModel->showStatisMessage();
}

void AnimeFilterProxyModel::setTime(const QSet<QString> &timeSet)
{
    timeFilterSet=timeSet;
    invalidateFilter();
    GlobalObjects::library->animeModel->showStatisMessage();
}

void AnimeFilterProxyModel::refreshAnimeCount(int cur, int total)
{
    if(cur<total)
    {
        emit animeMessage(tr("Current: %1/%2 Loaded: %2/%3").arg(rowCount()).arg(cur).arg(total),
                          PopMessageFlag::PM_INFO, true);
    }
    else
    {
        emit animeMessage(tr("Current: %1/%2").arg(rowCount()).arg(cur),
                          PopMessageFlag::PM_OK, false);
    }
}

bool AnimeFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
     QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
     Anime *anime=GlobalObjects::library->animeModel->getAnime(index,false);
     QString animeTime(anime->airDate.left(7));
     if(!timeFilterSet.isEmpty() && !timeFilterSet.contains(animeTime.isEmpty()?tr("Unknown"):animeTime))return false;
     for(const QString &tag:tagFilterList)
     {
         if(!GlobalObjects::library->labelModel->getTags()[tag].contains(anime->name))
             return false;
     }
     switch (filterType)
     {
     case 0://title
         return anime->name.contains(filterRegExp());
     case 1://summary
         return anime->desc.contains(filterRegExp());
     case 2://staff
     {
         for(auto iter=anime->staff.cbegin();iter!=anime->staff.cend();++iter)
         {
             if((*iter).second.contains(filterRegExp()))
                 return true;
         }
         return false;
     }
     case 3://crt
     {
         for(auto iter=anime->characters.cbegin();iter!=anime->characters.cend();++iter)
         {
             //if((*iter).name.contains(filterRegExp()) || (*iter).name_cn.contains(filterRegExp())
             if((*iter).name.contains(filterRegExp())
                     || (*iter).actor.contains(filterRegExp()))
                 return true;
         }
         return false;
     }
     }
     return true;
}
