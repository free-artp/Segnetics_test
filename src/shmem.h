#ifndef _SHMEM_H_
#define _SHMEM_H_

void shared_memory_init( int size );
void shm_init(const char *shmname, unsigned long Size);

int shm_read(unsigned long offset, void *buf, int len);
int shm_write(unsigned long offset, const void *buf, int len);

int shm_readInt(unsigned long offset, int index);
int shm_readShort(unsigned long offset, int index);
int shm_readByte(unsigned long offset, int index);
float shm_readFloat(unsigned long offset, int index);

int shm_writeInt(unsigned long offset, int index, int val);
int shm_writeShort(unsigned long offset, int index, int val);
int shm_writeByte(unsigned long offset, int index, unsigned char val);
int shm_writeFloat(unsigned long offset, int index, float val);

#endif
