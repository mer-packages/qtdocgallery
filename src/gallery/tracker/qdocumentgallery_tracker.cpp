/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDocGallery module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <tracker-sparql.h>

#include "qdocumentgallery.h"

#include "qabstractgallery_p.h"

#include "qgalleryitemrequest.h"
#include "qgalleryqueryrequest.h"
#include "qgallerytyperequest.h"

#include "qgallerytrackerchangenotifier_p.h"
#include "qgallerytrackerschema_p.h"
#include "qgallerytrackereditableresultset_p.h"
#include "qgallerytrackertyperesultset_p.h"
#include "qgallerydbusinterface_p.h"

#include <QtCore/qmetaobject.h>
#include <QtDBus/qdbusmetatype.h>
#include <QtDBus/qdbusargument.h>

#include <QtCore/qdebug.h>

QDBusArgument &operator<<(QDBusArgument &argument, const QGalleryTrackerGraphUpdate &t)
{
    argument.beginStructure();
    argument << t.graph << t.subject << t.predicate << t.object;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QGalleryTrackerGraphUpdate &t)
{
    argument.beginStructure();
    argument >> t.graph >> t.subject >> t.predicate >> t.object;
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(QVector<QStringList>)
Q_DECLARE_METATYPE(QGalleryTrackerGraphUpdate)
Q_DECLARE_METATYPE(QVector<QGalleryTrackerGraphUpdate>)

QT_BEGIN_NAMESPACE_DOCGALLERY

class QDocumentGalleryPrivate : public QAbstractGalleryPrivate, public QGalleryDBusInterfaceFactory
{
public:
    QDocumentGalleryPrivate()
        : connection(0)
    {
    }

    QGalleryAbstractResponse *createItemResponse(QGalleryItemRequest *request);
    QGalleryAbstractResponse *createTypeResponse(QGalleryTypeRequest *request);
    QGalleryAbstractResponse *createFilterResponse(QGalleryQueryRequest *request);

    QGalleryDBusInterfacePointer metaDataInterface();
    QGalleryDBusInterfacePointer statisticsInterface();

    QGalleryTrackerChangeNotifier *getChangeNotifier( const QString &type );
    QGalleryTrackerChangeNotifier *createChangeNotifier(
            QScopedPointer<QGalleryTrackerChangeNotifier> &notifier, const QString &serviceId);
    QGalleryTrackerChangeNotifier *fileChangeNotifier();
    QGalleryTrackerChangeNotifier *audioChangeNotifier();
    QGalleryTrackerChangeNotifier *artistChangeNotifier();
    QGalleryTrackerChangeNotifier *documentChangeNotifier();
    QGalleryTrackerChangeNotifier *videoChangeNotifier();
    QGalleryTrackerChangeNotifier *playlistChangeNotifier();
    QGalleryTrackerChangeNotifier *musicAlbumChangeNotifier();
    QGalleryTrackerChangeNotifier *imageListChangeNotifier();
    QGalleryTrackerChangeNotifier *imageChangeNotifier();

    QGalleryAbstractResponse *createItemListResponse(
            QGalleryTrackerResultSetArguments *arguments,
            bool autoUpdate,
            QGalleryTrackerChangeNotifier* notifier);

    TrackerSparqlConnection *connection;
    QGalleryDBusInterfacePointer metaDataService;
    QGalleryDBusInterfacePointer statisticsService;
    QScopedPointer<QGalleryTrackerChangeNotifier> fileNotifier;
    QScopedPointer<QGalleryTrackerChangeNotifier> audioNotifier;
    QScopedPointer<QGalleryTrackerChangeNotifier> artistNotifier;
    QScopedPointer<QGalleryTrackerChangeNotifier> documentNotifier;
    QScopedPointer<QGalleryTrackerChangeNotifier> videoNotifier;
    QScopedPointer<QGalleryTrackerChangeNotifier> playlistNotifier;
    QScopedPointer<QGalleryTrackerChangeNotifier> musicAlbumNotifier;
    QScopedPointer<QGalleryTrackerChangeNotifier> imageListNotifier;
    QScopedPointer<QGalleryTrackerChangeNotifier> imageNotifier;
};

QGalleryDBusInterfacePointer QDocumentGalleryPrivate::metaDataInterface()
{
    if (!metaDataService) {
        metaDataService = new QGalleryDBusInterface(
                QLatin1String("org.freedesktop.Tracker1"),
                QLatin1String("/org/freedesktop/Tracker1/Resources"),
                "org.freedesktop.Tracker1.Resources");
    }
    return metaDataService;
}

QGalleryDBusInterfacePointer QDocumentGalleryPrivate::statisticsInterface()
{
    if (!statisticsService) {
        statisticsService = new QGalleryDBusInterface(
                QLatin1String("org.freedesktop.Tracker1"),
                QLatin1String("/org/freedesktop/Tracker1/Statistics"),
                "org.freedesktop.Tracker1.Statistics");
    }
    return statisticsService;
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::createChangeNotifier(
        QScopedPointer<QGalleryTrackerChangeNotifier> &notifier,  const QString &serviceId)
{
    if (!notifier)
        notifier.reset(new QGalleryTrackerChangeNotifier(serviceId, metaDataInterface()));
    return notifier.data();
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::fileChangeNotifier()
{
    return createChangeNotifier(
            fileNotifier, QGalleryTrackerSchema::serviceForType(QDocumentGallery::File));
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::audioChangeNotifier()
{
    return createChangeNotifier(
            audioNotifier, QGalleryTrackerSchema::serviceForType(QDocumentGallery::Audio));
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::artistChangeNotifier()
{
    return createChangeNotifier(
            artistNotifier, QGalleryTrackerSchema::serviceForType(QDocumentGallery::Artist));
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::documentChangeNotifier()
{
    return createChangeNotifier(
            documentNotifier, QGalleryTrackerSchema::serviceForType(QDocumentGallery::Document));
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::videoChangeNotifier()
{
    return createChangeNotifier(
            videoNotifier, QGalleryTrackerSchema::serviceForType(QDocumentGallery::Video));
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::playlistChangeNotifier()
{
    return createChangeNotifier(
            playlistNotifier, QGalleryTrackerSchema::serviceForType(QDocumentGallery::Playlist));
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::musicAlbumChangeNotifier()
{
    return createChangeNotifier(
            musicAlbumNotifier, QGalleryTrackerSchema::serviceForType(QDocumentGallery::Album));
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::imageListChangeNotifier()
{
    return createChangeNotifier(
            imageListNotifier, QGalleryTrackerSchema::serviceForType(QDocumentGallery::PhotoAlbum));
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::imageChangeNotifier()
{
    return createChangeNotifier(
            imageNotifier, QGalleryTrackerSchema::serviceForType(QDocumentGallery::Image));
}

QGalleryAbstractResponse *QDocumentGalleryPrivate::createItemResponse(QGalleryItemRequest *request)
{
    QGalleryTrackerSchema schema = QGalleryTrackerSchema::fromItemId(request->itemId().toString());

    QGalleryTrackerResultSetArguments arguments;

    int error = schema.prepareItemResponse(
            &arguments, this, request->itemId().toString(), request->propertyNames());

    if (error != QDocumentGallery::NoError) {
        return new QGalleryAbstractResponse(error);
    } else {
        return createItemListResponse(
                &arguments,
                request->autoUpdate(),
                getChangeNotifier(schema.itemType()));
    }
}

QGalleryTrackerChangeNotifier *QDocumentGalleryPrivate::getChangeNotifier( const QString &itemType )
{
    QGalleryTrackerChangeNotifier * notifier = 0;

    if (itemType == QDocumentGallery::File.name()) {
        notifier = fileChangeNotifier();
    } else if (itemType == QDocumentGallery::Audio.name())
        notifier = audioChangeNotifier();
    else if (itemType == QDocumentGallery::Artist.name())
        notifier = artistChangeNotifier();
    else if (itemType == QDocumentGallery::Document.name())
        notifier = documentChangeNotifier();
    else if (itemType == QDocumentGallery::Video.name())
        notifier = videoChangeNotifier();
    else if (itemType == QDocumentGallery::Playlist.name())
        notifier = playlistChangeNotifier();
    else if (itemType == QDocumentGallery::Album.name())
        notifier = musicAlbumChangeNotifier();
    else if (itemType == QDocumentGallery::PhotoAlbum.name())
        notifier = imageListChangeNotifier();
    else if (itemType == QDocumentGallery::Image.name())
        notifier = imageChangeNotifier();

    return notifier;
}

QGalleryAbstractResponse *QDocumentGalleryPrivate::createTypeResponse(QGalleryTypeRequest *request)
{
    QGalleryTrackerSchema schema(request->itemType());

    QGalleryTrackerTypeResultSetArguments arguments;

    int error = schema.prepareTypeResponse(&arguments, this);

    if (error != QDocumentGallery::NoError) {
        return new QGalleryAbstractResponse(error);
    } else {
        QGalleryTrackerTypeResultSet *response = new QGalleryTrackerTypeResultSet(arguments);

        if (request->autoUpdate()) {
            QGalleryTrackerChangeNotifier * notifier = getChangeNotifier( request->itemType() );
            if ( notifier )
                QObject::connect(
                        notifier, SIGNAL(itemsChanged(int)),
                        response, SLOT(refresh(int)));
        }

        return response;
    }
}

QGalleryAbstractResponse *QDocumentGalleryPrivate::createItemListResponse(
        QGalleryTrackerResultSetArguments *arguments,
        bool autoUpdate,
        QGalleryTrackerChangeNotifier* notifier)
{
    if (!connection)
        return new QGalleryAbstractResponse(QDocumentGallery::ConnectionError);

    QGalleryTrackerResultSet *response = new QGalleryTrackerEditableResultSet(
            connection, arguments, metaDataInterface(), autoUpdate);

    if (autoUpdate) {
        if (notifier) {
            QObject::connect(
                notifier, SIGNAL(itemsChanged(int)), response, SLOT(refresh(int)));
        }
    }
    if (notifier) {
        QObject::connect(
            response, SIGNAL(itemEdited(QString)), notifier, SLOT(itemsEdited(QString)));
    }

    return response;
}

QGalleryAbstractResponse *QDocumentGalleryPrivate::createFilterResponse(
        QGalleryQueryRequest *request)
{
    QGalleryTrackerSchema schema(request->rootType());

    QGalleryTrackerResultSetArguments arguments;

    int error = schema.prepareQueryResponse(
            &arguments,
            this,
            request->scope(),
            request->rootItem().toString(),
            request->filter(),
            request->propertyNames(),
            request->sortPropertyNames(),
            request->offset(),
            request->limit());

    if (error != QDocumentGallery::NoError) {
        return new QGalleryAbstractResponse(error);
    } else {
        return createItemListResponse(
                &arguments,
                request->autoUpdate(),
                getChangeNotifier(request->rootType()) );
    }
}

QDocumentGallery::QDocumentGallery(QObject *parent)
    : QAbstractGallery(*new QDocumentGalleryPrivate, parent)
{
    Q_D(QDocumentGallery);

    qDBusRegisterMetaType<QVector<QStringList> >();
    qDBusRegisterMetaType<QGalleryTrackerGraphUpdate>();
    qDBusRegisterMetaType<QVector<QGalleryTrackerGraphUpdate> >();

    g_type_init();
    d->connection = tracker_sparql_connection_get(0, 0);
}

QDocumentGallery::~QDocumentGallery()
{
    Q_D(QDocumentGallery);
    if (d->connection)
        g_object_unref(d->connection);
}

bool QDocumentGallery::isRequestSupported(QGalleryAbstractRequest::RequestType type) const
{
    switch (type) {
    case QGalleryAbstractRequest::QueryRequest:
    case QGalleryAbstractRequest::ItemRequest:
    case QGalleryAbstractRequest::TypeRequest:
        return true;
    default:
        return false;
    }
}

QStringList QDocumentGallery::itemTypePropertyNames(const QString &itemType) const
{
    return QGalleryTrackerSchema(itemType).supportedPropertyNames();
}

QGalleryProperty::Attributes QDocumentGallery::propertyAttributes(
        const QString &propertyName, const QString &itemType) const
{
    return QGalleryTrackerSchema(itemType).propertyAttributes(propertyName);
}

QGalleryAbstractResponse *QDocumentGallery::createResponse(QGalleryAbstractRequest *request)
{
    Q_D(QDocumentGallery);

    switch (request->type()) {
    case QGalleryAbstractRequest::QueryRequest:
        return d->createFilterResponse(static_cast<QGalleryQueryRequest *>(request));
    case QGalleryAbstractRequest::ItemRequest:
        return d->createItemResponse(static_cast<QGalleryItemRequest *>(request));
    case QGalleryAbstractRequest::TypeRequest:
        return d->createTypeResponse(static_cast<QGalleryTypeRequest *>(request));
    default:
        return 0;
    }
}

QT_END_NAMESPACE_DOCGALLERY
