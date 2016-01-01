/**
 *  Copyright (C) 2015 3D Repo Ltd
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


#include "repo_widget_tree_unfilterable.h"
#include "ui_repo_widget_tree_unfilterable.h"

using namespace repo::widgets;

RepoWidgetTreeUnfilterable::RepoWidgetTreeUnfilterable(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RepoWidgetTreeUnfilterable)
    , newRowText({tr("<empty>"), tr("<empty>")})
    , tabWidget(0)
    , tab(0)
{
    ui->setupUi(this);

    //--------------------------------------------------------------------------

    QObject::connect(
                ui->treeWidget, &QTreeWidget::itemChanged,
                this, &RepoWidgetTreeUnfilterable::updateDelegate);

    QObject::connect(
                ui->addPushButton, SIGNAL(pressed()),
                this, SLOT(addRow()));

    QObject::connect(
                ui->removePushButton, SIGNAL(pressed()),
                this, SLOT(removeRow()));

    QObject::connect(
                this, &RepoWidgetTreeUnfilterable::rowCountChanged,
                this, &RepoWidgetTreeUnfilterable::notifyTabTextChange);
}

RepoWidgetTreeUnfilterable::~RepoWidgetTreeUnfilterable()
{
    delete ui;

    //--------------------------------------------------------------------------
    // Dealocate pointers
    QListIterator<QString> i(delegates.keys());
    while (i.hasNext())
    {
        QString item = i.next();
        delete delegates.take(item);
    }
}

void RepoWidgetTreeUnfilterable::setButtonsEnabled(bool enabled)
{
    ui->addPushButton->setEnabled(enabled);
    ui->removePushButton->setEnabled(enabled);
}

void RepoWidgetTreeUnfilterable::setHeaders(const QStringList &headers)
{
    ui->treeWidget->setHeaderLabels(headers);
}

void RepoWidgetTreeUnfilterable::removeRow()
{
    int oldRowCount = getRowCount();
    QTreeWidgetItem *item = 0;
    item = ui->treeWidget->currentItem();

    if (item)
    {
        delete item;

        int newRowCount = getRowCount();
        emit rowCountChanged(oldRowCount, newRowCount);
    }
}

QTreeWidgetItem *RepoWidgetTreeUnfilterable::addRow(const QStringList &list, bool enabled)
{
    QTreeWidgetItem *item = 0;
    if (list.size() == ui->treeWidget->columnCount())
    {
        int oldRowCount = getRowCount();
        item = new QTreeWidgetItem(ui->treeWidget, list);
        int i = 0;
        for (QString s : list)
        {
            item->setData(i++, Qt::DecorationRole, s);
        }
        Qt::ItemFlags flags = Qt::ItemIsEditable;
        if (enabled)
            flags |= Qt::ItemIsEnabled;
        item->setFlags(flags);

        ui->treeWidget->addTopLevelItem(item);

        setItemDelegateForRow(list[0]);

        ui->treeWidget->scrollToItem(item);
        ui->treeWidget->setCurrentItem(item ,0);

        int newRowCount = getRowCount();
        emit rowCountChanged(oldRowCount, newRowCount);
    }
    return item;
}

QTreeWidgetItem *RepoWidgetTreeUnfilterable::addRow(const QString &a, const QString &b)
{
    return addRow({a, b});
}

QTreeWidgetItem *RepoWidgetTreeUnfilterable::addRow(
        const std::pair<std::string, std::string> &pair,
        bool enabled)
{
    return addRow(
        {QString::fromStdString(pair.first), QString::fromStdString(pair.second)},
        enabled);
}

void RepoWidgetTreeUnfilterable::addRows(const std::list<std::pair<std::string, std::string> > &list)
{
    for (auto pair : list)
        addRow(pair);
}

std::list<std::pair<std::string, std::string> > RepoWidgetTreeUnfilterable::getItemsAsListOfPairsOfStrings() const
{
    std::list<std::pair<std::string, std::string> > list;
    if (ui->treeWidget->columnCount() >= 2)
    {
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem *item = ui->treeWidget->topLevelItem(i);
            std::string a = item->data(0, Qt::EditRole).toString().toStdString();
            std::string b = item->data(1, Qt::EditRole).toString().toStdString();
            list.push_back(std::make_pair(a, b));
        }
    }
    return list;
}

std::vector<std::string> RepoWidgetTreeUnfilterable::getItemsAsVectorOfStrings() const
{
    std::vector<std::string> vector(ui->treeWidget->topLevelItemCount());
    if (ui->treeWidget->columnCount() > 0)
    {
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem *item = ui->treeWidget->topLevelItem(i);
            vector[i] = item->data(0, Qt::EditRole).toString().toStdString();
        }
    }
    return vector;
}

int RepoWidgetTreeUnfilterable::getRowCount() const
{
    return ui->treeWidget->topLevelItemCount();
}

void RepoWidgetTreeUnfilterable::setItemDelegateForRow(const QString &item)
{
    setItemDelegateForRow(item, ui->treeWidget->topLevelItemCount() - 1);
}

void RepoWidgetTreeUnfilterable::setItemDelegateForRow(const QString &item, int row)
{
    if (delegates.contains(item))
    {
        ui->treeWidget->setItemDelegateForRow(row, delegates.value(item));
    }
}

void RepoWidgetTreeUnfilterable::setItemDelegateForColumn(const QString &item, int column)
{
    if (delegates.contains(item))
    {
        ui->treeWidget->setItemDelegateForColumn(column, delegates.value(item));
    }
}

void RepoWidgetTreeUnfilterable::updateDelegate(QTreeWidgetItem *current, int column)
{
    if (delegates.size() > 0 && current && 0 == column)
    {
        int row = ui->treeWidget->indexOfTopLevelItem(current);
        QString item = current->data(column, Qt::EditRole).toString();
        if (delegates.contains(item))
        {
            setItemDelegateForRow(item, row);
        }
        ui->treeWidget->setCurrentItem(current,
                                       ui->treeWidget->columnCount() > 1
                                       ? 1
                                       : 0);
    }
}

void RepoWidgetTreeUnfilterable::notifyTabTextChange(int oldRowCount, int newRowCount)
{
    QString text;
    if (tabWidget)
    {
        text = updateCountString(tabWidget->tabText(tab), oldRowCount, newRowCount);
    }
    emit tabTextChanged(tab, text);
}

void RepoWidgetTreeUnfilterable::registerTabWidget(QTabWidget *tabWidget, int tab)
{
    if (this->tabWidget)
    {
        QObject::disconnect(this, &RepoWidgetTreeUnfilterable::tabTextChanged,
                            this->tabWidget, &QTabWidget::setTabText);
    }

    this->tabWidget = tabWidget;
    this->tab = tab;
    QObject::connect(this, &RepoWidgetTreeUnfilterable::tabTextChanged,
                     tabWidget, &QTabWidget::setTabText);
}

QString RepoWidgetTreeUnfilterable::updateCountString(
        QString string,
        int oldCount,
        int newCount)
{
    QLocale locale;
    int chopCount = locale.toString(oldCount).length() + 3; // 2 brackets + 1 space
    string.chop(chopCount);
    string += QString(" (") + locale.toString(newCount) + QString(")");
    return string;
}