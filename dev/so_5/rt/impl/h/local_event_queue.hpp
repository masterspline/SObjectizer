/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A definition of agent's local event queue.
*/

#if !defined( _SO_5__RT__IMPL__LOCAL_EVENT_QUEUE_HPP_ )
#define _SO_5__RT__IMPL__LOCAL_EVENT_QUEUE_HPP_

#include <deque>
#include <memory>

#include <ace/Thread_Mutex.h>

#include <so_5/util/h/mutex_pool.hpp>

#include <so_5/rt/h/event_caller_block_ref.hpp>
#include <so_5/rt/h/message.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

//
// event_item_t
//

//! Event queue item.
struct event_item_t
{
	event_item_t()
	{}

	event_item_t(
		//! Event handler caller.
		const event_caller_block_ref_t & event_caller_block,
		//! Message.
		const message_ref_t & message_ref )
		:
			m_event_caller_block( event_caller_block ),
			m_message_ref( message_ref )
	{}

	//! Event handler caller.
	event_caller_block_ref_t m_event_caller_block;

	//! Message.
	message_ref_t m_message_ref;
};

//
// local_event_queue_t
//

//! Agent's local event queue.
/*!
 * \attention Not thread safe. Operation on queue should be
 * protected by agent. Synchronization object for that queue
 * could be obtained by local_event_queue_t::lock() method.
 */
class local_event_queue_t
{
	public:
		//! Constructor.
		explicit local_event_queue_t(
			util::mutex_pool_t< ACE_Thread_Mutex > & mutex_pool );
		~local_event_queue_t();

		//! Get the first event from queue.
		inline void
		pop( event_item_t & event_item );

		//! Push new event to the end of queue.
		inline void
		push(
			const event_item_t & evt );

		//! Get object lock.
		inline ACE_Thread_Mutex &
		lock();

		//! Current queue size.
		/*!
		 * \attention Not thread safe.
		 */
		inline size_t
		size() const;

		//! Clear queue.
		inline void
		clear();

	private:
		//! Mutex pool from which mutex has been obtained.
		/*!
		 * This reference is necessary to return mutex back.
		 */
		util::mutex_pool_t< ACE_Thread_Mutex > & m_mutex_pool;

		//! Reference to object lock.
		/*!
		 * This lock is obtained from \a m_mutex_pool.
		 */
		ACE_Thread_Mutex & m_lock;

		//! Typedef for queue underlying container type.
		typedef std::deque< event_item_t > events_queue_t;

		//! Events queue.
		events_queue_t m_events_queue;
};

inline void
local_event_queue_t::pop(
	event_item_t & event_item )
{
	event_item = m_events_queue.front();
	m_events_queue.pop_front();
}

inline void
local_event_queue_t::push(
	const event_item_t & evt )
{
	m_events_queue.push_back( evt );
}

inline ACE_Thread_Mutex &
local_event_queue_t::lock()
{
	return m_lock;
}

inline size_t
local_event_queue_t::size() const
{
	return m_events_queue.size();
}

inline void
local_event_queue_t::clear()
{
	m_events_queue.clear();
}

//! Typedef for local_event_queue autopointer.
typedef std::unique_ptr< local_event_queue_t >
	local_event_queue_unique_ptr_t;

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif

