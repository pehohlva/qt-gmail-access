/*
managementserver.h: Declaration of ManagementServer class

Copyright (C) 2013  Sarab D. Mattes <mattes@nixnux.org>

This file is part of omapd.

omapd is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

omapd is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with omapd.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MANAGEMENTSERVER_H
#define MANAGEMENTSERVER_H

#include <QtCore>
#include <QTcpServer>
#include "mapgraphinterface.h"

#define MANAGEMENT_MSG_MAX_LENGTH 1024

class ManagementServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ManagementServer(MapGraphInterface *mapGraph, QObject *parent = 0);
    bool startListening();

signals:
    
private slots:
    void handleMgmtConnection();
    void readMgmtRequest();

private:
    OmapdConfig* _omapdConfig;
    MapGraphInterface* _mapGraph;
};

#endif // MANAGEMENTSERVER_H
