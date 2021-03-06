#include "musictttextdownloadthread.h"
#///QJson import
#include "qjson/parser.h"

MusicTTTextDownLoadThread::MusicTTTextDownLoadThread(const QString &url, const QString &save,
                                                     Download_Type type, QObject *parent)
    : MusicDownLoadThreadAbstract(url, save, type, parent)
{

}

QString MusicTTTextDownLoadThread::getClassName()
{
    return staticMetaObject.className();
}

void MusicTTTextDownLoadThread::startToDownload()
{
    if( !m_file->exists() || m_file->size() < 4 )
    {
        if( m_file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) )
        {
            m_timer.start(MT_S2MS);
            m_manager = new QNetworkAccessManager(this);

            QNetworkRequest request;
            request.setUrl(m_url);
#ifndef QT_NO_SSL
            connect(m_manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
                               SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
            M_LOGGER_INFO(QString("%1 Support ssl: %2").arg(getClassName()).arg(QSslSocket::supportsSsl()));

            QSslConfiguration sslConfig = request.sslConfiguration();
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            request.setSslConfiguration(sslConfig);
#endif
            m_reply = m_manager->get( request );
            connect(m_reply, SIGNAL(finished()), SLOT(downLoadFinished()));
            connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)),
                             SLOT(replyError(QNetworkReply::NetworkError)) );
            connect(m_reply, SIGNAL(downloadProgress(qint64, qint64)),
                             SLOT(downloadProgress(qint64, qint64)));
        }
        else
        {
            emit downLoadDataChanged("The ttpod text file create failed");
            M_LOGGER_ERROR("The ttpod text file create failed!");
            deleteAll();
        }
    }
}

void MusicTTTextDownLoadThread::downLoadFinished()
{
    if(m_reply == nullptr)
    {
        deleteAll();
        return;
    }
    m_timer.stop();

    if(m_reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = m_reply->readAll();
        if(!bytes.contains("\"code\":2"))
        {
            QJson::Parser parser;
            bool ok;
            QVariant data = parser.parse(bytes, &ok);
            if(ok)
            {
                QVariantMap value = data.toMap();
                value = value["data"].toMap();
                if(!value.isEmpty())
                {
                    if(!value["lrc"].toString().isEmpty())
                    {
                        QTextStream outstream(m_file);
                        outstream.setCodec("utf-8");
                        outstream << value["lrc"].toString().remove("\r").toUtf8() << endl;
                        m_file->close();
                        M_LOGGER_INFO("ttpod text download  has finished!");
                    }
                }
            }
        }
        else
        {
            M_LOGGER_ERROR("ttpod text download file error!");
            m_file->remove();
            m_file->close();
        }
    }

    emit downLoadDataChanged("Lrc");
    deleteAll();
}
