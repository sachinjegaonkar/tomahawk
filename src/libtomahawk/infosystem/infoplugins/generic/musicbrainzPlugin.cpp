/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "musicbrainzPlugin.h"

#include <QNetworkReply>
#include <QDomDocument>

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

using namespace Tomahawk::InfoSystem;


MusicBrainzPlugin::MusicBrainzPlugin()
    : InfoPlugin()
{
    qDebug() << Q_FUNC_INFO;
    m_supportedGetTypes << Tomahawk::InfoSystem::InfoArtistReleases << Tomahawk::InfoSystem::InfoAlbumSongs;
}


MusicBrainzPlugin::~MusicBrainzPlugin()
{
    qDebug() << Q_FUNC_INFO;
}


void
MusicBrainzPlugin::namChangedSlot( QNetworkAccessManager *nam )
{
    if ( !nam )
        return;

    m_nam = QWeakPointer< QNetworkAccessManager >( nam );
}


void
MusicBrainzPlugin::getInfo( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoCriteriaHash >() )
    {
        emit info( requestId, requestData, QVariant() );
        return;
    }
    InfoCriteriaHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();
    if ( !hash.contains( "artist" ) )
    {
        emit info( requestId, requestData, QVariant() );
        return;
    }

    switch ( requestData.type )
    {
        case InfoArtistReleases:
        {
            QString requestString( "http://musicbrainz.org/ws/2/artist" );
            QUrl url( requestString );
            url.addQueryItem( "query", hash["artist"] );
            QNetworkReply* reply = m_nam.data()->get( QNetworkRequest( url ) );
            reply->setProperty( "requestId", requestId );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( artistSearchSlot() ) );
            break;
        }

        case InfoAlbumSongs:
        {
            QString requestString( "http://musicbrainz.org/ws/2/artist" );
            QUrl url( requestString );
            url.addQueryItem( "query", hash["artist"] );
            QNetworkReply* reply = m_nam.data()->get( QNetworkRequest( url ) );
            reply->setProperty( "requestId", requestId );
            reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

            connect( reply, SIGNAL( finished() ), SLOT( albumSearchSlot() ) );
            break;
        }

        default:
        {
            Q_ASSERT( false );
            break;
        }
    }
}


bool
MusicBrainzPlugin::isValidTrackData( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( requestData.input.isNull() || !requestData.input.isValid() || !requestData.input.canConvert< QVariantMap >() )
    {
        emit info( requestId, requestData, QVariant() );
        qDebug() << Q_FUNC_INFO << "Data null, invalid, or can't convert";
        return false;
    }
    QVariantMap hash = requestData.input.value< QVariantMap >();
    if ( hash[ "trackName" ].toString().isEmpty() )
    {
        emit info( requestId, requestData, QVariant() );
        qDebug() << Q_FUNC_INFO << "Track name is empty";
        return false;
    }
    if ( hash[ "artistName" ].toString().isEmpty() )
    {
        emit info( requestId, requestData, QVariant() );
        qDebug() << Q_FUNC_INFO << "No artist name found";
        return false;
    }
    return true;
}


void
MusicBrainzPlugin::artistSearchSlot()
{
    QNetworkReply* oldReply = qobject_cast<QNetworkReply*>( sender() );
    if ( !oldReply )
        return; //timeout will handle it

    QDomDocument doc;
    doc.setContent( oldReply->readAll() );
    QDomNodeList domNodeList = doc.elementsByTagName( "artist" );
    if ( domNodeList.isEmpty() )
    {
        emit info( oldReply->property( "requestId" ).toUInt(), oldReply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
        return;
    }

    QString artist_id = domNodeList.at( 0 ).toElement().attribute( "id" );
    QString requestString( "http://musicbrainz.org/ws/2/release?status=official&type=album|ep" );
    QUrl url( requestString );
    url.addQueryItem( "artist", artist_id );

    QNetworkReply* newReply = m_nam.data()->get( QNetworkRequest( url ) );
    newReply->setProperty( "requestId", oldReply->property( "requestId" ) );
    newReply->setProperty( "requestData", oldReply->property( "requestData" ) );
    connect( newReply, SIGNAL( finished() ), SLOT( albumFoundSlot() ) );
}


void
MusicBrainzPlugin::albumSearchSlot()
{
    QNetworkReply* oldReply = qobject_cast<QNetworkReply*>( sender() );
    if ( !oldReply )
        return; //timeout will handle it

    QDomDocument doc;
    doc.setContent( oldReply->readAll() );
    QDomNodeList domNodeList = doc.elementsByTagName( "artist" );
    if ( domNodeList.isEmpty() )
    {
        emit info( oldReply->property( "requestId" ).toUInt(), oldReply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
        return;
    }

    QString artist_id = domNodeList.at( 0 ).toElement().attribute( "id" );
    QString requestString( "http://musicbrainz.org/ws/2/release?status=official&type=album|ep" );
    QUrl url( requestString );
    url.addQueryItem( "artist", artist_id );

    QNetworkReply* newReply = m_nam.data()->get( QNetworkRequest( url ) );
    newReply->setProperty( "requestId", oldReply->property( "requestId" ) );
    newReply->setProperty( "requestData", oldReply->property( "requestData" ) );
    connect( newReply, SIGNAL( finished() ), SLOT( tracksSearchSlot() ) );
}


void
MusicBrainzPlugin::tracksSearchSlot()
{
    QNetworkReply* oldReply = qobject_cast<QNetworkReply*>( sender() );
    if ( !oldReply )
        return; //timeout will handle it

    QDomDocument doc;
    doc.setContent( oldReply->readAll() );
    QDomNodeList domNodeList = doc.elementsByTagName( "release" );
    if ( domNodeList.isEmpty() )
    {
        emit info( oldReply->property( "requestId" ).toUInt(), oldReply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
        return;
    }

    Tomahawk::InfoSystem::InfoRequestData requestData = oldReply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    InfoCriteriaHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash >();

    QDomElement element;
    for ( int i = 0; i < domNodeList.count(); i++ )
    {
        QDomNodeList albumNodeList = domNodeList.at( i ).toElement().elementsByTagName( "title" );
        if ( albumNodeList.at( 0 ).toElement().text() == hash["album"] )
            element = domNodeList.at( i ).toElement();
    }

    if ( element.isNull() )
    {
        emit info( oldReply->property( "requestId" ).toUInt(), oldReply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
        return;
    }

    QString release_id = element.attribute( "id" );
    QString requestString = QString( "http://musicbrainz.org/ws/2/release/%1?inc=recordings" ).arg( release_id );
    QUrl url( requestString );

    QNetworkReply* newReply = m_nam.data()->get( QNetworkRequest( url ) );
    newReply->setProperty( "requestId", oldReply->property( "requestId" ) );
    newReply->setProperty( "requestData", oldReply->property( "requestData" ) );
    connect( newReply, SIGNAL( finished() ), SLOT( tracksFoundSlot() ) );
}


void
MusicBrainzPlugin::albumFoundSlot()
{
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    if ( !reply )
        return; //timeout will handle it

    QDomDocument doc;
    doc.setContent( reply->readAll() );
    QDomNodeList domNodeList = doc.elementsByTagName( "title" );
    if ( domNodeList.isEmpty() )
    {
        emit info( reply->property( "requestId" ).toUInt(), reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
        return;
    }

    QStringList albums;
    for ( int i = 0; i < domNodeList.count(); i++ )
    {
        QString album = domNodeList.at( i ).toElement().text();
        if ( !albums.contains( album ) )
            albums << album;
    }

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    QVariantMap returnedData;
    returnedData["albums"] = albums;
    emit info( reply->property( "requestId" ).toUInt(), requestData, returnedData );

    Tomahawk::InfoSystem::InfoCriteriaHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash>();
    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = origData["artist"];
    emit updateCache( criteria, 0, requestData.type, returnedData );
}


void
MusicBrainzPlugin::tracksFoundSlot()
{
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    if ( !reply )
        return; //timeout will handle it

    QDomDocument doc;
    doc.setContent( reply->readAll() );
    QDomNodeList domNodeList = doc.elementsByTagName( "recording" );
    if ( domNodeList.isEmpty() )
    {
        emit info( reply->property( "requestId" ).toUInt(), reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >(), QVariant() );
        return;
    }

    QStringList tracks;
    for ( int i = 0; i < domNodeList.count(); i++ )
    {
        QDomNodeList trackNodeList = domNodeList.at( i ).toElement().elementsByTagName( "title" );

        for ( int j = 0; j < trackNodeList.count(); j++ )
        {
            QString track = trackNodeList.at( j ).toElement().text();
            if ( !tracks.contains( track ) )
                tracks << track;
        }
    }

    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    QVariantMap returnedData;
    returnedData["tracks"] = tracks;
    emit info( reply->property( "requestId" ).toUInt(), requestData, returnedData );

    Tomahawk::InfoSystem::InfoCriteriaHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoCriteriaHash>();
    Tomahawk::InfoSystem::InfoCriteriaHash criteria;
    criteria["artist"] = origData["artist"];
    criteria["album"] = origData["album"];
    emit updateCache( criteria, 0, requestData.type, returnedData );
}
