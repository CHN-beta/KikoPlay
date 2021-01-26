#include "animeworker.h"
#include "globalobjects.h"
#include "Common/network.h"
#include "Common/htmlparsersax.h"
#include "Service/bangumi.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
void AnimeWorker::addAnimeInfo(const QString &animeName,const QString &epName, const QString &path)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
    query.prepare("select Name from eps where LocalFile=?");
    query.bindValue(0,path);
    query.exec();
    if(query.first())
    {
        QString ep(query.value(0).toString());
        if(ep==epName) return;
        query.prepare("update eps set Name=? where LocalFile=?");
        query.bindValue(0,epName);
        query.bindValue(1,path);
        query.exec();
        Anime *anime=animesMap.value(animeName,nullptr);
        if(anime) anime->epList.clear();
        return;
    }
    std::function<void (const QString &,const QString &,const QString &)> insertEpInfo
            = [](const QString &animeName,const QString &epName,const QString &path)
    {
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("insert into eps(Anime,Name,LocalFile) values(?,?,?)");
        query.bindValue(0,animeName);
        query.bindValue(1,epName);
        query.bindValue(2,path);
        query.exec();
    };
    Anime *anime=animesMap.value(animeName,nullptr);
    if(anime)
    {
        insertEpInfo(animeName,epName,path);
        anime->epList.clear();
    }
    else
    {
        QString aliasOf(isAlias(animeName));
        if(aliasOf.isEmpty())
        {
            Anime *anime=new Anime;
            anime->id="";
            anime->name=animeName;
            anime->epCount=0;
            anime->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
            QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
            query.prepare("insert into anime(Anime,AddTime,BangumiID) values(?,?,?)");
            query.bindValue(0,anime->name);
            query.bindValue(1,anime->addTime);
            query.bindValue(2,anime->id);
            query.exec();
            insertEpInfo(animeName,epName,path);
            animesMap.insert(animeName,anime);
            emit addAnime(anime);
        }
        else
        {
            insertEpInfo(aliasOf,epName,path);
            Anime *anime=animesMap.value(aliasOf,nullptr);
            if(anime) //The anime has been loaded
            {
                anime->epList.clear();
            }
        }
    }

}

void AnimeWorker::addAnimeInfo(const QString &animeName, int bgmId)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("select * from anime where Anime=?");
    query.bindValue(0,animeName);
    query.exec();
    if(query.first()) return;
    Q_ASSERT(!animesMap.contains(animeName));
    Anime *anime=new Anime;
    anime->id= bgmId;
    anime->name=animeName;
    anime->epCount=0;
    anime->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
    query.prepare("insert into anime(Anime,AddTime,BangumiID) values(?,?,?)");
    query.bindValue(0,anime->name);
    query.bindValue(1,anime->addTime);
    query.bindValue(2,anime->id);
    query.exec();
    animesMap.insert(animeName,anime);
    emit addAnime(anime);
}

void AnimeWorker::downloadDetailInfo(Anime *anime, int bangumiId)
{
    QString animeUrl(QString("https://api.bgm.tv/subject/%1").arg(bangumiId));
    QUrlQuery animeQuery;
    QString errInfo;
    animeQuery.addQueryItem("responseGroup","medium");
    try
    {
        emit downloadDetailMessage(tr("Downloading General Info..."));
        QString str(Network::httpGet(animeUrl,animeQuery,QStringList()<<"Accept"<<"application/json"));
        QJsonDocument document(Network::toJson(str));
        if(!document.isObject())
        {
            throw Network::NetworkError(tr("Json Format Error"));
        }
        QJsonObject obj = document.object();
        if(obj.value("type").toInt()!=2)
        {
            throw Network::NetworkError(tr("Json Format Error"));
        }
        anime->id=bangumiId;
        QString newTitle(obj.value("name_cn").toString());
        if(newTitle.isEmpty())newTitle=obj.value("name").toString();
        newTitle.replace("&amp;","&");
        if(newTitle!=anime->name)
        {
            animesMap.remove(anime->name);
            setAlias(newTitle,anime->name);
            QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
            if(animesMap.contains(newTitle))
            {
                query.prepare("update eps set Anime=? where Anime=?");
                query.bindValue(0,newTitle);
                query.bindValue(1,anime->name);
                query.exec();
                animesMap[newTitle]->epList.clear();
                emit mergeAnime(anime,animesMap[newTitle]);
                return;
            }
            query.prepare("update anime set Anime=? where Anime=?");
            query.bindValue(0,newTitle);
            query.bindValue(1,anime->name);
            query.exec();
            anime->name=newTitle;
            animesMap.insert(newTitle,anime);
        }
        anime->desc=obj.value("summary").toString();
        anime->airDate=obj.value("air_date").toString();
        anime->epCount=obj.value("eps_count").toInt();
        int quality = GlobalObjects::appSetting->value("Library/CoverQuality", 1).toInt() % 3;
        const char *qualityTypes[] = {"images/medium","images/common","images/large"};
        QByteArray cover(Network::httpGet(Network::getValue(obj,qualityTypes[quality]).toString(),QUrlQuery()));
        anime->coverPixmap.loadFromData(cover);

        QJsonArray staffArray(obj.value("staff").toArray());
        anime->staff.clear();
        for(auto staffIter=staffArray.begin();staffIter!=staffArray.end();++staffIter)
        {
            QJsonObject staffObj=(*staffIter).toObject();
            QJsonArray jobArray(staffObj.value("jobs").toArray());
            for(auto jobIter=jobArray.begin();jobIter!=jobArray.end();++jobIter)
            {
                bool contains=false;
                QString job((*jobIter).toString());
                for(auto iter=anime->staff.begin();iter!=anime->staff.end();++iter)
                {
                    if((*iter).first==job)
                    {
                        (*iter).second+=" "+staffObj.value("name").toString();
                        contains=true;
                        break;
                    }
                }
                if(!contains)
                {
                    anime->staff.append(QPair<QString,QString>(job,staffObj.value("name").toString()));
                }
            }
        }
        anime->characters.clear();
        QJsonArray crtArray(obj.value("crt").toArray());
        emit downloadDetailMessage(tr("Downloading Character Info..."));

        for(auto crtIter=crtArray.begin();crtIter!=crtArray.end();++crtIter)
        {
            QJsonObject crtObj=(*crtIter).toObject();
            Character crt;
            crt.name=crtObj.value("name").toString();
            //crt.name_cn=crtObj.value("name_cn").toString();
            crt.id=crtObj.value("id").toInt();
            crt.actor=crtObj.value("actors").toArray().first().toObject().value("name").toString();
            QString imgUrl(Network::getValue(crtObj, "images/grid").toString());
            try
            {
                if(!imgUrl.isEmpty())
                {
                    crt.imgURL=imgUrl;
                    //crt.image=Network::httpGet(imgUrl,QUrlQuery());
                }
            } catch (Network::NetworkError &error) {
                emit downloadDetailMessage(tr("Downloading Character Info...%1 Failed: %2").arg(crt.name, error.errorInfo));
                                           //.arg(crt.name_cn.isEmpty()?crt.name:crt.name_cn, error.errorInfo));
            }
            anime->characters.append(crt);
        }
        updateAnimeInfo(anime);                      
        if(GlobalObjects::appSetting->value("Library/DownloadTag",Qt::Checked).toInt()==Qt::Checked)
        {
            emit downloadDetailMessage(tr("Downloading Label Info..."));
            downloadLabelInfo(anime);
        }
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
        emit downloadDetailMessage(tr("Downloading General Info Failed: %1").arg(errInfo));
    }
    emit downloadDone(errInfo);
}

void AnimeWorker::downloadTags(int bangumiID, QStringList &tags)
{
    QString errInfo(Bangumi::getTags(bangumiID, tags));
    emit downloadTagDone(errInfo);
}

void AnimeWorker::saveTags(const QString &title, const QStringList &tags)
{
    QSqlDatabase db=QSqlDatabase::database("Bangumi_W");
    db.transaction();
    QSqlQuery query(db);
    query.prepare("insert into tag(Anime,Tag) values(?,?)");
    query.bindValue(0,title);
    for(const QString &tag:tags)
    {
        query.bindValue(1,tag);
        query.exec();
    }
    db.commit();
}

void AnimeWorker::loadAnimes(QList<Anime *> *animes,int offset,int limit)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.exec(QString("select * from anime order by AddTime desc limit %1 offset %2").arg(limit).arg(offset));
    int animeNo=query.record().indexOf("Anime"),
        timeNo=query.record().indexOf("AddTime"),
        epCountNo=query.record().indexOf("EpCount"),
        summaryNo=query.record().indexOf("Summary"),
        dateNo=query.record().indexOf("Date"),
        staffNo=query.record().indexOf("Staff"),
        bangumiIdNo=query.record().indexOf("BangumiID"),
        coverNo=query.record().indexOf("Cover");
    int count=0;
    while (query.next())
    {
        Anime *anime=new Anime;
        anime->name=query.value(animeNo).toString();
        anime->desc=query.value(summaryNo).toString();
        anime->airDate=query.value(dateNo).toString();
        anime->addTime=query.value(timeNo).toLongLong();
        anime->epCount=query.value(epCountNo).toInt();
        //anime->loadCrtImage=false;
        QStringList staffs(query.value(staffNo).toString().split(';',QString::SkipEmptyParts));
        for(int i=0;i<staffs.count();++i)
        {
            int pos=staffs.at(i).indexOf(':');
            anime->staff.append(QPair<QString,QString>(staffs[i].left(pos),staffs[i].mid(pos+1)));
        }
        anime->id=query.value(bangumiIdNo).toInt();
        anime->coverPixmap.loadFromData(query.value(coverNo).toByteArray());

        QSqlQuery crtQuery(QSqlDatabase::database("Bangumi_W"));
        crtQuery.prepare("select * from character where Anime=?");
        crtQuery.bindValue(0,anime->name);
        crtQuery.exec();
        int nameNo=crtQuery.record().indexOf("Name"),
            nameCNNo=crtQuery.record().indexOf("NameCN"),
            actorNo=crtQuery.record().indexOf("Actor"),
            idNo=crtQuery.record().indexOf("BangumiID");
            //imageNo=crtQuery.record().indexOf("Image");
        while (crtQuery.next())
        {
            Character crt;
            crt.name=crtQuery.value(nameNo).toString();
            //crt.name_cn=crtQuery.value(nameCNNo).toString();
            crt.id=crtQuery.value(idNo).toInt();
            crt.actor=crtQuery.value(actorNo).toString();
            //crt.image=crtQuery.value(imageNo).toByteArray();
            anime->characters.append(crt);
        }
        animes->append(anime);
        animesMap.insert(anime->name,anime);
        count++;
    }
    emit loadDone(count);
}

void AnimeWorker::deleteAnime(Anime *anime)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("delete from anime where Anime=?");
    query.bindValue(0,anime->name);
    query.exec();
    query.prepare("delete from alias where Anime=?");
    query.bindValue(0,anime->name);
    query.exec();
    animesMap.remove(anime->name);
	delete anime;
    emit deleteDone();
}

void AnimeWorker::updatePlayTime(const QString &title, const QString &path)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("update eps set LastPlayTime=? where LocalFile=?");
    QString timeStr(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    query.bindValue(0,timeStr);
    query.bindValue(1,path);
    query.exec();
    if(animesMap.contains(title))
    {
        Anime *anime=animesMap[title];
        for(auto iter=anime->epList.begin();iter!=anime->epList.end();++iter)
        {
            if((*iter).localFile==path)
            {
                (*iter).lastPlayTime=QDateTime::currentDateTime().toSecsSinceEpoch();
                break;
            }
        }

    }
}

void AnimeWorker::loadLabelInfo(QMap<QString, QSet<QString> > &tagMap, QMap<QString,int> &timeMap)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("select * from tag");
    query.exec();
    int animeNo=query.record().indexOf("Anime"),tagNo=query.record().indexOf("Tag");
    QString tagName;
    while (query.next())
    {
        tagName=query.value(tagNo).toString();
        if(!tagMap.contains(tagName))
            tagMap.insert(tagName,QSet<QString>());
        tagMap[tagName].insert(query.value(animeNo).toString());
    }

    query.prepare("select Date from anime");
    query.exec();
    int dateNo=query.record().indexOf("Date");
    QString dateStr;
    while (query.next())
    {
        dateStr=query.value(dateNo).toString().left(7);
        if(dateStr.isEmpty())continue;
        if(!timeMap.contains(dateStr))
        {
            timeMap.insert(dateStr,0);
        }
        timeMap[dateStr]++;
    }
    emit loadLabelInfoDone();
}

void AnimeWorker::updateCrtImage(const QString &title, const Character *crt)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("update character_image set Image=? where Anime=? and CBangumiID=?");
    //query.bindValue(0,crt->image);
    query.bindValue(0,QPixmap());
    query.bindValue(1,title);
    query.bindValue(2,crt->id);
    query.exec();
}

void AnimeWorker::deleteTag(const QString &tag, const QString &animeTitle)
{
    QSqlDatabase db=QSqlDatabase::database("Bangumi_W");
    QSqlQuery query(db);
    db.transaction();
    if(animeTitle.isEmpty())
    {
        query.prepare("delete from tag where Tag=?");
        query.bindValue(0,tag);
    }
    else if(tag.isEmpty())
    {
        query.prepare("delete from tag where Anime=?");
        query.bindValue(0,animeTitle);
    }
    else
    {
        query.prepare("delete from tag where Anime=? and Tag=?");
        query.bindValue(0,animeTitle);
        query.bindValue(1,tag);
    }
    query.exec();
    db.commit();
}

AnimeWorker::~AnimeWorker()
{
    qDeleteAll(animesMap);
}

void AnimeWorker::updateAnimeInfo(Anime *anime)
{

    QSqlDatabase db=QSqlDatabase::database("Bangumi_W");
    db.transaction();

    QSqlQuery query(db);
    query.prepare("update anime set Summary=?,Date=?,Staff=?,BangumiID=?,Cover=?,EpCount=? where Anime=?");
    query.bindValue(0,anime->desc);
    query.bindValue(1,anime->airDate);
    QStringList staffStrList;
    for(auto iter=anime->staff.cbegin();iter!=anime->staff.cend();++iter)
        staffStrList.append((*iter).first+":"+(*iter).second);
        //staffStrList.append(iter.key()+":"+iter.value());
    query.bindValue(2,staffStrList.join(';'));
    query.bindValue(3,anime->id);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    anime->coverPixmap.save(&buffer,"jpg");
    query.bindValue(4,bytes);
    query.bindValue(5,anime->epCount);
    query.bindValue(6,anime->name);
    query.exec();

    query.prepare("delete from character where Anime=?");
    query.bindValue(0,anime->name);
    query.exec();

    query.prepare("insert into character(Anime,Name,NameCN,Actor,BangumiID) values(?,?,?,?,?)");
    query.bindValue(0,anime->name);

    QSqlQuery imgQuery(db);
    imgQuery.prepare("insert into character_image(Anime,CBangumiID,ImageURL,Image) values(?,?,?,?)");
    imgQuery.bindValue(0,anime->name);
    for(auto iter=anime->characters.cbegin();iter!=anime->characters.cend();++iter)
    {
        query.bindValue(1,(*iter).name);
        // todo-----------------
        query.bindValue(2,(*iter).name);
        query.bindValue(3,(*iter).actor);
        query.bindValue(4,(*iter).id);
        query.exec();

        imgQuery.bindValue(1,(*iter).id);
        imgQuery.bindValue(2,(*iter).imgURL);
        //imgQuery.bindValue(3,(*iter).image);
        imgQuery.bindValue(3,QPixmap());
        imgQuery.exec();
    }
    db.commit();
   // anime->loadCrtImage=true;
}

QString AnimeWorker::downloadLabelInfo(Anime *anime)
{
    QStringList tags;
    QString err(Bangumi::getTags(anime->id.toInt(), tags));
    if(err.isEmpty())
    {
        QRegExp yearRe("(19|20)\\d{2}");
        QStringList trivialTags={"TV","OVA","WEB"};
        QStringList tagList;
        for(auto &tagName : tags)
        {
            if(yearRe.indexIn(tagName)==-1 && !trivialTags.contains(tagName)
                    && !anime->name.contains(tagName) && !tagName.contains(anime->name))
                tagList.append(tagName);
            if(tagList.count()>12)
                break;
        }
        emit newTagDownloaded(anime->name, tagList);
    }
    return err;
}

QString AnimeWorker::isAlias(const QString &animeName)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
    query.prepare("select Anime from alias where Alias=?");
    query.bindValue(0,animeName);
    query.exec();
    if(query.first())
        return query.value(0).toString();
    return "";
}

void AnimeWorker::setAlias(const QString &animeName, const QString &alias)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
    query.prepare("insert into alias(Alias,Anime) values(?,?)");
    query.bindValue(0,alias);
    query.bindValue(1,animeName);
    query.exec();
}

