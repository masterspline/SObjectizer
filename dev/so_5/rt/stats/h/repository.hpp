/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since v.5.5.4
 * \brief Interfaces of data source and data sources repository.
 */

#pragma once

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/mbox.hpp>

namespace so_5
{

namespace rt
{

namespace stats
{

/*!
 * \since v.5.5.4
 * \brief An interface of data source.
 */
class SO_5_TYPE source_t
	{
		friend class repository_t;

	protected :
		source_t();
		~source_t();

	public :
		//! Send appropriate notification about the current value.
		virtual void
		distribute(
			//! Target mbox for the appropriate message.
			const mbox_t & distribution_mbox ) = 0;

	private :
		//! Previous item in the data sources list.
		source_t * m_prev;
		//! Next item in the data sources list.
		source_t * m_next;
	};

/*!
 * \since v.5.5.4
 * \brief An interface of data sources repository.
 */
class SO_5_TYPE repository_t
	{
	protected :
		~repository_t();

	public :
		//! Registration of new data source.
		/*!
		 * Caller must guarantee that the data source will live until
		 * it is registered in the repository.
		 */
		virtual void
		add( source_t & what ) = 0;

		//! Deregistration of previously registered data source.
		virtual void
		remove( source_t & what ) = 0;

	protected :
		//! Helper method for adding data source to existing list.
		static void
		source_list_add(
			//! A new data source to be added to the list.
			source_t & what,
			//! Marker of the list head.
			//! Will be modified if the list is empty.
			source_t *& head,
			//! Marker of the list tail.
			//! Will be modified.
			source_t *& tail );

		//! Helper method for removing data source from existing list.
		static void
		source_list_remove(
			//! An item to be removed.
			source_t & what,
			//! Marker of the list head.
			//! Will be modified if the list becomes empty.
			source_t *& head,
			//! Marker of the list tail.
			//! Will be modified if the item at the end of the list.
			source_t *& tail );

		//! Helper method for accessing next data source in the list.
		static source_t *
		source_list_next(
			//! The current item.
			const source_t & what );
	};

} /* namespace stats */

} /* namespace rt */

} /* namespace so_5 */

