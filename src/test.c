#include <sqlite3.h>
#include <stdio.h>

#include <vars.h>
#include <vardefs.h>

#include <shmem.h>

mem_var Vars[] = 
{
    {
        .name = "my_cnt"
    },
    {
        .name = "my_sec"
    }
};

int main(void) {
	int i,o;

//	printf("%s\n", sqlite3_libversion());

	vars_init();

	o = 0;
	while(1){
		i = shm_readShort(Vars[MY_SEC].offset,0);
		if ( o != i ) {
			printf("my_sec: %d\n",i );
			o = i;
		}
	}
	return 0;
}