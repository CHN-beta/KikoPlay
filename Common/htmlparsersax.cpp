#include "htmlparsersax.h"

void HTMLParserSax::parseNode()
{
    begin++;
    if(begin==end)return;
    if(*begin=='!')
    {
        parseComment();
		nodeName.clear();
		propertyMap.clear();
        return;
    }
    QString::const_iterator iter=begin;
    nodeName.clear();
    isStart=!(*iter=='/');
    if(!isStart)iter++;
    while (iter!=end && iter->isLetterOrNumber()) nodeName+=*iter++;
    propertyMap.clear();
    while(iter!=end && *iter!='>')
    {
        while(iter!=end && iter->isSpace())iter++;
        if(*iter=='/')
        {
            while(iter!=end && *iter!='>')iter++;
            break;
        }
        else if(*iter!='>')
        {
            QString propertyName,propertyVal;
            while (iter!=end && *iter!='=' && *iter!='>') propertyName+=*iter++;
            if(iter!=end && *iter!='>')
            {
                iter++;
                QChar q(*iter++);
                while (iter!=end && *iter!=q) propertyVal+=*iter++;
                propertyMap.insert(propertyName,propertyVal);
                if(iter!=end) iter++;
            }
        }
    }
    if(iter!=end && *iter=='>')iter++;
    begin=iter;
}

void HTMLParserSax::parseComment()
{
    int commentState=0;
    while(begin!=end)
    {
        if(*begin=='-')
        {
            if(commentState==0)commentState=1;
            else if(commentState==1)commentState=2;
        }
        else if(*begin=='>' && commentState==2)
        {
            begin++;
            break;
        }
        else
        {
            commentState=0;
        }
        begin++;
    }
}

void HTMLParserSax::skip()
{
    while(begin!=end)
    {
        if(*begin!='<')begin++;
        else return;
    }
}

HTMLParserSax::HTMLParserSax(const QString &content):cHtml(content), isStart(false)
{
    begin=content.cbegin();
    end=content.cend();
}

void HTMLParserSax::seekTo(int pos)
{
    pos = qBound<int>(0, pos, end-begin);
    begin=cHtml.begin()+pos;
    nodeName="";
    isStart=false;
    propertyMap.clear();
}

void HTMLParserSax::readNext()
{
    if(begin==end)return;
    if(begin->isSpace())
        while(begin!=end && begin->isSpace())begin++;
    if(*begin=='<')
    {
        parseNode();
    }
    else
    {
        skip();
        isStart=false;
        nodeName.clear();
        propertyMap.clear();
    }
}

const QString HTMLParserSax::readContentText()
{
    int contentState=0;
    QString text;
    while(begin!=end)
    {
        if(*begin=='<')
        {
            if(contentState==0)contentState=1;
        }
        else if(*begin=='/' && contentState==1)
        {
            begin-=1;
            break;
        }
        else
        {
            if(contentState==1)
                text+='<';
            text+=*begin;
        }
        begin++;
    }
    return text;
}

const QString HTMLParserSax::readContentUntil(const QString &node, bool isStart)
{
    int startPos = curPos();
    while(begin!=end)
    {
        readNext();
        if(nodeName==node && this->isStart==isStart) break;
    }
    return cHtml.mid(startPos,curPos()-startPos);
}
