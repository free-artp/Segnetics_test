#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <syslog.h>
#include <pthread.h>

#include <shmem.h>

#include <config.h>
#include <vars.h>
#include <vardefs.h>


void v_printer( VARS_EVENTS e, mem_var *v ){
	if ( e == VARS_CHANGE_VALUE) {
		printf("change '%s' %d->%d\n", v->name, (short)*((short *)v->value_old), (short)*((short *)v->value));
	}
}

mem_var Vars[] = 
{
    {
        .name = "my_cnt"
    },
    {
        .name = "my_sec",
		.callback = v_printer
    }
};

int main( int argc, char* argv[] ) {
	pthread_t tchecker;
	int result;

//	printf("%s\n", sqlite3_libversion());

	openlog(SYSLOG_NAME, LOG_PID, LOG_USER);
	syslog(LOG_INFO, "start");

	if (config(argc, argv)) {
		perror("config troubles:");
		exit(EXIT_FAILURE);
	}

	int vars_number;
	vars_number = sizeof(Vars) / sizeof(mem_var);
	vars_init( vars_number );

	result = pthread_create(&tchecker, NULL, vars_checker, (void*)&vars_number);
	if ( result != 0 ) {
		perror("Create thread writer:");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "checker created");

	result = pthread_join(tchecker, NULL);
	if (result != 0) {
		perror("Joining the checker thread:");
		exit(EXIT_FAILURE);
	}

	printf("Done\n");
	exit(EXIT_SUCCESS);
}