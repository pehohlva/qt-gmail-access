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

#include "qmailmessagethreadedmodel.h"
#include "qmailstore.h"
#include "qmailnamespace.h"
#include <QCache>
#include <QtAlgorithms>

static const int fullRefreshCutoff = 10;

class QMailMessageThreadedModelItem
{
public:
    explicit QMailMessageThreadedModelItem(const QMailMessageId& id, QMailMessageThreadedModelItem *parent = 0) : _id(id), _parent(parent) {}
    ~QMailMessageThreadedModelItem() {}

    int rowInParent() const { return _parent->_children.indexOf(*this); }

    QMailMessageIdList childrenIds() const 
    { 
        QMailMessageIdList ids; 
        foreach (const QMailMessageThreadedModelItem &item, _children) {
            ids.append(item._id);
        }
        
        return ids;
    }

    bool operator== (const QMailMessageThreadedModelItem& other) { return (_id == other._id); }

    QMailMessageId _id;
    QMailMessageThreadedModelItem *_parent;
    QList<QMailMessageThreadedModelItem> _children;
};


class QMailMessageThreadedModelPrivate : public QMailMessageModelImplementation
{
public:
    QMailMessageThreadedModelPrivate(QMailMessageThreadedModel& model, const QMailMessageKey& key, const QMailMessageSortKey& sortKey, bool sychronizeEnabled);
    ~QMailMessageThreadedModelPrivate();

    QMailMessageKey key() const;
    void setKey(const QMailMessageKey& key);

    QMailMessageSortKey sortKey() const;
    void setSortKey(const QMailMessageSortKey& sortKey);

    bool isEmpty() const;

    int rowCount(const QModelIndex& idx) const;
    int columnCount(const QModelIndex& idx) const;

    QMailMessageId idFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromId(const QMailMessageId& id) const;

    Qt::CheckState checkState(const QModelIndex &idx) const;
    void setCheckState(const QModelIndex &idx, Qt::CheckState state);

    int rootRow(const QModelIndex& idx) const;

    void reset();

    bool ignoreMailStoreUpdates() const;
    bool setIgnoreMailStoreUpdates(bool ignore);

    bool processMessagesAdded(const QMailMessageIdList &ids);
    bool processMessagesUpdated(const QMailMessageIdList &ids);
    bool processMessagesRemoved(const QMailMessageIdList &ids);

    bool addMessages(const QMailMessageIdList &ids);
    bool updateMessages(const QMailMessageIdList &ids);
    bool removeMessages(const QMailMessageIdList &ids, QMailMessageIdList *readditions);

    void insertItemAt(int row, const QModelIndex &parentIndex, const QMailMessageId &id);
    void removeItemAt(int row, const QModelIndex &parentIndex);

    QModelIndex index(int row, int column, const QModelIndex &parentIndex) const;
    QModelIndex parent(const QModelIndex &idx) const;

private:
    void init() const;

    QModelIndex index(const QMailMessageThreadedModelItem *item, int column) const;
    QModelIndex parentIndex(const QMailMessageThreadedModelItem *item) const;

    QMailMessageThreadedModelItem *itemFromIndex(const QModelIndex &index) const;
    QModelIndex indexFromItem(const QMailMessageThreadedModelItem *item) const;

    QMailMessageThreadedModel &_model;
    QMailMessageKey _key;
    QMailMessageSortKey _sortKey;
    bool _ignoreUpdates;
    mutable QMailMessageThreadedModelItem _root;
    mutable QMap<QMailMessageId, QMailMessageThreadedModelItem*> _messageItem;
    mutable QSet<QMailMessageId> _checkedIds;
    mutable QList<QMailMessageId> _currentIds;
    mutable bool _initialised;
    mutable bool _needSynchronize;
};


QMailMessageThreadedModelPrivate::QMailMessageThreadedModelPrivate(QMailMessageThreadedModel& model,
                                                                   const QMailMessageKey& key, 
                                                                   const QMailMessageSortKey& sortKey, 
                                                                   bool ignoreUpdates)
:
    _model(model),
    _key(key),
    _sortKey(sortKey),
    _ignoreUpdates(ignoreUpdates),
    _root(QMailMessageId()),
    _initialised(false),
    _needSynchronize(true)
{
}

QMailMessageThreadedModelPrivate::~QMailMessageThreadedModelPrivate()
{
}

QMailMessageKey QMailMessageThreadedModelPrivate::key() const
{
    return _key; 
}

void QMailMessageThreadedModelPrivate::setKey(const QMailMessageKey& key) 
{
    _key = key;
}

QMailMessageSortKey QMailMessageThreadedModelPrivate::sortKey() const
{
   return _sortKey;
}

void QMailMessageThreadedModelPrivate::setSortKey(const QMailMessageSortKey& sortKey) 
{
    _sortKey = sortKey;
}

bool QMailMessageThreadedModelPrivate::isEmpty() const
{
    init();

    return _root._children.isEmpty();
}

int QMailMessageThreadedModelPrivate::rowCount(const QModelIndex &idx) const
{
    init();

    if (idx.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(idx)) {
            return item->_children.count();
        }
    } else {
        return _root._children.count();
    }

    return -1;
}

int QMailMessageThreadedModelPrivate::columnCount(const QModelIndex &idx) const
{
    init();

    return 1;
    
    Q_UNUSED(idx)
}

QMailMessageId QMailMessageThreadedModelPrivate::idFromIndex(const QModelIndex& idx) const
{
    init();

    if (idx.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(idx)) {
            return item->_id;
        }
    }

    return QMailMessageId();
}

QModelIndex QMailMessageThreadedModelPrivate::indexFromId(const QMailMessageId& id) const
{
    init();

    if (id.isValid()) {
        QMap<QMailMessageId, QMailMessageThreadedModelItem*>::const_iterator it = _messageItem.find(id);
        if (it != _messageItem.end()) {
            return indexFromItem(it.value());
        }
    }

    return QModelIndex();
}

Qt::CheckState QMailMessageThreadedModelPrivate::checkState(const QModelIndex &idx) const
{
    init();

    if (idx.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(idx)) {
            return (_checkedIds.contains(item->_id) ? Qt::Checked : Qt::Unchecked);
        }
    }

    return Qt::Unchecked;
}

void QMailMessageThreadedModelPrivate::setCheckState(const QModelIndex &idx, Qt::CheckState state)
{
    if (idx.isValid()) {
        if (QMailMessageThreadedModelItem *item = itemFromIndex(idx)) {
            // No support for partial checking in this model...
            if (state == Qt::Checked) {
                _checkedIds.insert(item->_id);
            } else {
                _checkedIds.remove(item->_id);
            }
        }
    }
}

int QMailMessageThreadedModelPrivate::rootRow(const QModelIndex& idx) const
{
    if (idx.isValid()) {
        QMailMessageThreadedModelItem *item = itemFromIndex(idx);
        while (item->_parent != &_root) {
            item = item->_parent;
        }

        return item->rowInParent();
    }

    return -1;
}

void QMailMessageThreadedModelPrivate::reset()
{
    _initialised = false;
}

bool QMailMessageThreadedModelPrivate::ignoreMailStoreUpdates() const
{
    return _ignoreUpdates;
}

bool QMailMessageThreadedModelPrivate::setIgnoreMailStoreUpdates(bool ignore)
{
    _ignoreUpdates = ignore;
    return (!_ignoreUpdates && _needSynchronize);
}

bool QMailMessageThreadedModelPrivate::processMessagesAdded(const QMailMessageIdList &ids)
{
    if (!_initialised) {
        // Nothing to do yet
        return true;
    }
    
    if (_ignoreUpdates) {
        // Defer until resynchronised
        _needSynchronize = true;
        return true;
    }

    if (_key.isEmpty()) {
        // No messages are relevant
        return true;
    }

    // Find if and where these messages should be added
    if (!addMessages(ids)) {
        return false;
    }

    return true;
}

bool QMailMessageThreadedModelPrivate::addMessages(const QMailMessageIdList &ids)
{
    // Are any of these messages members of our display set?
    // Note - we must only consider messages in the set given by (those we currently know +
    // those we have now been informed of) because the database content may have changed between
    // when this event was recorded and when we're processing the signal.
    
    QMailMessageKey idKey(QMailMessageKey::id(_currentIds + ids));
    const QMailMessageIdList newIds(QMailStore::instance()->queryMessages(_key & idKey, _sortKey));

    // Find which of the messages we must add (in ascending insertion order)
    QList<int> validIndices;
    QHash<QMailMessageId, int> idIndexMap;
    foreach (const QMailMessageId &id, ids) {
        int index = newIds.indexOf(id);
        if (index != -1) {
            validIndices.append(index);
            idIndexMap[id] = index;
        }
    }

    if (validIndices.isEmpty())
        return true;

    qSort(validIndices);

    QMailMessageIdList additionIds;
    foreach (int index, validIndices) {
        additionIds.append(newIds.at(index));
    }

    // Find all messages involved in conversations with the new messages, along with their predecessor ID
    QMailMessageKey conversationKey(QMailMessageKey::conversation(QMailMessageKey::id(additionIds)));
    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::InResponseTo);

    QMap<QMailMessageId, QMailMessageId> predecessor;
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(conversationKey, props)) {
        predecessor.insert(metaData.id(), metaData.inResponseTo());
    }

    // Process the messages to insert into the tree
    foreach (const QMailMessageId& id, additionIds) {
        // See if we have already added this message
        QMap<QMailMessageId, QMailMessageThreadedModelItem*>::iterator it = _messageItem.find(id);
        if (it != _messageItem.end()) {
            continue;
        }

        QMailMessageId messageId(id);
        QList<QMailMessageId> descendants;

        // Find the first message ancestor that is in our display set
        QMailMessageId predecessorId(predecessor[messageId]);
        while (predecessorId.isValid() && !newIds.contains(predecessorId)) {
            predecessorId = predecessor[predecessorId];
        }

        do {
            int messagePos = newIds.indexOf(messageId);

            QMailMessageThreadedModelItem *insertParent = 0;

            if (!predecessorId.isValid()) {
                // This message is a root node - we need to find where it fits in
                insertParent = &_root;
            } else {
                // Find the predecessor and add to the tree
                it = _messageItem.find(predecessorId);
                if (it != _messageItem.end()) {
                    insertParent = it.value();
                }
            }

            if (insertParent != 0) {
                int insertIndex = 0;

                // Find the insert location within the parent
                foreach (const QMailMessageId &id, insertParent->childrenIds()) {
                    int childPos = -1;
                    if (idIndexMap.contains(id)) {
                        childPos = idIndexMap[id];
                    } else {
                        childPos = newIds.indexOf(id);
                        idIndexMap[id] = childPos;
                    }
                    if ((childPos != -1) && (childPos < messagePos)) {
                        // Ths child precedes us in the sort order
                        ++insertIndex;
                    }
                }

                QModelIndex parentIndex = indexFromItem(insertParent);

                _model.emitBeginInsertRows(parentIndex, insertIndex, insertIndex);
                insertItemAt(insertIndex, parentIndex, messageId);
                _model.emitEndInsertRows();

                if (descendants.isEmpty()) {
                    messageId = QMailMessageId();
                } else {
                    // Process the descendants of the added node
                    predecessorId = messageId;
                    messageId = descendants.takeLast();
                }
            } else {
                // We need to add the parent before we can be added to it
                descendants.append(messageId);

                messageId = predecessorId;
                predecessorId = predecessor[messageId];
                while (predecessorId.isValid() && !newIds.contains(predecessorId)) {
                    predecessorId = predecessor[predecessorId];
                }
            }
        } while (messageId.isValid());
    }

    return true;
}

bool QMailMessageThreadedModelPrivate::processMessagesUpdated(const QMailMessageIdList &ids)
{
    if (!_initialised) {
        // Nothing to do yet
        return true;
    }
    
    if (_ignoreUpdates) {
        // Defer until resynchronised
        _needSynchronize = true;
        return true;
    }

    if (_key.isEmpty()) {
        // No messages are relevant
        return true;
    }

    // Find if and where these messages should be added/removed/updated
    if (!updateMessages(ids)) {
        return false;
    }

    return true;
}

bool QMailMessageThreadedModelPrivate::updateMessages(const QMailMessageIdList &ids)
{
    QSet<QMailMessageId> existingIds(_currentIds.toSet());

    QMailMessageKey idKey(QMailMessageKey::id((existingIds + ids.toSet()).toList()));
    QMailMessageIdList newIds(QMailStore::instance()->queryMessages(_key & idKey, _sortKey));

    QSet<QMailMessageId> currentIds(newIds.toSet());

    // Find which of the messages we must add and remove
    QMailMessageIdList additionIds;
    QMailMessageIdList removalIds;
    QMailMessageIdList temporaryRemovalIds;
    QMailMessageIdList updateIds;

    foreach (const QMailMessageId &id, ids) {
        bool existingMember(existingIds.contains(id));
        bool currentMember(currentIds.contains(id));

        if (!existingMember && currentMember) {
            additionIds.append(id);
        } else if (existingMember && !currentMember) {
            removalIds.append(id);
        } else if (currentMember) {
            updateIds.append(id);
        }
    }

    if (additionIds.isEmpty() && removalIds.isEmpty() && updateIds.isEmpty()) {
        return true;
    }

    // For the updated messages, find if they have a changed predecessor

    // Find all messages involved in conversations with the updated messages, along with their predecessor ID
    QMailMessageKey conversationKey(QMailMessageKey::conversation(QMailMessageKey::id(updateIds)));
    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::InResponseTo);

    QMap<QMailMessageId, QMailMessageId> predecessor;
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(conversationKey, props)) {
        predecessor.insert(metaData.id(), metaData.inResponseTo());
    }

    QList<QPair<QModelIndex, int> > updateIndices;

    foreach (const QMailMessageId &messageId, updateIds) {
        // Find the first message ancestor that is in our display set
        QMailMessageId predecessorId(predecessor[messageId]);
        while (predecessorId.isValid() && !currentIds.contains(predecessorId)) {
            predecessorId = predecessor[predecessorId];
        }

        bool reinsert(false);

        QMailMessageThreadedModelItem *item = _messageItem[messageId];
        if (item->_parent == &_root) {
            // This is a root item
            if (predecessorId.isValid()) {
                // This item now has a predecessor
                reinsert = true;
            }
        } else {
            if (item->_parent->_id != predecessorId) {
                // This item has a changed predecessor
                reinsert = true;
            }
        }

        if (!reinsert) {
            // We need to see if this item has changed in the sort order
            int row = item->rowInParent();
            int messagePos = newIds.indexOf(messageId);

            QList<QMailMessageThreadedModelItem> &container(item->_parent->_children);
            if (row > 0) {
                // Ensure that we still sort after our immediate predecessor
                if (newIds.indexOf(container.at(row - 1)._id) > messagePos) {
                    reinsert = true;
                }
            }
            if (row < (container.count() - 1)) {
                // Ensure that we still sort before our immediate successor
                if (newIds.indexOf(container.at(row + 1)._id) < messagePos) {
                    reinsert = true;
                }
            }
        }

        if (reinsert) {
            // Remove and re-add this item
            temporaryRemovalIds.append(messageId);
        } else {
            // Find the location for this updated item (not its parent)
            _model.emitDataChanged(index(item, 0));
        }
    }

    // Find the locations for removals
    removeMessages(removalIds, 0);

    // Find the locations for those to be removed and any children IDs that need to be reinserted after removal
    QMailMessageIdList readditionIds;
    removeMessages(temporaryRemovalIds, &readditionIds);

    // Find the locations for the added and reinserted messages
    addMessages((additionIds.toSet() + temporaryRemovalIds.toSet() + readditionIds.toSet()).toList());

    return true;
}

bool QMailMessageThreadedModelPrivate::processMessagesRemoved(const QMailMessageIdList &ids)
{
    if (!_initialised) {
        // Nothing to do yet
        return true;
    }
    
    if (_ignoreUpdates) {
        // Defer until resynchronised
        _needSynchronize = true;
        return true;
    }
    
    if (_key.isEmpty()) {
        // No messages are relevant
        return true;
    }

    // Find if and where these messages should be removed from
    if (!removeMessages(ids, 0)) {
        return false;
    }

    return true;
}

bool QMailMessageThreadedModelPrivate::removeMessages(const QMailMessageIdList &ids, QMailMessageIdList *readditions)
{
    QSet<QMailMessageId> removedIds;
    QSet<QMailMessageId> childIds;

    foreach (const QMailMessageId &id, ids) {
        if (!removedIds.contains(id)) {
            QMap<QMailMessageId, QMailMessageThreadedModelItem*>::iterator it = _messageItem.find(id);
            if (it != _messageItem.end()) {
                QMailMessageThreadedModelItem *item = it.value();
                QModelIndex idx(indexFromItem(item));

                // Find all the children of this item that need to be removed/re-added
                QList<const QMailMessageThreadedModelItem*> items;
                items.append(item);

                while (!items.isEmpty()) {
                    const QMailMessageThreadedModelItem *parent = items.takeFirst();
                    foreach (const QMailMessageThreadedModelItem &child, parent->_children) {
                        if (!removedIds.contains(child._id)) {
                            removedIds.insert(child._id);
                            if (readditions) {
                                // Don't re-add this child if it is being deleted itself
                                if (!ids.contains(child._id)) {
                                    childIds.insert(child._id);
                                }
                            }

                            items.append(&child);
                        }
                    }
                }

                _model.emitBeginRemoveRows(idx.parent(), item->rowInParent(), item->rowInParent());
                removeItemAt(item->rowInParent(), idx.parent());
                removedIds.insert(id);
                _model.emitEndRemoveRows();
            }
        }
    }
    
    if (readditions) {
        *readditions = childIds.toList();
    }

    return true;
}

void QMailMessageThreadedModelPrivate::insertItemAt(int row, const QModelIndex &parentIndex, const QMailMessageId &id)
{
    QMailMessageThreadedModelItem *parent;
    if (parentIndex.isValid()) {
        parent = itemFromIndex(parentIndex);
    } else {
        parent = &_root;
    }

    QList<QMailMessageThreadedModelItem> &container(parent->_children);

    container.insert(row, QMailMessageThreadedModelItem(id, parent));
    _messageItem[id] = &(container[row]);
    _currentIds.append(id);
}

void QMailMessageThreadedModelPrivate::removeItemAt(int row, const QModelIndex &parentIndex)
{
    QMailMessageThreadedModelItem *parent;
    if (parentIndex.isValid()) {
        parent = itemFromIndex(parentIndex);
    } else {
        parent = &_root;
    }

    QList<QMailMessageThreadedModelItem> &container(parent->_children);

    if (container.count() > row) {
        QMailMessageThreadedModelItem *item = &(container[row]);

        // Find all the descendants of this item that no longer exist
        QList<const QMailMessageThreadedModelItem*> items;
        items.append(item);

        while (!items.isEmpty()) {
            const QMailMessageThreadedModelItem *parent = items.takeFirst();
            foreach (const QMailMessageThreadedModelItem &child, parent->_children) {
                QMailMessageId childId(child._id);
                if (_messageItem.contains(childId)) {
                    _checkedIds.remove(childId);
                    _currentIds.removeAll(childId);
                    _messageItem.remove(childId);
                }

                items.append(&child);
            }
        }

        QMailMessageId id(item->_id);

        _checkedIds.remove(id);
        _currentIds.removeAll(id);
        _messageItem.remove(id);
        container.removeAt(row);
    }
}

QModelIndex QMailMessageThreadedModelPrivate::index(int row, int column, const QModelIndex &parentIndex) const
{
    init();

    if (row >= 0) {
        QMailMessageThreadedModelItem *parent;
        if (parentIndex.isValid()) {
            parent = itemFromIndex(parentIndex);
        } else {
            parent = &_root;
        }

        // Allow excessive row values (although these indices won't be dereferencable)
        void *item = 0;
        if (parent && (parent->_children.count() > row)) {
            item = static_cast<void*>(const_cast<QMailMessageThreadedModelItem*>(&(parent->_children.at(row))));
        }

        return _model.generateIndex(row, column, item);
    }

    return QModelIndex();
}

QModelIndex QMailMessageThreadedModelPrivate::parent(const QModelIndex &idx) const
{
    init();

    if (QMailMessageThreadedModelItem *item = itemFromIndex(idx))
        return parentIndex(item);

    return QModelIndex();
}

void QMailMessageThreadedModelPrivate::init() const
{
    if (!_initialised) {
        _root._children.clear();
        _messageItem.clear();
        _currentIds.clear();

        // Find all messages involved in conversations with the messages to show, along with their predecessor ID
        QMailMessageKey conversationKey(QMailMessageKey::conversation(_key));
        QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::InResponseTo);

        QMap<QMailMessageId, QMailMessageId> predecessor;
        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(conversationKey, props)) {
            predecessor.insert(metaData.id(), metaData.inResponseTo());
        }

        // Now find all the messages we're going to show, in order
        const QMailMessageIdList ids = QMailStore::instance()->queryMessages(_key, _sortKey);
        QHash<QMailMessageId, int> idIndexMap;
        idIndexMap.reserve(ids.count());
        int i;
        for (i = 0; i < ids.count(); ++i)
            idIndexMap[ids[i]] = i;

        // Process the messages to build a tree
        for (i = 0; i < ids.count(); ++i) {
            // See if we have already added this message
            QMap<QMailMessageId, QMailMessageThreadedModelItem*>::iterator it = _messageItem.find(ids[i]);
            if (it != _messageItem.end())
                continue;

            QMailMessageId messageId(ids[i]);
            
            QList<QMailMessageId> descendants;

            // Find the first message ancestor that is in our display set
            QMailMessageId predecessorId(predecessor[messageId]);
            while (predecessorId.isValid() && !ids.contains(predecessorId)) {
                predecessorId = predecessor[predecessorId];
            }

            do {
                QMailMessageThreadedModelItem *insertParent = 0;

                if (!predecessorId.isValid()) {
                    // This message is a root node
                    insertParent = &_root;
                } else {
                    // Find the predecessor and add to the tree
                    it = _messageItem.find(predecessorId);
                    if (it != _messageItem.end()) {
                        // Add this message to its parent
                        insertParent = it.value();
                    }
                }

                if (insertParent != 0) {
                    // Append the message to the existing children of the parent
                    QList<QMailMessageThreadedModelItem> &container(insertParent->_children);

                    // Determine where this message should sort amongst its siblings
                    int index = container.count();
                    for ( ; index > 0; --index) {
                        if (!idIndexMap.contains(container.at(index - 1)._id)) {
                            qWarning() << "Warning: Threading hash failure" << __FUNCTION__;
                            idIndexMap[container.at(index - 1)._id] = ids.indexOf(container.at(index - 1)._id);
                        }
                        if (idIndexMap[container.at(index - 1)._id] < i) {
                            break;
                        }
                    }

                    container.insert(index, QMailMessageThreadedModelItem(messageId, insertParent));
                    _messageItem[messageId] = &(container[index]);
                    _currentIds.append(messageId);
                    
                    if (descendants.isEmpty()) {
                        messageId = QMailMessageId();
                    } else {
                        // Process the descendants of the added node
                        predecessorId = messageId;
                        messageId = descendants.takeLast();
                    }
                } else {
                    // We need to add the parent before we can be added to it
                    descendants.append(messageId);

                    messageId = predecessorId;
                    predecessorId = predecessor[messageId];
                    while (predecessorId.isValid() && !ids.contains(predecessorId)) {
                        predecessorId = predecessor[predecessorId];
                    }
                }
            } while (messageId.isValid());
        }

        _initialised = true;
        _needSynchronize = false;
    }
}

QModelIndex QMailMessageThreadedModelPrivate::parentIndex(const QMailMessageThreadedModelItem *item) const
{
    if (const QMailMessageThreadedModelItem *parent = item->_parent)
        if (parent->_parent != 0)
            return index(parent, 0);

    return QModelIndex();
}

QModelIndex QMailMessageThreadedModelPrivate::index(const QMailMessageThreadedModelItem *item, int column) const
{
    if (item->_parent)
        return _model.generateIndex(item->rowInParent(), column, const_cast<void*>(static_cast<const void*>(item)));

    return QModelIndex();
}

QMailMessageThreadedModelItem *QMailMessageThreadedModelPrivate::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<QMailMessageThreadedModelItem*>(index.internalPointer());

    return 0;
}

QModelIndex QMailMessageThreadedModelPrivate::indexFromItem(const QMailMessageThreadedModelItem *item) const
{
    return index(item, 0);
}


/*!
    \class QMailMessageThreadedModel 

    \preliminary
    \ingroup messaginglibrary 
    \brief The QMailMessageThreadedModel class provides access to a tree of stored messages. 

    The QMailMessageThreadedModel presents a tree of all the messages currently stored in
    the message store. By using the setKey() and sortKey() functions it is possible to 
    have the model represent specific user filtered subsets of messages sorted in a particular order.

    The QMailMessageThreadedModel represents the hierarchical links between messages 
    implied by conversation threads.  The model presents messages as children of predecessor
    messages, where the parent is the nearest ancestor of the message that is present in
    the filtered set of messages.
*/

/*!
    Constructs a QMailMessageThreadedModel with a parent \a parent.

    By default, the model will match all messages in the database, and display them in
    the order they were submitted, and mail store updates are not ignored.

    \sa setKey(), setSortKey(), setIgnoreMailStoreUpdates()
*/
QMailMessageThreadedModel::QMailMessageThreadedModel(QObject* parent)
    : QMailMessageModelBase(parent),
      d(new QMailMessageThreadedModelPrivate(*this, QMailMessageKey::nonMatchingKey(), QMailMessageSortKey::id(), false))
{
}

/*!
    Deletes the QMailMessageThreadedModel object.
*/
QMailMessageThreadedModel::~QMailMessageThreadedModel()
{
    delete d; d = 0;
}

// TODO - is this useful?  Maybe "QMailMessageId rootAncestor()" instead?
/*! \internal */
int QMailMessageThreadedModel::rootRow(const QModelIndex& idx) const
{
    return d->rootRow(idx);
}

/*! \reimp */
QModelIndex QMailMessageThreadedModel::index(int row, int column, const QModelIndex &parentIndex) const
{
    return d->index(row, column, parentIndex);
}

/*! \reimp */
QModelIndex QMailMessageThreadedModel::parent(const QModelIndex &idx) const
{
    return d->parent(idx);
}

/*! \reimp */
QModelIndex QMailMessageThreadedModel::generateIndex(int row, int column, void *ptr)
{
    return createIndex(row, column, ptr);
}

/*! \reimp */
QMailMessageModelImplementation *QMailMessageThreadedModel::impl()
{
    return d;
}

/*! \reimp */
const QMailMessageModelImplementation *QMailMessageThreadedModel::impl() const
{
    return d;
}

