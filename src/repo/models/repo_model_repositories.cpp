/**
 *  Copyright (C) 2016 3D Repo Ltd
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "repo_model_repositories.h"

using namespace repo;
using namespace repo::gui::widget;
using namespace repo::models;
using namespace repo::gui::primitive;
using namespace repo::worker;

const QString RepositoriesModel::REPO_SETTINGS_REPOSITORIES_MODEL_HEADERS = "RepositoriesModel/headers";


RepositoriesModel::RepositoriesModel(
        RepoController *controller,
        FilterableTreeWidget *widget)
    : controller(controller)
    , widget(widget)
{

    widget->restoreHeaders({"Name", "Count", "Actual", "Allocated"},
                           REPO_SETTINGS_REPOSITORIES_MODEL_HEADERS);
    widget->getTreeView()->sortByColumn(RepoDatabasesColumns::NAME, Qt::SortOrder::AscendingOrder);

    proxy = new repo::gui::primitive::RepoSortFilterProxyModel((QWidget*) widget, false);
    widget->setProxyModel(proxy);

    QObject::connect(
                widget, &FilterableTreeWidget::expanded,
                this, &RepositoriesModel::expand);

    QObject::connect(
                widget, &FilterableTreeWidget::collapsed,
                this, &RepositoriesModel::collapse);
}

RepositoriesModel::~RepositoriesModel()
{
    cancelAllThreads();
    widget->storeHeaders(REPO_SETTINGS_REPOSITORIES_MODEL_HEADERS);
    delete proxy;
}

bool RepositoriesModel::cancelAllThreads()
{
    emit cancel();
    return threadPool.waitForDone(); // msecs
}

void RepositoriesModel::connect(RepoController::RepoToken* token)
{    
    addHost(token);
}

bool RepositoriesModel::disconnect()
{
    //    controller->disconnectFromDatabase(token);
    //    controller->destroyToken(token);
    //    token = nullptr;
    //    clearDatabaseModel();
    //    clearCollectionModel();
    return false;

}

repo::RepoController::RepoToken* RepositoriesModel::getConnection(
        const QString &host) const
{
    RepoController::RepoToken* token = nullptr;
    for(int i = 0; i < widget->getModel()->invisibleRootItem()->rowCount(); ++i)
    {
        RepoStandardItem* hostItem = (RepoStandardItem *)widget->getModel()->invisibleRootItem()->child(i);
        if (host == getRow(hostItem)[NAME]->text())
        {
            token = (RepoController::RepoToken*)
                    hostItem->data(Qt::UserRole + 2).value<void *>();
        }
    }
    return token;
}

QList<QString> RepositoriesModel::getDatabases(const QString& host) const
{
    qDebug() << "getDatabases host: " << host;
    QStringList databases;
    for (int i = 0; i < widget->getModel()->invisibleRootItem()->rowCount(); ++i)
    {
        RepoStandardItem* hostItem = (RepoStandardItem *)widget->getModel()->invisibleRootItem()->child(i);
        if (host == getRow(hostItem)[NAME]->text())
        {
            for (int j = 0; j < hostItem->rowCount(); ++j)
            {
                databases.append(getRow(hostItem->child(j))[NAME]->text());
            }
            break;
        }
    }
    return databases;
}

QList<QString> RepositoriesModel::getHosts() const
{
    // TODO: Make sure duplicates are handled correctly by whoever uses this
    // Should really change to a map of connections/tokens against string labels
    QList<QString> hosts;
    for (int i = 0; i < widget->getModel()->invisibleRootItem()->rowCount(); ++i)
    {
        RepoStandardItem* host = (RepoStandardItem *)widget->getModel()->invisibleRootItem()->child(i);
        hosts.append(getRow(host)[NAME]->text());
    }
    return hosts;
}

QList<QString> RepositoriesModel::getProjects(
        const QString &host,
        const QString &database) const
{
    // TODO: Change so that a host token is passed in rather than a string for host
    // DB can stay as a string
    // Also, this whole data structure could be turned into a map but won't bother for now

    QStringList projects;
    for (int i = 0; i < widget->getModel()->invisibleRootItem()->rowCount(); ++i)
    {
        RepoStandardItem* hostItem = (RepoStandardItem *)widget->getModel()->invisibleRootItem()->child(i);
        if (host == getRow(hostItem)[NAME]->text())
        {
            for (int j = 0; j < hostItem->rowCount(); ++j)
            {
                RepoStandardItem* databaseItem = (RepoStandardItem *)hostItem->child(j);
                if (database == getRow(databaseItem)[NAME]->text())
                {
                    for (int k = 0; k < databaseItem->rowCount(); ++k)
                    {
                        projects.append(getRow(databaseItem->child(k))[NAME]->text());
                    }
                    break;
                }
            }
        }
    }
    return projects;
}

void RepositoriesModel::refresh()
{
    for(int i = 0; i < widget->getModel()->invisibleRootItem()->rowCount(); ++i)
    {
        refreshHost((RepoStandardItem *)widget->getModel()->invisibleRootItem()->child(i));
    }
}

repo::RepoController::RepoToken* RepositoriesModel::getSelectedConnection() const
{
    RepoController::RepoToken* token = nullptr;
    RepoStandardItem * item = (RepoStandardItem *) widget->getCurrentItem(NAME);
    if (item)
    {
        switch (item->type())
        {
        case HOST_DIRTY:
            // Item is already corret, do nothing.
            break;
        case DATABASE_DIRTY :
            item = (RepoStandardItem *) item->parent(); // parent is host
            break;
        case PROJECT_DIRTY :
            item = (RepoStandardItem *) item->parent(); // parent is DB
            if (item)
                item = (RepoStandardItem *) item->parent(); // parent is host
            break;
        }
        if (item)
            token = (RepoController::RepoToken*)
                        item->data(Qt::UserRole + 2).value<void *>();
    }
    return token;
}

//! Returns selected database.
QString RepositoriesModel::getSelectedDatabase() const
{
    QString database;
    RepoStandardItem * item = (RepoStandardItem *) widget->getCurrentItem(NAME);
    if (item)
    {
        switch (item->type())
        {
        case HOST_DIRTY:
            // Do nothing, if host is selected, database is empty
            break;
        case DATABASE_DIRTY :
            database = getRow(item)[NAME]->text(); // item is DB
            break;
        case PROJECT_DIRTY :
            item = (RepoStandardItem *) item->parent(); // parent is DB
            if (item)
                database = getRow(item)[NAME]->text();
            break;
        }
    }
    return database;
}

//! Returns selected host.
QString RepositoriesModel::getSelectedHost() const
{
    QString host;
    RepoStandardItem * item = (RepoStandardItem *) widget->getCurrentItem(NAME);

    // If there is only a single server connected, use that directly even if the actual
    // host item is not selected by the user
    if (widget->getModel()->invisibleRootItem()->rowCount() == 1)
        item = (RepoStandardItem *) widget->getModel()->invisibleRootItem()->child(0);

    if (item)
    {
        switch (item->type())
        {
        case HOST_DIRTY:
        case HOST_CACHED:
            host = getRow(item)[NAME]->text(); // item is host
            break;
        case DATABASE_DIRTY :
        case DATABASE_CACHED:
            item = (RepoStandardItem *) item->parent(); // parent is host
            if (item)
                host = getRow(item)[NAME]->text();
            break;
        case PROJECT_DIRTY :
        case PROJECT_CACHED:
            item = (RepoStandardItem *) item->parent(); // parent is DB
            if (item)
            {
                item = (RepoStandardItem *) item->parent(); // parent is host
                if (item)
                    host = getRow(item)[NAME]->text();
            }
            break;
        }
    }
    return host;
}

QString RepositoriesModel::getSelectedProject() const
{    
    QString project;
    RepoStandardItem * item = (RepoStandardItem *) widget->getCurrentItem(NAME);
    if (item && item->type() == PROJECT_DIRTY)
        project = getRow(item)[NAME]->text();
    return project;
}

void RepositoriesModel::refreshHost(RepoStandardItem *host)
{
    //--------------------------------------------------------------------------
    // Cancel any previously running threads.
    if (cancelAllThreads())
    {
        repoLog("Fetching databases...");
        RepoController::RepoToken* token = (RepoController::RepoToken*)
                host->data(Qt::UserRole + 2).value<void *>();

        //----------------------------------------------------------------------
        // Clear any previous entries in the databases and collection models
        host->removeRows(0, host->rowCount());

        RepoStandardItemRow hostRow = getRow(host);

        // Rest counts
        hostRow[COUNT]->setDataNumber(0);
        hostRow[ALLOCATED]->setDataNumber(0, true);
        hostRow[RepoDatabasesColumns::SIZE]->setDataNumber(0, true);

        //----------------------------------------------------------------------
        DatabasesWorker * worker = new DatabasesWorker(controller, token, hostRow);
        worker->setAutoDelete(true);

        //----------------------------------------------------------------------
        // Direct connection ensures cancel signal is processed ASAP
        QObject::connect(
                    this, &RepositoriesModel::cancel,
                    worker, &DatabasesWorker::cancel, Qt::DirectConnection);

        QObject::connect(
                    worker, &DatabasesWorker::databaseFetched,
                    this, &RepositoriesModel::addDatabase);

        QObject::connect(
                    worker, &DatabasesWorker::databaseStatsFetched,
                    this, &RepositoriesModel::setDatabaseStats);

        QObject::connect(
                    worker, &DatabasesWorker::finished,
                    widget->getProgressBar(), &QProgressBar::hide);

        QObject::connect(
                    worker, &DatabasesWorker::progressRangeChanged,
                    widget->getProgressBar(), &QProgressBar::setRange);

        QObject::connect(
                    worker, &DatabasesWorker::progressValueChanged,
                    widget->getProgressBar(), &QProgressBar::setValue);

        //----------------------------------------------------------------------
        widget->getProgressBar()->show();
        threadPool.start(worker);
    }
}

void RepositoriesModel::refreshDatabase(RepoStandardItem *database)
{
    //        repoLog("Fetching projects...");
    RepoController::RepoToken* token = (RepoController::RepoToken*)
            database->parent()->data(Qt::UserRole + 2).value<void *>();

    //----------------------------------------------------------------------
    // Clear any previous entries in the databases and collection models
    database->removeRows(0, database->rowCount());

    RepoStandardItemRow databaseRow = getRow(database);

    //        // Rest counts
    //        hostRow[COUNT]->setDataNumber(0);
    //        hostRow[ALLOCATED]->setDataNumber(0, true);
    //        hostRow[RepoDatabasesColumns::SIZE]->setDataNumber(0, true);

    //----------------------------------------------------------------------
    RepositoriesWorker * worker = new RepositoriesWorker(controller, token, databaseRow);
    worker->setAutoDelete(true);

    //----------------------------------------------------------------------
    // Direct connection ensures cancel signal is processed ASAP
    QObject::connect(
                this, &RepositoriesModel::cancel,
                worker, &RepositoriesWorker::cancel, Qt::DirectConnection);

    QObject::connect(
                worker, &RepositoriesWorker::projectFetched,
                this, &RepositoriesModel::addProject);

    //        QObject::connect(
    //                    worker, &DatabasesWorker::databaseStatsFetched,
    //                    this, &RepositoriesWorker::setDatabaseStats);

    //        QObject::connect(
    //                    worker, &RepositoriesWorker::finished,
    //                    widget->getProgressBar(), &QProgressBar::hide);

    //        QObject::connect(
    //                    worker, &DatabasesWorker::progressRangeChanged,
    //                    widget->getProgressBar(), &QProgressBar::setRange);

    //        QObject::connect(
    //                    worker, &DatabasesWorker::progressValueChanged,
    //                    widget->getProgressBar(), &QProgressBar::setValue);

    //----------------------------------------------------------------------
    //        widget->getProgressBar()->show();
    threadPool.start(worker);

}

void RepositoriesModel::addHost(RepoController::RepoToken* token)
{
    RepoStandardItemRow hostRow = RepoStandardItemFactory::makeHostRow(controller, token);
    addHost(hostRow);
    //    return hostRow;
}

void RepositoriesModel::addHost(const RepoStandardItemRow &hostRow)
{    
    widget->addTopLevelRow(hostRow.toQList());
    widget->expandItem(hostRow[NAME]);
}

void RepositoriesModel::addDatabase(const RepoStandardItemRow &hostRow,
                                    const RepoStandardItemRow &databaseRow)
{
    hostRow[NAME]->appendRow(databaseRow.toQList());
}

void RepositoriesModel::addProject(const RepoStandardItemRow &databaseRow,
                                   const RepoStandardItemRow &projectRow)
{
    databaseRow[NAME]->appendRow(projectRow.toQList());
}

void RepositoriesModel::setDatabaseStats(const RepoStandardItemRow &hostRow,
                                         const RepoStandardItemRow &databaseRow,
                                         const core::model::DatabaseStats &databaseStats)
{  
    uint64_t count = databaseStats.getCollectionsCount();
    uint64_t allocated = databaseStats.getStorageSize();
    uint64_t size = databaseStats.getDataSize();

    databaseRow[COUNT]->setDataNumber(count);
    databaseRow[ALLOCATED]->setDataNumber(allocated, true);
    databaseRow[repo::gui::primitive::RepoDatabasesColumns::SIZE]->setDataNumber(
                size, true);

    hostRow[COUNT]->setDataNumber(hostRow[COUNT]->data().toLongLong() + count);
    hostRow[ALLOCATED]->setDataNumber(hostRow[ALLOCATED]->data().toLongLong() + allocated, true);
    hostRow[RepoDatabasesColumns::SIZE]->setDataNumber(
                hostRow[RepoDatabasesColumns::SIZE]->data().toLongLong() + size, true);
}

void RepositoriesModel::expand(QStandardItem *item)
{
    switch (item->type())
    {
    case RepoDatabasesTypes::HOST_DIRTY :
        refreshHost((RepoStandardItem *)item);
        break;
    case RepoDatabasesTypes::DATABASE_DIRTY :
        refreshDatabase((RepoStandardItem *)item);
        break;
    case RepoDatabasesTypes::PROJECT_DIRTY :
        break;
    }
}

void RepositoriesModel::collapse(QStandardItem *item)
{
    switch (item->type())
    {
    case RepoDatabasesTypes::HOST_DIRTY :
        // cancel loading
        cancelAllThreads();
        break;
    case RepoDatabasesTypes::DATABASE_DIRTY :
        // cancel loading
        break;
    case RepoDatabasesTypes::PROJECT_DIRTY :
        // cancel ??
        break;
    }
}

RepoStandardItemRow RepositoriesModel::getRow(const QStandardItem *item) const
{
    RepoStandardItemRow row;
    row.append((RepoStandardItem *)widget->getSiblingFromItem(item, NAME));
    row.append((RepoStandardItem *)widget->getSiblingFromItem(item, COUNT));
    row.append((RepoStandardItem *)widget->getSiblingFromItem(item, ALLOCATED));
    row.append((RepoStandardItem *)widget->getSiblingFromItem(item, RepoDatabasesColumns::SIZE));
    return row;
}
