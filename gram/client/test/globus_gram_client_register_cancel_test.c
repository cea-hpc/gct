#include "globus_common.h"
#include "globus_gram_client.h"

typedef struct
{
	int				state;
	int				errorcode;
	globus_mutex_t			mutex;
	globus_cond_t			cond;
}
monitor_t;

static
void
gram_state_callback(
    void *				arg,
    char *				job_contact,
    int					state,
    int					errorcode);

static
void
nonblocking_callback(
    void *				arg,
    globus_gram_protocol_error_t        operation_failure_code,
    const char *                        job_contact,
    globus_gram_protocol_job_state_t    job_state,
    globus_gram_protocol_error_t        job_failure_code);

int main(int argc, char *argv[])
{
    char *				callback_contact;
    char *				job_contact;
    monitor_t				monitor;
    int					rc = 0;

    if(argc != 2)
    {
	fprintf(stderr, "Usage: %s resource-manager-contact\n", argv[0]);
	exit(1);
    }
    rc = globus_module_activate(GLOBUS_COMMON_MODULE);
    if(rc)
    {
	goto end;
    }
    rc = globus_module_activate(GLOBUS_GRAM_CLIENT_MODULE);
    if(rc)
    {
	goto disable_modules;
    }

    globus_mutex_init(&monitor.mutex ,GLOBUS_NULL);
    globus_cond_init(&monitor.cond, GLOBUS_NULL);
    monitor.state = GLOBUS_GRAM_PROTOCOL_JOB_STATE_PENDING;

    rc = globus_gram_client_callback_allow(gram_state_callback,
	                                   &monitor,
					   &callback_contact);
    if(rc != GLOBUS_SUCCESS)
    {
	fprintf(stderr,
		"Error creating callback contact %s.\n",
		globus_gram_client_error_string(rc));

	goto error_exit;
    }

    globus_mutex_lock(&monitor.mutex);
    rc = globus_gram_client_job_request(
	    argv[1],
	    "&(executable=/bin/sleep)(arguments=300)",
	    GLOBUS_GRAM_PROTOCOL_JOB_STATE_ALL,
	    callback_contact,
	    &job_contact);

    if(rc != GLOBUS_SUCCESS)
    {
	fprintf(stderr,
		"Error submitting job request %s.\n",
		globus_gram_client_error_string(rc));

	goto destroy_callback_contact;
    }

    while(monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_ACTIVE &&
	  monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED &&
	  monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE)
    {
	globus_cond_wait(&monitor.cond, &monitor.mutex);
    }

    if(monitor.state == GLOBUS_GRAM_PROTOCOL_JOB_STATE_ACTIVE)
    {
	rc = globus_gram_client_register_job_cancel(
		job_contact,
		GLOBUS_GRAM_CLIENT_NO_ATTR,
		nonblocking_callback,
		&monitor);

	if(rc != GLOBUS_SUCCESS)
	{
	    fprintf(stderr,
		    "Error cancelling job %s.\n",
		    globus_gram_client_error_string(rc));

	    goto destroy_callback_contact;
	}
    }

    while(monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_DONE &&
	  monitor.state != GLOBUS_GRAM_PROTOCOL_JOB_STATE_FAILED)
    {
	globus_cond_wait(&monitor.cond, &monitor.mutex);
    }
    rc = monitor.errorcode;
    if(rc == GLOBUS_GRAM_PROTOCOL_ERROR_USER_CANCELLED)
    {
	rc = GLOBUS_SUCCESS;
    }
destroy_callback_contact:
    globus_mutex_unlock(&monitor.mutex);
    globus_gram_client_callback_disallow(callback_contact);
    globus_libc_free(callback_contact);
    globus_libc_free(job_contact);
error_exit:
    globus_mutex_destroy(&monitor.mutex);
    globus_cond_destroy(&monitor.cond);
disable_modules:
    globus_module_deactivate_all();
end:
    return rc;
}
/* main() */

static
void
gram_state_callback(
    void *				arg,
    char *				job_contact,
    int					state,
    int					errorcode)
{
    nonblocking_callback(arg, 0, job_contact, state, errorcode);
}
/* gram_state_callback() */

static
void
nonblocking_callback(
    void *				arg,
    globus_gram_protocol_error_t        operation_failure_code,
    const char *                        job_contact,
    globus_gram_protocol_job_state_t    job_state,
    globus_gram_protocol_error_t        job_failure_code)
{
    monitor_t *				monitor;

    monitor = arg;

    globus_mutex_lock(&monitor->mutex);
    monitor->state = job_state;
    monitor->errorcode =
	operation_failure_code ? operation_failure_code : job_failure_code;
    globus_cond_signal(&monitor->cond);
    globus_mutex_unlock(&monitor->mutex);
}
