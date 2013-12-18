/*
 * A sample for the exception handler and cooperation notifications.
 */

#include <iostream>
#include <stdexcept>

// Main SObjectizer header files.
#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

// A class of an agent which will throw an exception.
class a_child_t
	:	public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public :
		a_child_t(
			so_5::rt::so_environment_t & env,
			bool should_throw )
			:	base_type_t( env )
			,	m_should_throw( should_throw )
		{}

		virtual void
		so_evt_start()
		{
			if( m_should_throw )
				throw std::runtime_error( "A child agent failure!" );
		}

	private :
		const bool m_should_throw;
};

// A class of parent agent.
class a_parent_t
	:	public so_5::rt::agent_t
{
	typedef so_5::rt::agent_t base_type_t;

	public :
		a_parent_t(
			so_5::rt::so_environment_t & env )
			:	base_type_t( env )
			,	m_self_mbox( env.create_local_mbox() )
			,	m_counter( 0 )
			,	m_max_counter( 3 )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe( m_self_mbox ).event( &a_parent_t::evt_child_created );
			so_subscribe( m_self_mbox ).event( &a_parent_t::evt_child_destroyed );
		}

		void
		so_evt_start()
		{
			register_child_coop();
		}

		void
		evt_child_created(
			const so_5::rt::event_data_t<
					so_5::rt::msg_coop_registered > & evt )
		{
			std::cout << "coop_reg: " << evt->m_coop_name << std::endl;

			if( m_counter >= m_max_counter )
				so_environment().stop();

			// Otherwise should wait for cooperation shutdown.
		}

		void
		evt_child_destroyed(
			const so_5::rt::event_data_t<
					so_5::rt::msg_coop_deregistered > & evt )
		{
			std::cout << "coop_dereg: " << evt->m_coop_name
				<< ", reason: " << evt->m_reason.reason() << std::endl;

			++m_counter;
			register_child_coop();
		}

	private :
		so_5::rt::mbox_ref_t m_self_mbox;

		int m_counter;
		const int m_max_counter;

		void
		register_child_coop()
		{
			auto coop = so_environment().create_coop( "child" );
			coop->set_parent_coop_name( so_coop_name() );
			coop->add_reg_notificator(
					so_5::rt::make_coop_reg_notificator( m_self_mbox ) );
			coop->add_dereg_notificator(
					so_5::rt::make_coop_dereg_notificator( m_self_mbox ) );

			coop->add_agent(
					new a_child_t(
							so_environment(),
							m_counter < m_max_counter ) );

			std::cout << "registering coop: " << coop->query_coop_name()
					<< std::endl;

			so_environment().register_coop( std::move( coop ) );
		}
};

// The SObjectizer Environment initialization.
void
init( so_5::rt::so_environment_t & env )
{
	// Creating and registering a cooperation.
	env.register_agent_as_coop( "parent", new a_parent_t( env ) );
}

int
main( int, char ** )
{
	try
	{
		so_5::api::run_so_environment( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
