/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.0
 * \file
 * \brief Tools for logging error messages inside SObjectizer core.
 */

#include <so_5/h/error_logger.hpp>

#include <chrono>
#include <ctime>
#include <iostream>

#include <so_5/h/current_thread_id.hpp>

namespace so_5
{

//
// error_logger_t
//
error_logger_t::error_logger_t()
	{}

error_logger_t::~error_logger_t()
	{}

//
// stderr_logger_t
//
/*!
 * \since v.5.5.0
 * \brief A standard implementation of error_logger interface.
 */
class stderr_logger_t : public error_logger_t
	{
	public :
		stderr_logger_t()
			{}

		virtual void
		log(
			const char * file,
			unsigned int line,
			const std::string & message ) override
			{
				using namespace std;
				using namespace chrono;

				ostringstream total_message;

				auto now = system_clock::now();
				auto ms = duration_cast< milliseconds >( now.time_since_epoch() );
				time_t unix_time = duration_cast< seconds >( ms ).count();

				char date_time_first_part[ 64 ];
				strftime( date_time_first_part, sizeof( date_time_first_part ) - 1,
						"%Y-%m-%d %H:%M:%S", localtime( &unix_time ) );
				char date_time_second_part[ 16 ];
				sprintf( date_time_second_part, ".%03u", 
						static_cast< unsigned int >( ms.count() % 1000u ) );

				total_message << "[" << date_time_first_part
						<< date_time_second_part
						<< " TID:" << query_current_thread_id()
						<< "] " << message
						<< " (" << file << ":" << line
						<< ")\n";

				cerr << total_message.str();
			}
	};

//
// create_stderr_logger
//
SO_5_EXPORT_FUNC_SPEC( error_logger_shptr_t )
create_stderr_logger()
	{
		return error_logger_shptr_t( new stderr_logger_t() );
	}

} /* namespace so_5 */
