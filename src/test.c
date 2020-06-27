#include <sqlite3.h>
#include <stdio.h>


int get_shmem_size();
void shared_memory_init( void );
void vars_init(void);

int main(void) {
	//int i;

	printf("%s\n", sqlite3_libversion());

	//get_shmem_size();
	//shared_memory_init();
	vars_init();

	return 0;
}