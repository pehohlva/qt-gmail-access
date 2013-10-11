/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmailpluginmanager.h"
#include <QMap>
#include <QPluginLoader>
#include <QDir>
#include <QtDebug>
#include <qmailnamespace.h>


/*!
  \class QMailPluginManager

  \brief The QMailPluginManager class is a helper class that simplifies plug-in loading
  for the Messaging framework.

  The list() function returns a list of available plugins in the subdirectory specified in the constructor.

  Plugin subdirectories are searched for from the directory specified by QMail::pluginPath().

  In order to load a plugin, call the instance() function with the
  name of the plugin to load.  qobject_cast() may then be used to query
  for the desired interface.
*/


/*!
  \fn QMailPluginManager::QMailPluginManager(const QString& dir, QObject* parent)

  Creates a QMailPluginManager for plugins located in the plugin subdirectory \a dir with the given
  \a parent.

  The plugins must be installed in the QMail::pluginPath()/\i{dir}  directory.
*/

/*!
  \fn QMailPluginManager::~QMailPluginManager()

  Destroys the QMailPluginManager and releases any resources allocated by
  the PluginManager.
*/

/*!
  \fn QStringList QMailPluginManager::list() const

  Returns the list of plugins that are available in the plugin subdirectory.
*/

/*!
  \fn QObject* QMailPluginManager::instance(const QString& name)

  Load the plug-in specified by \a name.

  Returns the plugin interface if found, otherwise 0.

  \code
    QObject *instance = pluginManager->instance("name");
    if (instance) {
        EffectsInterface *iface = 0;
        iface = qobject_cast<EffectsInterface*>(instance);
        if (iface) {
            // We have an instance of the desired type.
        }
    }
  \endcode
*/

namespace {

QStringList pluginFilePatterns()
{
#ifdef Q_OS_WIN
	return QStringList() << "*.dll" << "*.DLL";
#else
	return QStringList() << "*.so*";
#endif
}

}

class QMailPluginManagerPrivate
{
public:
    QMailPluginManagerPrivate(const QString& ident);

public:
    QMap<QString,QPluginLoader*> pluginMap;
    QString pluginPath;
};

QMailPluginManagerPrivate::QMailPluginManagerPrivate(const QString& path)
:
    pluginPath(QMail::pluginsPath() + path)
{
    //initialize the plugin map
    QDir dir(pluginPath.toLatin1());
    QStringList libs = dir.entryList(pluginFilePatterns(), QDir::Files);

    if(libs.isEmpty())
    {
        qWarning() << "Could not find any plugins in path " << pluginPath << "!";
        return;
    }

    foreach(const QString& libname,libs)
        pluginMap[libname] = 0;
}

QMailPluginManager::QMailPluginManager(const QString& dir, QObject* parent)
:
    QObject(parent),
    d(new QMailPluginManagerPrivate(dir))
{
}

QMailPluginManager::~QMailPluginManager()
{
    delete d; d = 0;
}

QStringList QMailPluginManager::list() const
{
    return d->pluginMap.keys();
}

QObject* QMailPluginManager::instance(const QString& name)
{
    QString libfile = d->pluginPath + "/" + name;
    if (!QFile::exists(libfile))
        return 0;

    QPluginLoader *lib = 0;
    QMap<QString,QPluginLoader*>::const_iterator it = d->pluginMap.find(name);
    if (it != d->pluginMap.end())
        lib = *it;
    if ( !lib ) {
        lib = new QPluginLoader(libfile);
        lib->load();
        if ( !lib->isLoaded() ) {
            qWarning() << "Could not load" << libfile << "errorString()" << lib->errorString();
            delete lib;
            return 0;
        }
    }
    d->pluginMap[name] = lib;
    return lib->instance();
}
