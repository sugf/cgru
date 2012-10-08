#include "afcommon.h"
#include "jobcontainer.h"
#include "monitoraf.h"
#include "monitorcontainer.h"
#include "rendercontainer.h"
#include "threadargs.h"
#include "usercontainer.h"

#define AFOUTPUT
#undef AFOUTPUT
#include "../include/macrooutput.h"

af::Msg * threadProcessJSON( ThreadArgs * i_args, af::Msg * i_msg)
{
	rapidjson::Document document;
	std::string error;
	char * data = af::jsonParseMsg( document, i_msg, &error);
	if( data == NULL )
	{
		AFCommon::QueueLogError( error);
		delete i_msg;
		return NULL;
	}

    af::Msg * o_msg_response = NULL;

	JSON & getObj = document["get"];
	if( getObj.IsObject())
	{
		std::string type, mode;
		bool binary = false;
		af::jr_string("type", type, getObj);
		af::jr_string("mode", mode, getObj);
		af::jr_bool("binary", binary, getObj);

		bool json = true;
		if( binary )
			json = false;
		bool full = false;
		if( mode == "full")
			full = true;

		std::vector<int32_t> ids;
		af::jr_int32vec("ids", ids, getObj);

		std::string mask;
		af::jr_string("mask", mask, getObj);

		if( type == "jobs" )
		{
			std::vector<int32_t> uids;
			af::jr_int32vec("uids", uids, getObj);
			if( uids.size())
			{
				AfContainerLock jLock( i_args->jobs,  AfContainerLock::READLOCK);
				AfContainerLock uLock( i_args->users, AfContainerLock::READLOCK);
				o_msg_response = i_args->users->generateJobsList( uids, type, json);
			}
			else
			{
				AfContainerLock lock( i_args->jobs, AfContainerLock::READLOCK);
				JobAf * job = NULL;
				bool was_error = false;
				if( ids.size() == 1 )
				{
					JobContainerIt it( i_args->jobs);
					job = it.getJob( ids[0]);
					if( job == NULL )
						o_msg_response = af::jsonMsgError( "Invalid ID");
				}

				if( job )
				{
					std::vector<int32_t> block_ids;
					af::jr_int32vec("block_ids", block_ids, getObj);
					if( block_ids.size())
					{
						std::vector<int32_t> task_ids;
						af::jr_int32vec("task_ids", task_ids, getObj);
						if( task_ids.size())
							o_msg_response = job->writeTask( block_ids[0], task_ids[0], mode);
						else
						{
							std::vector<std::string> modes;
							af::jr_stringvec("mode", modes, getObj);
							o_msg_response = job->writeBlocks( block_ids, modes);
						}
					}
					else if( mode.size())
					{
						if(( mode == "progress" ) && ( job != NULL ))
							o_msg_response = job->writeProgress( json);
						else if(( mode == "error_hosts" ) && ( job != NULL ))
							o_msg_response = job->writeErrorHosts();
						else if(( mode == "log" ) && ( job != NULL ))
							o_msg_response = job->writeLog();
					}
				}
				
				if( o_msg_response == NULL )
					o_msg_response = i_args->jobs->generateList(
						full ? af::Msg::TJob : af::Msg::TJobsList, type, ids, mask, json);
			}
		}
		else if( type == "users")
		{
			AfContainerLock lock( i_args->users, AfContainerLock::READLOCK);
			if( mode.size())
			{
				UserAf * user = NULL;
				if( ids.size() == 1 )
				{
					UserContainerIt it( i_args->users);
					user = it.getUser( ids[0]);
					if( user == NULL )
						o_msg_response = af::jsonMsgError( "Invalid ID");
				}
				if( user )
				{
					if( mode == "log" )
						o_msg_response = user->writeLog();
				}
			}
			if( o_msg_response == NULL )
				o_msg_response = i_args->users->generateList( af::Msg::TUsersList, type, ids, mask, json);
		}
		else if( type == "renders")
		{
			AfContainerLock lock( i_args->renders, AfContainerLock::READLOCK);
			if( mode.size())
			{
				RenderAf * render = NULL;
				if( ids.size() == 1 )
				{
					RenderContainerIt it( i_args->renders);
					render = it.getRender( ids[0]);
					if( render == NULL )
						o_msg_response = af::jsonMsgError( "Invalid ID");
				}
				if( render )
				{
					if( full )
						o_msg_response = render->jsonWrite( type, "object");
					else if( mode == "log" )
						o_msg_response = render->writeLog();
					else if( mode == "tasks_log" )
						o_msg_response = af::jsonMsg("tasks_log", render->getName(), render->getTasksLog());
				}
			}
			if( o_msg_response == NULL )
				o_msg_response = i_args->renders->generateList( af::Msg::TRendersList, type, ids, mask, json);
		}
		else if( type == "monitors")
		{
			AfContainerLock lock( i_args->monitors, AfContainerLock::READLOCK);
			if( mode == "events")
			{
				MonitorContainerIt it( i_args->monitors);
				if( ids.size() )
				{
					MonitorAf* node = it.getMonitor( ids[0]);
					if( node != NULL )
					{
						o_msg_response = node->getEvents();
					}
					else
					{
						o_msg_response = af::jsonMsg("{\"id\":0}");
					}
				}
				else
				{
					o_msg_response = af::jsonMsg("{\"error\":\"id not specified\"}");
				}
			}
			else
				o_msg_response = i_args->monitors->generateList( af::Msg::TMonitorsList, type, ids, mask, json);
		}
		else if( type == "files")
		{
			std::string path;
			std::ostringstream files;
			af::jr_string("path", path, getObj);
			std::vector<std::string> list = af::getFilesList( path);
			files << "{\"path\":\"" << path << "\",\n";
			files << "\"files\":[";
			for( int i = 0; i < list.size(); i++)
			{
				if( i )
					files << ',';
				files << '"' << list[i] << '"';
			}
			files << "]}";
			o_msg_response = af::jsonMsg( files);
		}
	}
	else if( document.HasMember("action"))
	{
		i_args->msgQueue->pushMsg( i_msg);
		// To not to detele it, set to NULL, as it pushed to another queue
		i_msg = NULL; //< To not to detele it, as it pushed to another queue
	}
	else if( document.HasMember("job"))
	{
		if( i_msg->isMagicInvalid() )
		{
			AFCommon::QueueLogError("Job registration is not allowed: Magic number mismatch.");
		}
		else
		{
			// No containers locks needed here.
			// Job registration is a complex procedure.
			// It locks and unlocks needed containers itself.
			i_args->jobs->job_register( new JobAf( document["job"]), i_args->users, i_args->monitors);
		}
	}
	else if( document.HasMember("monitor"))
	{
		AfContainerLock lock( i_args->monitors, AfContainerLock::WRITELOCK);
		MonitorAf * newMonitor = new MonitorAf( document["monitor"]);
		newMonitor->setAddressIP( i_msg->getAddress());
		o_msg_response = i_args->monitors->addMonitor( newMonitor, true);
	}


	delete [] data;
	if( i_msg ) delete i_msg;

	return o_msg_response;
}
