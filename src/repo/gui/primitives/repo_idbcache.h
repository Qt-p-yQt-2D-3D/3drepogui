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

#pragma once

//------------------------------------------------------------------------------
// Core
#include <repo/repo_controller.h>

//------------------------------------------------------------------------------
// GUI
#include "../primitives/repo_fontawesome.h"

//------------------------------------------------------------------------------
// Qt
#include <QList>
#include <QString>
#include <QComboBox>

namespace repo {
namespace gui {
namespace primitive{

	//! Interface to database cache.
	class RepoIDBCache
	{

	public:

		virtual ~RepoIDBCache() {}

	public:

		//! Returns connection corresponding to given host.
		virtual repo::RepoController::RepoToken* getConnection(const QString &host) const = 0;

		//! Returns a list of available databases.
		virtual QList<QString> getDatabases(const QString& host) const = 0;

		//! Returns a list of available hosts.
		virtual QList<QString> getHosts() const = 0;

		//! Returns a list of available projects (a subset of all collections in a given database).
		virtual QList<QString> getProjects(const QString &host, const QString &database) const = 0;

		//! Refreshes cache.
		virtual void refresh() = 0;

		//! Returns selected connection, needs to be reconnected and reauthenticated.
		virtual repo::RepoController::RepoToken* getSelectedConnection() const = 0;

		//! Returns selected database.
		virtual QString getSelectedDatabase() const = 0;

		//! Returns selected host.
		virtual QString getSelectedHost() const = 0;

		//! Returns selected project.
		virtual QString getSelectedProject() const = 0;


		//--------------------------------------------------------------------------
		//
		// Helpers
		//
		//--------------------------------------------------------------------------

	public:

		virtual void setHostsComboBox(QComboBox *comboBox) const;

		virtual void setDatabasesComboBox(QComboBox *comboBox) const;

        virtual void setDatabasesComboBox(QComboBox *comboBox, const QString &selection) const;

		virtual void setProjectsComboBox(QComboBox *comboBox) const;

		//! Populates given combo box with given items.
		virtual void setComboBox(
			QComboBox *comboBox,
			const QIcon &icon,
			const QList<QString> &list,
			const QString &selected = QString()) const;

	};
}
} // end namespace gui
} // end namespace repo

