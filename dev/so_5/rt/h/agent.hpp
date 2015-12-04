/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A base class for agents.
*/

#pragma once

#include <so_5/h/compiler_features.hpp>
#include <so_5/h/declspec.hpp>
#include <so_5/h/types.hpp>
#include <so_5/h/current_thread_id.hpp>
#include <so_5/h/atomic_refcounted.hpp>
#include <so_5/h/spinlocks.hpp>

#include <so_5/h/exception.hpp>
#include <so_5/h/error_logger.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>
#include <so_5/details/h/abort_on_fatal_error.hpp>

#include <so_5/rt/h/fwd.hpp>

#include <so_5/rt/h/agent_ref_fwd.hpp>
#include <so_5/rt/h/agent_context.hpp>
#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/agent_state_listener.hpp>
#include <so_5/rt/h/event_queue.hpp>
#include <so_5/rt/h/subscription_storage_fwd.hpp>
#include <so_5/rt/h/handler_makers.hpp>

#include <atomic>
#include <map>
#include <memory>
#include <vector>
#include <utility>
#include <type_traits>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

/*!
 * \since v.5.3.0
 * \brief A special class which will be used as return value for
 * signal-indication helper.
 *
 * \tparam S type of signal.
 */
template< class S >
struct signal_indicator_t {};

/*!
 * \since v.5.3.0
 * \brief A special signal-indicator.
 *
 * Must be used as signal-indicator in subscription_bind_t::event()
 * methods:
\code
virtual void
my_agent_t::so_define_agent() {
	so_subscribe(mbox).event(
			so_5::signal< get_status >,
			&my_agent_t::evt_get_status );

	so_subscribe(mbox).event(
			so_5::signal< shutdown >,
			[this]() { so_environment().stop(); } );
}
\endcode
 *
 * \tparam S type of signal.
 */
template< class S >
signal_indicator_t< S >
signal() { return signal_indicator_t< S >(); }

//
// exception_reaction_t
//
/*!
 * \since v.5.2.3
 * \brief A reaction of SObjectizer to an exception from agent event.
 */
enum exception_reaction_t
{
	//! Execution of application must be aborted immediatelly.
	abort_on_exception = 1,
	//! Agent must be switched to special state and SObjectizer
	//! Environment will be stopped.
	shutdown_sobjectizer_on_exception = 2,
	//! Agent must be switched to special state and agent's cooperation
	//! must be deregistered.
	deregister_coop_on_exception = 3,
	//! Exception should be ignored and agent should continue its work.
	ignore_exception = 4,
	/*!
	 * \since v.5.3.0
	 * \brief Exception reaction should be inherited from SO Environment.
	 */
	inherit_exception_reaction = 5
};

//
// subscription_bind_t
//

/*!
 * \brief A class for creating a subscription to messages from the mbox.
*/
class subscription_bind_t
{
	public:
		inline
		subscription_bind_t(
			//! Agent to subscribe.
			agent_t & agent,
			//! Mbox for messages to be subscribed.
			const mbox_t & mbox_ref );

		//! Set up a state in which events are allowed be processed.
		inline subscription_bind_t &
		in(
			//! State in which events are allowed.
			const state_t & state );

		//! Make subscription to the message.
		/*!
		 * \note Since v.5.3.0 could be used for event handlers and service handlers.
		 *
		 * \note This method supports event-methods which receive
		 * message or signal information via event_data_t object.
		 *
		 * \par Usage example
		 * \code
			struct engine_control : public so_5::message_t { ... };
			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self().event( &engine_controller::evt_control );
					...
				}
				...
			private :
				void evt_control( const so_5::event_data_t< engine_control > & cmd )
				{
					...
				}
			};
		 * \endcode
		 */
		template< class RESULT, class MESSAGE, class AGENT >
		subscription_bind_t &
		event(
			//! Event handling method.
			RESULT (AGENT::*pfn)( const event_data_t< MESSAGE > & ),
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		//! Make subscription to the message.
		/*!
		 * \note Since v.5.3.0 could be used for event handlers and service
		 * handlers.
		 *
		 * \note This method supports event-methods for messages only.
		 * Message object is passed to event-method directly, without
		 * event_data_t wrapper.
		 *
		 * \par Usage example
		 * \code
			struct engine_control : public so_5::message_t { ... };
			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self().event( &engine_controller::evt_control );
					...
				}
				...
			private :
				void evt_control( const engine_control & cmd )
				{
					...
				}
			};
		 * \endcode
		 */
		template< class RESULT, class MESSAGE, class AGENT >
		subscription_bind_t &
		event(
			//! Event handling method.
			RESULT (AGENT::*pfn)( const MESSAGE & ),
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		//! Make subscription to the message.
		/*!
		 * \since v.5.5.9
		 *
		 * \note This method is intended to use with messages whose types
		 * are not derived from so_5::message_t. Message content is
		 * passed to event handler by copy. It can be costly if message is
		 * a heavy object.
		 *
		 * \note This method supports event-methods for messages only.
		 * Message object is passed to event-method directly, without
		 * event_data_t wrapper.
		 *
		 * \par Usage example
		 * \code
			enum class engine_control { turn_on, turn_off, slow_down };
			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self().event( &engine_controller::evt_control );
					...
				}
				...
			private :
				void evt_control( engine_control cmd )
				{
					...
				}
			};
		 * \endcode
		 */
		template< class RESULT, class MESSAGE, class AGENT >
		subscription_bind_t &
		event(
			//! Event handling method.
			RESULT (AGENT::*pfn)( MESSAGE ),
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		/*!
		 * \since v.5.3.0
		 * \brief Make subscription to the signal.
		 *
		 * \note This method supports event-methods for signals only.
		 *
		 * \par Usage example
		 * \code
			struct turn_on : public so_5::signal_t {};
			struct turn_off : public so_5::signal_t {};
			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self()
						.event( so_5::signal< turn_on >, &engine_controller::evt_turn_on )
						.event( so_5::signal< turn_off >, &engine_controller::evt_turn_off );
					...
				}
				...
			private :
				void evt_turn_on() { ... }
				void evt_turn_off() { ... }
			};
		 * \endcode
		 *
		 * \attention There is a more convient form of event() for subscription
		 * of signal handlers:
		 * \code
			so_subscribe_self().event< turn_on >( &engine_controller::evt_turn_on );
		 * \endcode
		 */
		template< class RESULT, class MESSAGE, class AGENT >
		subscription_bind_t &
		event(
			//! Signal indicator.
			signal_indicator_t< MESSAGE >(),
			//! Event handling method.
			RESULT (AGENT::*pfn)(),
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		/*!
		 * \since v.5.3.0
		 * \brief Make subscription to the message by lambda-function.
		 *
		 * \attention Only lambda-function in the forms:
		 * \code
			RESULT (const MESSAGE &)
			RESULT (MESSAGE)
		 * \endcode
		 * are supported.
		 *
		 * \note This method supports event-lambdas for messages only.
		 * Message object is passed to the lambda directly, without
		 * event_data_t wrapper.
		 * 
		 * \par Usage example.
		 * \code
			enum class engine_control { turn_on, turn_off, slow_down };
			struct setup_params : public so_5::message_t { ... };
			struct update_settings { ... };

			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self()
						.event( [this]( engine_control evt ) {...} )
						.event( [this]( const setup_params & evt ) {...} )
						.event( [this]( const update_settings & evt ) {...} )
					...
				}
				...
			};
		 * \endcode
		 */
		template< class LAMBDA >
		subscription_bind_t &
		event(
			//! Event handler code.
			LAMBDA && lambda,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		/*!
		 * \since v.5.3.0
		 * \brief Make subscription to the signal by lambda-function.
		 *
		 * \attention Only lambda-function in the form:
		 * \code
			RESULT ()
		 * \endcode
		 * is supported.
		 *
		 * \note This method supports event-lambdas for signals only.
		 *
		 * \par Usage example
		 * \code
			struct turn_on : public so_5::signal_t {};
			struct turn_off : public so_5::signal_t {};
			class engine_controller : public so_5::agent_t
			{
			public :
				virtual void so_define_agent() override
				{
					so_subscribe_self()
						.event( so_5::signal< turn_on >, [this]{ ... } )
						.event( so_5::signal< turn_off >, [this]{ ... } );
					...
				}
				...
			};
		 * \endcode
		 *
		 * \attention There is a more convient form of event() for subscription
		 * of signal handlers:
		 * \code
			so_subscribe_self().event< turn_on >( [this]{...} );
		 * \endcode
		 */
		template< class MESSAGE, class LAMBDA >
		subscription_bind_t &
		event(
			//! Signal indicator.
			signal_indicator_t< MESSAGE >(),
			//! Event handling lambda.
			LAMBDA && lambda,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety = not_thread_safe );

		/*!
		 * \since v.5.5.1
		 * \brief Make subscription to the signal.
		 *
		 * \par Usage sample:
		 * \code
		   virtual void my_agent::so_define_agent()
		   {
		   	so_subscribe_self().event< msg_my_signal >( &my_agent::event );
		   	so_subscribe_self().event< msg_another_signal( [=] { ... } );
		   }
		 * \endcode
		 */
		template< typename SIGNAL, typename... ARGS >
		subscription_bind_t &
		event( ARGS&&... args )
			{
				return this->event( so_5::signal< SIGNAL >,
						std::forward< ARGS >(args)... );
			}

	private:
		//! Agent to which we are subscribing.
		agent_t * m_agent;
		//! Mbox for messages to subscribe.
		mbox_t m_mbox_ref;

		/*!
		 * \since v.5.3.0
		 * \brief Type of vector of states.
		 */
		typedef std::vector< const state_t * > state_vector_t;

		/*!
		 * \since v.5.3.0
		 * \brief States of agents the event to be subscribed in.
		 */
		state_vector_t m_states;

		/*!
		 * \since v.5.3.0
		 * \brief Create subscription of event for all states.
		 */
		void
		create_subscription_for_states(
			const std::type_index & msg_type,
			const event_handler_method_t & method,
			thread_safety_t thread_safety ) const;

		/*!
		 * \since v.5.5.9
		 * \brief Common implementation of subscription for method
		 * of agent's class.
		 */
		template< class RESULT, class MESSAGE, class AGENT, class METHOD >
		subscription_bind_t &
		event_impl(
			//! Event handling method.
			METHOD pfn,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety );
};

//
// agent_t
//

//! A base class for agents.
/*!
	An agent in SObjctizer must be derived from the agent_t.

	The base class provides various methods whose can be splitted into
	the following groups:
	\li methods for the interaction with SObjectizer;
	\li predefined hook-methods which are called during: cooperation
	registration, starting and stopping of an agent;
	\li methods for the message subscription and unsubscription;
	\li methods for working with an agent state;

	<b>Methods for the interaction with SObjectizer</b>

	Method so_5::agent_t::so_environment() serves for the access to the 
	SObjectizer Environment (and, therefore, to all methods of the 
	SObjectizer Environment).
	This method could be called immediatelly after the agent creation.
	This is because agent is bound to the SObjectizer Environment during
	the creation process.

	<b>Hook methods</b>

	The base class defines several hook-methods. Its default implementation
	do nothing.

	The method agent_t::so_define_agent() is called just before agent will
	started by SObjectizer as a part of the agent registration process.
	It should be reimplemented for the initial subscription of the agent
	to messages.

	There are two hook-methods related to important agent's lifetime events:
	agent_t::so_evt_start() and agent_t::so_evt_finish(). They are called
	by SObjectizer in next circumstances:
	- method so_evt_start() is called when the agent is starting its work
	  inside of SObjectizer. At that moment all agents are defined (all 
	  their methods agent_t::so_define_agent() have executed);
	- method so_evt_finish() is called during the agent's cooperation
	  deregistration just after agent processed the last pending event.

	Methods so_evt_start() and so_evt_finish() are called by SObjectizer and
	user can just reimplement them to implement the agent-specific logic.

	<b>Message subscription and unsubscription methods</b>

	Any method with the following prototype can be used as an event
	handler:
	\code
		void
		evt_handler(
			const so_5::event_data_t< MESSAGE > & msg );
	\endcode
	Where \c evt_handler is a name of the event handler, \c MESSAGE is a 
	message type.

	The class so_5::event_data_t is a wrapper on pointer to an instance 
	of the \c MESSAGE. It is very similar to <tt>std::unique_ptr</tt>. 
	The pointer to \c MESSAGE can be a nullptr. It happens in case when 
	the message has no actual data and servers just a signal about something.

	Since v.5.3.0 there is also support for two additional forms of
	event handlers:
	\code
		void
		evt_handler( const MESSAGE & msg );
	\endcode
	This form is used for ordinary messages with some data inside.

	This form is used only for signals (messages without actual data):
	\code
		void
		evt_handler();
	\endcode

	A subscription to the message is performed by the method so_subscribe().
	This method returns an instance of the so_5::subscription_bind_t which
	does all actual actions of the subscription process. This instance already
	knows agents and message mbox and uses the default agent state for
	the event subscription (binding to different state is also possible). 

	<b>Methods for working with an agent state</b>

	The agent can change its state by his so_change_state() method.

	An attempt to switch an agent to the state which belongs to the another
	agent is an error. If state is belong to the same agent there are
	no possibility to any run-time errors. In this case changing agent
	state is a very safe operation.

	In some cases it is necessary to detect agent state switching.
	For example for application monitoring purposes. This can be done
	by "state listeners".

	Any count of state listeners can be set for an agent. There are
	two methods for that:
	- so_add_nondestroyable_listener() is for listeners whose lifetime
	  are controlled by a programmer, not by SObjectizer;
	- so_add_destroyable_listener() is for listeners whose lifetime
	  must be controlled by agent itself.

	<b>Working thread identification</b>

	Since v.5.4.0 some operations for agent are enabled only on agent's
	working thread. They are:
	- subscription management operations (creation or dropping);
	- changing agent's state.

	Working thread for an agent is defined as follows:
	- before invocation of so_define_agent() the working thread is a
	  thread on which agent is created (id of that thread is detected in
	  agent's constructor);
	- during cooperation registration the working thread is a thread on
	  which so_environment::register_coop() is working;
	- after successful agent registration the working thread for it is
	  specified by the dispatcher.

	\note Some dispatchers could provide several working threads for
	an agent. In such case there would not be working thread id. And
	operations like changing agent state or creation of subscription
	would be prohibited after agent registration.
*/
class SO_5_TYPE agent_t
	:	private atomic_refcounted_t
	,	public message_limit::message_limit_methods_mixin_t
{
		friend class subscription_bind_t;
		friend class intrusive_ptr_t< agent_t >;
		friend class coop_t;
		friend class state_t;

		friend class so_5::impl::mpsc_mbox_t;

	public:
		/*!
		 * \since v.5.5.4
		 * \brief Short alias for agent_context.
		 */
		using context_t = so_5::agent_context_t;

		//! Constructor.
		/*!
			Agent must be bound to the SObjectizer Environment during
			its creation. And that binding cannot be changed anymore.
		*/
		explicit agent_t(
			//! The Environment for this agent must exist.
			environment_t & env );

		/*!
		 * \since v.5.5.3
		 * \brief Constructor which allows specification of
		 * agent's tuning options.
		 *
		 * \par Usage sample:
		 \code
		 using namespace so_5;
		 class my_agent : public agent_t
		 {
		 public :
		 	my_agent( environment_t & env )
				:	agent_t( env, agent_t::tuning_options()
						.subscription_storage_factory(
								vector_based_subscription_storage_factory() ) )
				{...}
		 }
		 \endcode
		 */
		agent_t(
			environment_t & env,
			agent_tuning_options_t tuning_options );

		/*!
		 * \since v.5.5.4
		 * \brief Constructor which simplifies agent construction with
		 * or without agent's tuning options.
		 *
		 * \par Usage sample:
		 * \code
		 class my_agent : public so_5::agent_t
		 {
		 public :
		 	my_agent( context_t ctx )
				:	so_5::agent( ctx + limit_then_drop< get_status >(1) )
				{}
			...
		 };
		 class my_more_specific_agent : public my_agent
		 {
		 public :
		 	my_more_specific_agent( context_t ctx )
				:	my_agent( ctx + limit_then_drop< reconfigure >(1) )
				{}
		 };

		 // Then somewhere in the code:
		 auto coop = env.create_coop( so_5::autoname );
		 auto a = coop->make_agent< my_agent >();
		 auto b = coop->make_agent< my_more_specific_agent >();
		 * \endcode
		 */
		agent_t( context_t ctx );

		virtual ~agent_t();

		//! Get the raw pointer of itself.
		/*!
			This method is intended for use in the member initialization
			list instead 'this' to suppres compiler warnings.
			For example for an agent state initialization:
			\code
			class a_sample_t : public so_5::agent_t
			{
					typedef so_5::agent_t base_type_t;

					// Agent state.
					const so_5::state_t m_sample_state;
				public:
					a_sample_t( so_5::environment_t & env )
						:
							base_type_t( env ),
							m_sample_state( self_ptr() )
					{
						// ...
					}

				// ...

			};
			\endcode
		*/
		inline const agent_t *
		self_ptr() const
		{
			return this;
		}

		inline agent_t *
		self_ptr()
		{
			return this;
		}

		//! Hook on agent start inside SObjectizer.
		/*!
			It is guaranteed that this method will be called first
			just after end of the cooperation registration process.

			During cooperation registration agent is bound to some
			working thread. And the first method which is called for
			the agent on that working thread context is this method.

			\code
			class a_sample_t : public so_5::agent_t
			{
				// ...
				virtual void
				so_evt_start();
				// ...
			};

			a_sample_t::so_evt_start()
			{
				std::cout << "first agent action on bound dispatcher" << std::endl;
				... // Some application logic actions.
			}
			\endcode
		*/
		virtual void
		so_evt_start();

		//! Hook of agent finish in SObjectizer.
		/*!
			It is guaranteed that this method will be called last
			just before deattaching agent from it's working thread.

			This method should be used to perform some cleanup
			actions on it's working thread.
			\code
			class a_sample_t : public so_5::agent_t
			{
				// ...
				virtual void
				so_evt_finish();
				// ...
			};

			a_sample_t::so_evt_finish()
			{
				std::cout << "last agent activity";

				if( so_current_state() == m_db_error_happened )
				{
					// Delete the DB connection on the same thread where
					// connection was established and where some
					// error happened.
					m_db.release();
				}
			}
			\endcode
		*/
		virtual void
		so_evt_finish();

		//! Access to the current agent state.
		inline const state_t &
		so_current_state() const
		{
			return *m_current_state_ptr;
		}

		//! Name of the agent's cooperation.
		/*!
		 * \note It is safe to use this method when agent is working inside 
		 * SObjectizer because agent can be registered only in some
		 * cooperation. When agent is not registered in SObjectizer this
		 * method should be used carefully.
		 *
		 * \throw so_5::exception_t If the agent doesn't belong to any cooperation.
		 *
		 * \return Cooperation name if the agent is bound to the cooperation.
		 */
		const std::string &
		so_coop_name() const;

		//! Add a state listener to the agent.
		/*!
		 * A programmer should guarantee that the lifetime of
		 * \a state_listener is exceeds lifetime of the agent.
		 */
		void
		so_add_nondestroyable_listener(
			agent_state_listener_t & state_listener );

		//! Add a state listener to the agent.
		/*!
		 * Agent takes care of the \a state_listener destruction.
		 */
		void
		so_add_destroyable_listener(
			agent_state_listener_unique_ptr_t state_listener );

		/*!
		 * \since v.5.2.3.
		 * \brief A reaction from SObjectizer to an exception from
		 * agent's event.
		 *
		 * If an exception is going out from agent's event it will be
		 * caught by SObjectizer. Then SObjectizer will call this method
		 * and perform some actions in dependence of return value.
		 *
		 * \note Since v.5.3.0 default implementation calls
		 * coop_t::exception_reaction() for agent's cooperation
		 * object.
		 */
		virtual exception_reaction_t
		so_exception_reaction() const;

		/*!
		 * \since 5.2.3
		 * \brief Switching agent to special state in case of unhandled
		 * exception.
		 */
		void
		so_switch_to_awaiting_deregistration_state();

		//! Push an event to the agent's event queue.
		/*!
			This method is used by SObjectizer for the 
			agent's event scheduling.
		*/
		static inline void
		call_push_event(
			agent_t & agent,
			const message_limit::control_block_t * limit,
			mbox_id_t mbox_id,
			std::type_index msg_type,
			const message_ref_t & message )
		{
			agent.push_event( limit, mbox_id, msg_type, message );
		}

		/*!
		 * \since v.5.3.0
		 * \brief Push service request to the agent's event queue.
		 */
		static inline void
		call_push_service_request(
			agent_t & agent,
			const message_limit::control_block_t * limit,
			mbox_id_t mbox_id,
			std::type_index msg_type,
			const message_ref_t & message )
		{
			agent.push_service_request( limit, mbox_id, msg_type, message );
		}

		/*!
		 * \since v.5.4.0
		 * \brief Get the agent's direct mbox.
		 */
		const mbox_t &
		so_direct_mbox() const;

		/*!
		 * \since v.5.5.3
		 * \brief Create tuning options object with default values.
		 */
		inline static agent_tuning_options_t
		tuning_options()
		{
			return agent_tuning_options_t();
		}

	protected:
		/*!
		 * \name Methods for working with the agent state.
		 * \{
		 */

		//! Access to the agent's default state.
		const state_t &
		so_default_state() const;

	public : /* Note: since v.5.5.1 method so_change_state() is public */

		//! Method changes state.
		/*!
			Usage sample:
			\code
			void
			a_sample_t::evt_smth(
				const so_5::event_data_t< message_one_t > & msg )
			{
				// If something wrong with the message then we should
				// switch to the error_state.
				if( error_in_data( *msg ) )
					so_change_state( m_error_state );
			}
			\endcode
		*/
		void
		so_change_state(
			//! New agent state.
			const state_t & new_state );
		/*!
		 * \}
		 */

	public : /* Note: since v.5.2.3.2 subscription-related method are
					made public. */

		/*!
		 * \name Subscription methods.
		 * \{
		 */

		//! Initiate subscription.
		/*!
			Usage sample:
			\code
			void
			a_sample_t::so_define_agent() override
			{
				so_subscribe( m_mbox_target )
					.in( m_state_one )
						.event( &a_sample_t::evt_sample_handler );
			}
			\endcode
		*/
		inline subscription_bind_t
		so_subscribe(
			//! Mbox for messages to subscribe.
			const mbox_t & mbox_ref )
		{
			return subscription_bind_t( *this, mbox_ref );
		}

		/*!
		 * \since v.5.5.1
		 * \brief Initiate subscription to agent's direct mbox.
		 *
		 * \par Usage sample:
			\code
			void a_sample_t::so_define_agent() override
			{
				so_subscribe_self().in( m_state_one ).event( ... );
				so_subscribe_self().in( m_state_two ).event( ... );
			}
			\endcode
		 */
		inline subscription_bind_t
		so_subscribe_self()
		{
			return so_subscribe( so_direct_mbox() );
		}

		/*!
		 * \since v.5.2.3
		 * \brief Drop subscription for the state specified.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 *
		 * \note Subscription is removed even if agent was subscribed
		 * for this message type with different method pointer.
		 * The pointer to event routine is necessary only to
		 * detect MSG type.
		 */
		template< class AGENT, class MESSAGE >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			const state_t & target_state,
			void (AGENT::*)( const event_data_t< MESSAGE > & ) )
		{
			ensure_classical_message< MESSAGE >();

			do_drop_subscription( mbox,
					message_payload_type< MESSAGE >::payload_type_index(),
					target_state );
		}

		/*!
		 * \since v.5.4.0
		 * \brief Drop subscription for the state specified.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 *
		 * \note Subscription is removed even if agent was subscribed
		 * for this message type with different method pointer.
		 * The pointer to event routine is necessary only to
		 * detect MSG type.
		 */
		template< class AGENT, class MESSAGE >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			const state_t & target_state,
			void (AGENT::*)( const MESSAGE & ) )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index(),
					target_state );
		}

		/*!
		 * \since v.5.4.0
		 * \brief Drop subscription for the state specified.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 */
		template< class MESSAGE >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			const state_t & target_state,
			signal_indicator_t< MESSAGE >() )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index(),
					target_state );
		}

		/*!
		 * \since v.5.5.3
		 * \brief Drop subscription for the state specified.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 */
		template< class MESSAGE >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			const state_t & target_state )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index(),
					target_state );
		}

		/*!
		 * \since v.5.2.3
		 * \brief Drop subscription for the default agent state.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 *
		 * \note Subscription is removed even if agent was subscribed
		 * for this message type with different method pointer.
		 * The pointer to event routine is necessary only to
		 * detect MSG type.
		 */
		template< class AGENT, class MESSAGE >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			void (AGENT::*)( const event_data_t< MESSAGE > & ) )
		{
			ensure_classical_message< MESSAGE >();

			do_drop_subscription(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index(),
					so_default_state() );
		}

		/*!
		 * \since v.5.4.0
		 * \brief Drop subscription for the default agent state.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 *
		 * \note Subscription is removed even if agent was subscribed
		 * for this message type with different method pointer.
		 * The pointer to event routine is necessary only to
		 * detect MSG type.
		 */
		template< class AGENT, class MESSAGE >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			void (AGENT::*)( const MESSAGE & ) )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index(),
					so_default_state() );
		}

		/*!
		 * \since v.5.4.0
		 * \brief Drop subscription for the default agent state.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 */
		template< class MESSAGE >
		inline void
		so_drop_subscription(
			const mbox_t & mbox,
			signal_indicator_t< MESSAGE >() )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index(),
					so_default_state() );
		}

		/*!
		 * \since v.5.5.3
		 * \brief Drop subscription for the default agent state.
		 *
		 * \note Doesn't throw if there is no such subscription.
		 */
		template< class MESSAGE >
		inline void
		so_drop_subscription(
			const mbox_t & mbox )
		{
			do_drop_subscription(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index(),
					so_default_state() );
		}

		/*!
		 * \since v.5.2.3
		 * \brief Drop subscription for all states.
		 *
		 * \note Doesn't throw if there is no any subscription for
		 * that mbox and message type.
		 *
		 * \note Subscription is removed even if agent was subscribed
		 * for this message type with different method pointer.
		 * The pointer to event routine is necessary only to
		 * detect MSG type.
		 */
		template< class AGENT, class MESSAGE >
		inline void
		so_drop_subscription_for_all_states(
			const mbox_t & mbox,
			void (AGENT::*)( const event_data_t< MESSAGE > & ) )
		{
			ensure_classical_message< MESSAGE >();

			do_drop_subscription_for_all_states(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index() );
		}

		/*!
		 * \since v.5.4.0
		 * \brief Drop subscription for all states.
		 *
		 * \note Doesn't throw if there is no any subscription for
		 * that mbox and message type.
		 *
		 * \note Subscription is removed even if agent was subscribed
		 * for this message type with different method pointer.
		 * The pointer to event routine is necessary only to
		 * detect MSG type.
		 */
		template< class AGENT, class MESSAGE >
		inline void
		so_drop_subscription_for_all_states(
			const mbox_t & mbox,
			void (AGENT::*pfn)( const MESSAGE & ) )
		{
			do_drop_subscription_for_all_states(
					mbox, 
					message_payload_type< MESSAGE >::payload_type_index() );
		}

		/*!
		 * \since v.5.4.0
		 * \brief Drop subscription for all states.
		 *
		 * \note Doesn't throw if there is no any subscription for
		 * that mbox and message type.
		 */
		template< class MESSAGE >
		inline void
		so_drop_subscription_for_all_states(
			const mbox_t & mbox,
			signal_indicator_t< MESSAGE >() )
		{
			do_drop_subscription_for_all_states(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index() );
		}

		/*!
		 * \since v.5.5.3
		 * \brief Drop subscription for all states.
		 *
		 * \note Doesn't throw if there is no any subscription for
		 * that mbox and message type.
		 */
		template< class MESSAGE >
		inline void
		so_drop_subscription_for_all_states(
			const mbox_t & mbox )
		{
			do_drop_subscription_for_all_states(
					mbox,
					message_payload_type< MESSAGE >::payload_type_index() );
		}
		/*!
		 * \}
		 */

	protected :

		/*!
		 * \name Agent initialization methods.
		 * \{
		 */
		/*!
		 * \since v.5.4.0
		 * \brief A correct initiation of so_define_agent method call.
		 *
		 * Before the actual so_define_agent() method it is necessary
		 * to temporary set working thread id. And then drop this id
		 * to non-actual value after so_define_agent() return.
		 *
		 * Because of that this method must be called during cooperation
		 * registration procedure instead of direct call of so_define_agent().
		 */
		void
		so_initiate_agent_definition();

		//! Hook on define agent for SObjectizer.
		/*!
			This method is called by SObjectizer during the cooperation
			registration process before agent will be bound to its
			working thread.

			Should be used by the agent to make necessary message subscriptions.

			Usage sample;
			\code
			class a_sample_t : public so_5::agent_t
			{
				// ...
				virtual void
				so_define_agent();

				void
				evt_handler_1(
					const so_5::event_data_t< message1_t > & msg );
				// ...
				void
				evt_handler_N(
					const so_5::event_data_t< messageN_t > & msg );

			};

			void
			a_sample_t::so_define_agent()
			{
				// Make subscriptions...
				so_subscribe( m_mbox1 )
					.in( m_state_1 )
						.event( &a_sample_t::evt_handler_1 );
				// ...
				so_subscribe( m_mboxN )
					.in( m_state_N )
						.event( &a_sample_t::evt_handler_N );
			}
			\endcode
		*/
		virtual void
		so_define_agent();

		//! Is method define_agent already called?
		/*!
			Usage sample:
			\code
			class a_sample_t : public so_5::agent_t
			{
				// ...

				public:
					void
					set_target_mbox( const so_5::mbox_t & mbox )
					{
						// mbox cannot be changed after agent registration.
						if( !so_was_defined() )
						{
							m_target_mbox = mbox;
						}
					}

				private:
					so_5::mbox_t m_target_mbox;
			};
			\endcode
		*/
		bool
		so_was_defined() const;
		/*!
		 * \}
		 */

	public:
		//! Access to the SObjectizer Environment which this agent is belong.
		/*!
			Usage sample for other cooperation registration:
			\code
			void
			a_sample_t::evt_on_smth(
				const so_5::event_data_t< some_message_t > & msg )
			{
				so_5::coop_unique_ptr_t coop =
					so_environment().create_coop( "first_coop" );

				// Filling the cooperation...
				coop->make_agent< a_another_t >( ... );
				// ...

				// Registering cooperation.
				so_environment().register_coop( std::move(coop) );
			}
			\endcode

			Usage sample for the SObjectizer shutting down:
			\code
			void
			a_sample_t::evt_last_event(
				const so_5::event_data_t< message_one_t > & msg )
			{
				...
				so_environment().stop();
			}
			\endcode
		*/
		environment_t &
		so_environment() const;

		/*!
		 * \since v.5.4.0
		 * \brief Binding agent to the dispatcher.
		 *
		 * This is an actual start of agent's work in SObjectizer.
		 */
		void
		so_bind_to_dispatcher(
			//! Actual event queue for an agent.
			event_queue_t & queue );

		/*!
		 * \since v.5.4.0
		 * \brief Create execution hint for the specified demand.
		 *
		 * The hint returned is intendent for the immediately usage.
		 * It must not be stored for the long time and used sometime in
		 * the future. It is because internal state of the agent
		 * can be changed and some references from hint object to
		 * agent's internals become invalid.
		 */
		static execution_hint_t
		so_create_execution_hint(
			//! Demand for execution of event handler.
			execution_demand_t & demand );

		/*!
		 * \since v.5.4.0
		 * \brief A helper method for deregistering agent's coop.
		 */
		void
		so_deregister_agent_coop( int dereg_reason );

		/*!
		 * \since v.5.4.0
		 * \brief A helper method for deregistering agent's coop
		 * in case of normal deregistration.
		 *
		 * \note It is just a shorthand for:
			\code
			so_deregister_agent_coop( so_5::dereg_reason::normal );
			\endcode
		 */
		void
		so_deregister_agent_coop_normally();

		/*!
		 * \name Methods for dealing with message delivery filters.
		 * \{
		 */
		/*!
		 * \since v.5.5.5
		 * \brief Set a delivery filter.
		 *
		 * \tparam MESSAGE type of message to be filtered.
		 */
		template< typename MESSAGE >
		void
		so_set_delivery_filter(
			//! Message box from which message is expected.
			//! This must be MPMC-mbox.
			const mbox_t & mbox,
			//! Delivery filter instance.
			delivery_filter_unique_ptr_t filter )
			{
				ensure_not_signal< MESSAGE >();

				do_set_delivery_filter(
						mbox,
						message_payload_type< MESSAGE >::payload_type_index(),
						std::move(filter) );
			}

		/*!
		 * \since v.5.5.5
		 * \brief Set a delivery filter.
		 *
		 * \tparam LAMBDA type of lambda-function or functional object which
		 * must be used as message filter.
		 *
		 * \par Usage sample:
		 \code
		 void my_agent::so_define_agent() {
		 	so_set_delivery_filter( temp_sensor,
				[]( const current_temperature & msg ) {
					return !is_normal_temperature( msg );
				} );
			...
		 }
		 \endcode
		 */
		template< typename LAMBDA >
		void
		so_set_delivery_filter(
			//! Message box from which message is expected.
			//! This must be MPMC-mbox.
			const mbox_t & mbox,
			//! Delivery filter as lambda-function or functional object.
			LAMBDA && lambda );

		/*!
		 * \since v.5.5.5
		 * \brief Drop a delivery filter.
		 *
		 * \tparam MESSAGE type of message filtered.
		 */
		template< typename MESSAGE >
		void
		so_drop_delivery_filter(
			//! Message box to which delivery filter was set.
			//! This must be MPMC-mbox.
			const mbox_t & mbox ) SO_5_NOEXCEPT
			{
				do_drop_delivery_filter(
						mbox,
						message_payload_type< MESSAGE >::payload_type_index() );
			}
		/*!
		 * \}
		 */

		/*!
		 * \name Dealing with priority.
		 * \{
		 */
		/*!
		 * \since v.5.5.8
		 * \brief Get the priority of the agent.
		 */
		priority_t
		so_priority() const
			{
				return m_priority;
			}
		/*!
		 * \}
		 */

	protected :
		/*!
		 * \name Helpers for state object creation.
		 * \{
		 */
		/*!
		 * \since v.5.4.0
		 * \brief Helper method for creation of anonymous state object.
		 *
		 * \par Usage:
		 	\code
			class my_agent_t : public so_5::agent_t
			{
				so_5::state_t st_1 = so_make_state();
				so_5::state_t st_2 = so_make_state();
				...
			};
			\endcode
		 *
		 */
		inline state_t
		so_make_state()
		{
			return state_t( self_ptr() );
		}

		/*!
		 * \since v.5.4.0
		 * \brief Helper method for creation of named state object.
		 *
		 * \par Usage:
		 	\code
			class my_agent_t : public so_5::agent_t
			{
				so_5::state_t st_1 = so_make_state( "st_one" );
				so_5::state_t st_2 = so_make_state( "st_two" );
				...
			};
			\endcode
		 *
		 */
		inline state_t
		so_make_state( std::string name )
		{
			return state_t( self_ptr(), std::move( name ) );
		}
		/*!
		 * \}
		 */

	private:
		const state_t st_default = so_make_state( "<DEFAULT>" );

		//! Current agent state.
		const state_t * m_current_state_ptr;

		//! Agent definition flag.
		/*!
		 * Set to true after a successful return from the so_define_agent().
		 */
		bool m_was_defined;

		//! State listeners controller.
		std::unique_ptr< impl::state_listener_controller_t >
			m_state_listener_controller;

		/*!
		 * \since v.5.5.9
		 * \brief Type of function for searching event handler.
		 */
		using handler_finder_t = 
			const impl::event_handler_data_t *(*)(
					execution_demand_t & /* demand */,
					const char * /* context_marker */ );

		/*!
		 * \since v.5.5.9
		 * \brief Function for searching event handler.
		 *
		 * \note The value is set only once in the constructor and
		 * doesn't changed anymore.
		 */
		handler_finder_t m_handler_finder;

		/*!
		 * \since v.5.4.0
		 * \brief All agent's subscriptions.
		 */
		impl::subscription_storage_unique_ptr_t m_subscriptions;

		/*!
		 * \since v.5.5.4
		 * \brief Run-time information for message limits.
		 *
		 * Created only of message limits are described in agent's
		 * tuning options.
		 *
		 * \attention This attribute must be initialized before the
		 * \a m_direct_mbox attribute. It is because the value of
		 * \a m_message_limits is used in \a m_direct_mbox creation.
		 * Because of that \a m_message_limits is declared before
		 * \a m_direct_mbox.
		 */
		std::unique_ptr< message_limit::impl::info_storage_t > m_message_limits;

		//! SObjectizer Environment for which the agent is belong.
		environment_t & m_env;

		/*!
		 * \since v.5.5.8
		 * \brief Event queue operation protector.
		 *
		 * Initially m_event_queue is NULL. It is changed to actual value
		 * in so_bind_to_dispatcher() method. And reset to nullptr again
		 * in shutdown_agent().
		 *
		 * nullptr in m_event_queue means that methods push_event() and
		 * push_service_request() will throw away any new demand.
		 *
		 * It is necessary to provide guarantee that m_event_queue will
		 * be reset to nullptr in shutdown_agent() only if there is no
		 * working push_event()/push_service_request() methods. To do than
		 * default_rw_spinlock_t is used. Methods push_event() and
		 * push_service_request() acquire it in read-mode and shutdown_agent()
		 * acquires it in write-mode. It means that shutdown_agent() cannot
		 * get access to m_event_queue until there is working
		 * push_event()/push_service_request().
		 */
		default_rw_spinlock_t	m_event_queue_lock;

		/*!
		 * \since v.5.5.8
		 * \brief A pointer to event_queue.
		 *
		 * After binding to the dispatcher is it pointed to the actual
		 * event queue.
		 *
		 * After shutdown it is set to nullptr.
		 *
		 * \attention Access to m_event_queue value must be done only
		 * under acquired m_event_queue_lock.
		 */
		event_queue_t * m_event_queue;

		/*!
		 * \since v.5.4.0
		 * \brief A direct mbox for the agent.
		 */
		const mbox_t m_direct_mbox;

		/*!
		 * \since v.5.4.0
		 * \brief Working thread id.
		 *
		 * Some actions like managing subscriptions and changing states
		 * are enabled only on working thread id.
		 */
		so_5::current_thread_id_t m_working_thread_id;

		//! Agent is belong to this cooperation.
		coop_t * m_agent_coop;

		/*!
		 * \since v.5.5.5
		 * \brief Delivery filters for that agents.
		 *
		 * \note Storage is created only when necessary.
		 */
		std::unique_ptr< impl::delivery_filter_storage_t > m_delivery_filters;

		/*!
		 * \since v.5.5.8
		 * \brief Priority of the agent.
		 */
		const priority_t m_priority;

		//! Make an agent reference.
		/*!
		 * This is an internal SObjectizer method. It is called when
		 * it is guaranteed that the agent is still necessary and something
		 * has reference to it.
		 */
		agent_ref_t
		create_ref();

		/*!
		 * \name Embedding agent into the SObjectizer Run-time.
		 * \{
		 */

		//! Bind agent to the cooperation.
		/*!
		 * Initializes an internal cooperation pointer.
		 *
		 * Drops m_is_coop_deregistered to false.
		 */
		void
		bind_to_coop(
			//! Cooperation for that agent.
			coop_t & coop );

		//! Agent shutdown deriver.
		/*!
		 * \since v.5.2.3
		 *
		 * Method destroys all agent subscriptions.
		 */
		void
		shutdown_agent() SO_5_NOEXCEPT;
		/*!
		 * \}
		 */

		/*!
		 * \name Subscription/unsubscription implementation details.
		 * \{
		 */

		//! Create binding between agent and mbox.
		void
		create_event_subscription(
			//! Message's mbox.
			const mbox_t & mbox_ref,
			//! Message type.
			std::type_index type_index,
			//! State for event.
			const state_t & target_state,
			//! Event handler caller.
			const event_handler_method_t & method,
			//! Thread safety of the event handler.
			thread_safety_t thread_safety );

		/*!
		 * \since v.5.5.4
		 * \brief Detect limit for that message type.
		 *
		 * \return nullptr if message limits are not used.
		 *
		 * \throw exception_t if message limits are used but the limit
		 * for that message type is not found.
		 */
		const message_limit::control_block_t *
		detect_limit_for_message_type(
			const std::type_index & msg_type ) const;

		/*!
		 * \since v.5.2.3
		 * \brief Remove subscription for the state specified.
		 */
		void
		do_drop_subscription(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type,
			//! State for event.
			const state_t & target_state );

		/*!
		 * \since v.5.2.3
		 * \brief Remove subscription for all states.
		 */
		void
		do_drop_subscription_for_all_states(
			//! Message's mbox.
			const mbox_t & mbox,
			//! Message type.
			const std::type_index & msg_type );
		/*!
		 * \}
		 */

		/*!
		 * \name Event handling implementation details.
		 * \{
		 */

		//! Push event into the event queue.
		void
		push_event(
			//! Optional message limit.
			const message_limit::control_block_t * limit,
			//! ID of mbox for this event.
			mbox_id_t mbox_id,
			//! Message type for event.
			std::type_index msg_type,
			//! Event message.
			const message_ref_t & message );

		/*!
		 * \since v.5.3.0
		 * \brief Push service request to event queue.
		 */
		void
		push_service_request(
			//! Optional message limit.
			const message_limit::control_block_t * limit,
			//! ID of mbox for this event.
			mbox_id_t mbox_id,
			//! Message type for event.
			std::type_index msg_type,
			//! Event message.
			const message_ref_t & message );
		/*!
		 * \}
		 */

		// NOTE: demand handlers declared as public to allow
		// access this handlers from unit-tests.
	public :
		/*!
		 * \name Demand handlers.
		 * \{
		 */
		/*!
		 * \since v.5.2.0
		 * \brief Calls so_evt_start method for agent.
		 */
		static void
		demand_handler_on_start(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since v.5.5.8
		 * \brief Ensures that all agents from cooperation are
		 * bound to dispatchers.
		 */
		void
		ensure_binding_finished();

		/*!
		 * \since v.5.4.0
		 * \note This method is necessary for GCC on Cygwin.
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_start_ptr();

		/*!
		 * \since v.5.2.0
		 * \brief Calls so_evt_finish method for agent.
		 */
		static void
		demand_handler_on_finish(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since v.5.4.0
		 * \note This method is necessary for GCC on Cygwin.
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_finish_ptr();

		/*!
		 * \since v.5.2.0
		 * \brief Calls event handler for message.
		 */
		static void
		demand_handler_on_message(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since v.5.4.0
		 * \note This method is necessary for GCC on Cygwin.
		 */
		static demand_handler_pfn_t
		get_demand_handler_on_message_ptr();

		/*!
		 * \since v.5.3.0
		 * \brief Calls request handler for message.
		 */
		static void
		service_request_handler_on_message(
			current_thread_id_t working_thread_id,
			execution_demand_t & d );

		/*!
		 * \since v.5.4.0
		 * \note This method is necessary for GCC on Cygwin.
		 */
		static demand_handler_pfn_t
		get_service_request_handler_on_message_ptr();

		/*!
		 * \}
		 */

	private :
		/*!
		 * \since v.5.4.0
		 * \brief Actual implementation of message handling.
		 */
		static void
		process_message(
			current_thread_id_t working_thread_id,
			execution_demand_t & d,
			const event_handler_method_t & method );
		/*!
		 * \since v.5.4.0
		 * \brief Actual implementation of service request handling.
		 *
		 * \note handler_data.first == true only if handler_data.second is an
		 * actual result of searching handler for the message. If
		 * handler_data.first == second then it is necessary to search event
		 * handler for the message.
		 */
		static void
		process_service_request(
			current_thread_id_t working_thread_id,
			execution_demand_t & d,
			std::pair< bool, const impl::event_handler_data_t * > handler_data );

		/*!
		 * \since v.5.4.0
		 * \brief Enables operation only if it is performed on agent's
		 * working thread.
		 */
		void
		ensure_operation_is_on_working_thread(
			const char * operation_name ) const;

		/*!
		 * \since v.5.5.0
		 * \brief Drops all delivery filters.
		 */
		void
		drop_all_delivery_filters() SO_5_NOEXCEPT;

		/*!
		 * \since v.5.5.5
		 * \brief Set a delivery filter.
		 */
		void
		do_set_delivery_filter(
			const mbox_t & mbox,
			const std::type_index & msg_type,
			delivery_filter_unique_ptr_t filter );

		/*!
		 * \since v.5.5.5
		 * \brief Drop a delivery filter.
		 */
		void
		do_drop_delivery_filter(
			const mbox_t & mbox,
			const std::type_index & msg_type ) SO_5_NOEXCEPT;

		/*!
		 * \since v.5.5.9
		 * \brief Handler finder for the case when message delivery
		 * tracing is disabled.
		 */
		static const impl::event_handler_data_t *
		handler_finder_msg_tracing_disabled(
			execution_demand_t & demand,
			const char * context_marker );

		/*!
		 * \since v.5.5.9
		 * \brief Handler finder for the case when message delivery
		 * tracing is enabled.
		 */
		static const impl::event_handler_data_t *
		handler_finder_msg_tracing_enabled(
			execution_demand_t & demand,
			const char * context_marker );
};

/*!
 * \since v.5.5.5
 * \brief Template-based implementations of delivery filters.
 */
namespace delivery_filter_templates
{

/*!
 * \since v.5.5.5
 * \brief An implementation of delivery filter represented by lambda-function
 * like object.
 *
 * \tparam LAMBDA type of lambda-function or functional object.
 */
template< typename LAMBDA, typename MESSAGE >
class lambda_as_filter_t : public delivery_filter_t
	{
		LAMBDA m_filter;

	public :
		lambda_as_filter_t( LAMBDA && filter )
			:	m_filter( std::forward< LAMBDA >( filter ) )
			{}

		virtual bool
		check(
			const agent_t & receiver,
			const message_t & msg ) const SO_5_NOEXCEPT override
			{
				return so_5::details::do_with_rollback_on_exception(
					[&] {
						return m_filter( message_payload_type< MESSAGE >::payload_reference( msg ) );
					},
					[&] {
						so_5::details::abort_on_fatal_error( [&] {
							SO_5_LOG_ERROR( receiver.so_environment(), serr ) {
								serr << "An exception from delivery filter "
									"for message type "
									<< message_payload_type< MESSAGE >::payload_type_index().name()
									<< ". Application will be aborted"
									<< std::endl;
							}
						} );
					} );
			}
	};

} /* namespace delivery_filter_templates */

template< typename LAMBDA >
void
agent_t::so_set_delivery_filter(
	const mbox_t & mbox,
	LAMBDA && lambda )
	{
		using namespace so_5::details::lambda_traits;
		using namespace delivery_filter_templates;

		using argument_type = typename argument_type_if_lambda< LAMBDA >::type;

		ensure_not_signal< argument_type >();

		do_set_delivery_filter(
				mbox,
				message_payload_type< argument_type >::payload_type_index(),
				delivery_filter_unique_ptr_t{ 
					new lambda_as_filter_t< LAMBDA, argument_type >(
							std::move( lambda ) )
				} );
	}

//
// subscription_bind_t implementation
//
inline
subscription_bind_t::subscription_bind_t(
	agent_t & agent,
	const mbox_t & mbox_ref )
	:	m_agent( &agent )
	,	m_mbox_ref( mbox_ref )
{
}

inline subscription_bind_t &
subscription_bind_t::in(
	const state_t & state )
{
	if( !state.is_target( m_agent ) )
	{
		SO_5_THROW_EXCEPTION(
			rc_agent_is_not_the_state_owner,
			"agent doesn't own the state" );
	}

	m_states.push_back( &state );

	return *this;
}

template< class RESULT, class MESSAGE, class AGENT >
inline subscription_bind_t &
subscription_bind_t::event(
	RESULT (AGENT::*pfn)( const event_data_t< MESSAGE > & ),
	thread_safety_t thread_safety )
{
	/*!
	 * \note This method uses special trick to allow to receve and
	 * resend instances of user_type_message_t<T>. To do that
	 * a message_payload_type<T>::extract_envelope_ptr() is used,
	 * but subscription is made for
	 * message_payload_type<T>::payload_type.
	 * This trick works because in case when MESSAGE is not
	 * user_type_message_t<T> then payload_type and envelope_type
	 * will be the same type (type MESSAGE).
	 */

	ensure_classical_message< MESSAGE >();

	using namespace details::event_subscription_helpers;

	// Agent must have right type.
	auto cast_result = get_actual_agent_pointer< AGENT >( *m_agent );

	auto method = [cast_result,pfn](
			invocation_type_t invocation_type,
			message_ref_t & message_ref)
		{
			if( invocation_type_t::service_request == invocation_type )
				{
					using namespace details::promise_result_setting_details;

					auto actual_request_ptr =
							get_actual_service_request_pointer< RESULT, MESSAGE >(
									message_ref );

					const event_data_t< MESSAGE > event_data{
							// Extraction of whole envelope, not just a payload.
							message_payload_type< MESSAGE >::extract_envelope_ptr(
									actual_request_ptr->m_param ) };

					// All exceptions will be processed in service_handler_on_message.
					result_setter_t< RESULT >().call_old_format_event_and_set_result(
							actual_request_ptr->m_promise,
							cast_result,
							pfn,
							event_data );
				}
			else
				{
					const event_data_t< MESSAGE > event_data{
							// Extraction of whole envelope, not just a payload.
							message_payload_type< MESSAGE >::extract_envelope_ptr(
									message_ref ) };

					(cast_result->*pfn)( event_data );
				}
		};

	create_subscription_for_states( 
			message_payload_type< MESSAGE >::payload_type_index(),
			method,
			thread_safety );

	return *this;
}

template< class RESULT, class MESSAGE, class AGENT >
inline subscription_bind_t &
subscription_bind_t::event(
	RESULT (AGENT::*pfn)( const MESSAGE & ),
	thread_safety_t thread_safety )
{
	return this->event_impl< RESULT, MESSAGE, AGENT, decltype(pfn) >( pfn, thread_safety );
}

template< class RESULT, class MESSAGE, class AGENT >
inline subscription_bind_t &
subscription_bind_t::event(
	RESULT (AGENT::*pfn)( MESSAGE ),
	thread_safety_t thread_safety )
{
	return this->event_impl< RESULT, MESSAGE, AGENT, decltype(pfn) >( pfn, thread_safety );
}

template< class RESULT, class MESSAGE, class AGENT >
inline subscription_bind_t &
subscription_bind_t::event(
	signal_indicator_t< MESSAGE >(),
	RESULT (AGENT::*pfn)(),
	thread_safety_t thread_safety )
{
	ensure_signal< MESSAGE >();

	using namespace details::event_subscription_helpers;

	// Agent must have right type.
	auto cast_result = get_actual_agent_pointer< AGENT >( *m_agent );

	auto method = [cast_result,pfn](
			invocation_type_t invocation_type,
			message_ref_t & message_ref)
		{
			if( invocation_type_t::service_request == invocation_type )
				{
					using namespace details::promise_result_setting_details;

					auto actual_request_ptr =
							get_actual_service_request_pointer< RESULT, MESSAGE >(
									message_ref );

					// All exceptions will be processed in service_handler_on_message.
					result_setter_t< RESULT >().call_new_format_signal_and_set_result(
							actual_request_ptr->m_promise,
							cast_result,
							pfn );
				}
			else
				{
					(cast_result->*pfn)();
				}
		};

	create_subscription_for_states(
			message_payload_type< MESSAGE >::payload_type_index(),
			method,
			thread_safety );

	return *this;
}

template< class LAMBDA >
inline subscription_bind_t &
subscription_bind_t::event(
	LAMBDA && lambda,
	thread_safety_t thread_safety )
{
	auto ev = handler( std::forward<LAMBDA>(lambda) );

	create_subscription_for_states(
			ev.m_msg_type,
			ev.m_handler,
			thread_safety );

	return *this;
}

template< class MESSAGE, class LAMBDA >
inline subscription_bind_t &
subscription_bind_t::event(
	signal_indicator_t< MESSAGE > (*)(),
	LAMBDA && lambda,
	thread_safety_t thread_safety )
{
	auto ev = handler< MESSAGE, LAMBDA >( std::forward<LAMBDA>(lambda) );

	create_subscription_for_states(
			ev.m_msg_type,
			ev.m_handler,
			thread_safety );

	return *this;
}

inline void
subscription_bind_t::create_subscription_for_states(
	const std::type_index & msg_type,
	const event_handler_method_t & method,
	thread_safety_t thread_safety ) const
{
	if( m_states.empty() )
		// Agent should be subscribed only in default state.
		m_agent->create_event_subscription(
			m_mbox_ref,
			msg_type,
			m_agent->so_default_state(),
			method,
			thread_safety );
	else
		for( auto s : m_states )
			m_agent->create_event_subscription(
					m_mbox_ref,
					msg_type,
					*s,
					method,
					thread_safety );
}

template< class RESULT, class MESSAGE, class AGENT, class METHOD >
subscription_bind_t &
subscription_bind_t::event_impl(
	METHOD pfn,
	thread_safety_t thread_safety )
{
	using namespace details::event_subscription_helpers;

	// Agent must have right type.
	auto cast_result = get_actual_agent_pointer< AGENT >( *m_agent );

	auto method = [cast_result,pfn](
			invocation_type_t invocation_type,
			message_ref_t & message_ref)
		{
			if( invocation_type_t::service_request == invocation_type )
				{
					using namespace details::promise_result_setting_details;

					auto actual_request_ptr =
							get_actual_service_request_pointer< RESULT, MESSAGE >(
									message_ref );

					auto msg = message_payload_type< MESSAGE >::extract_payload_ptr(
							actual_request_ptr->m_param );
					ensure_message_with_actual_data( msg );

					// All exceptions will be processed in service_handler_on_message.
					result_setter_t< RESULT >().call_new_format_event_and_set_result(
							actual_request_ptr->m_promise,
							cast_result,
							pfn,
							*msg );
				}
			else
				{
					auto msg = message_payload_type< MESSAGE >::extract_payload_ptr(
							message_ref );
					ensure_message_with_actual_data( msg );

					(cast_result->*pfn)( *msg );
				}
		};

	create_subscription_for_states(
			message_payload_type< MESSAGE >::payload_type_index(),
			method,
			thread_safety );

	return *this;
}

/*
 * Implementation of template methods of state_t class.
 */
template< typename... ARGS >
const state_t &
state_t::event( ARGS&&... args ) const
{
	return this->subscribe_message_handler(
			m_target_agent->so_direct_mbox(),
			std::forward< ARGS >(args)... );
}

template< typename... ARGS >
const state_t &
state_t::event( mbox_t from, ARGS&&... args ) const
{
	return this->subscribe_message_handler( from,
			std::forward< ARGS >(args)... );
}

template< typename SIGNAL, typename... ARGS >
const state_t &
state_t::event( ARGS&&... args ) const
{
	return this->subscribe_signal_handler< SIGNAL >(
			m_target_agent->so_direct_mbox(),
			std::forward< ARGS >(args)... );
}

template< typename SIGNAL, typename... ARGS >
const state_t &
state_t::event( mbox_t from, ARGS&&... args ) const
{
	return this->subscribe_signal_handler< SIGNAL >(
			from,
			std::forward< ARGS >(args)... );
}

template< typename... ARGS >
const state_t &
state_t::subscribe_message_handler(
	const mbox_t & from,
	ARGS&&... args ) const
{
	m_target_agent->so_subscribe( from ).in( *this )
			.event( std::forward< ARGS >(args)... );

	return *this;
}

template< typename SIGNAL, typename... ARGS >
const state_t &
state_t::subscribe_signal_handler(
	const mbox_t & from,
	ARGS&&... args ) const
{
	m_target_agent->so_subscribe( from )
			.in( *this ).event(
					so_5::signal< SIGNAL >,
					std::forward< ARGS >(args)... );

	return *this;
}

/*!
 * \since v.5.5.1
 * \brief A shortcat for switching the agent state.
 *
 * \par Usage example.
	\code
	class my_agent : public so_5::agent_t
	{
		const so_5::state_t st_normal = so_make_state();
		const so_5::state_t st_error = so_make_state();
		...
	public :
		virtual void so_define_agent() override
		{
			this >>= st_normal;

			st_normal.handle( [=]( const msg_failure & evt ) {
				this >>= st_error;
				...
			});
			...
		};
		...
	};
	\endcode
 */
inline void
operator>>=( agent_t * agent, const state_t & new_state )
{
	agent->so_change_state( new_state );
}

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::exception_reaction_t
 * instead.
 */
using exception_reaction_t = so_5::exception_reaction_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::abort_on_exception
 * instead.
 */
so_5::exception_reaction_t abort_on_exception = so_5::abort_on_exception;

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::shutdown_sobjectizer_on_exception instead.
 */
so_5::exception_reaction_t shutdown_sobjectizer_on_exception = so_5::shutdown_sobjectizer_on_exception;

/*!
 * \deprecated Will be removed in v.5.6.0. Use
 * so_5::deregister_coop_on_exception instead.
 */
so_5::exception_reaction_t deregister_coop_on_exception = so_5::deregister_coop_on_exception,

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::ignore_exception
 * instead.
 */
so_5::exception_reaction_t ignore_exception = so_5::ignore_exception;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::inherit_exception_reaction
 * instead.
 */
so_5::exception_reaction_t inherit_exception_reaction = so_5::inherit_exception_reaction;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::subscription_bind_t
 * instead.
 */
using subscription_bind_t = so_5::subscription_bind_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::agent_t
 * instead.
 */
using agent_t = so_5::agent_t;

} /* namespace rt */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

