#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sem.h>

/*int semctl(int semid, int semnum, int cmd, union semun arg);

cmd:SETVAL IPC_RMID（主要这两个）
	IPC_STAT GETALL SETALL IPC_SET

union semun 类型（具体的需要由程序员自己定义），它至少包含以下几
个成员：
union semun{
	int val; // Value for SETVAL 
	struct semid_ds *buf; // Buffer for IPC_STAT, IPC_SET 
	unsigned short *array; // Array for GETALL, SETALL 
}; */

union semun  
{  
    int val;  				/*选中信号量集中指定信号量 第一个为0*/
    struct semid_ds *buf;  /*设置信号量初始值*/
    unsigned short *array;  /*批量输出(设置)信号量集中的当前值(初始值)*/
};  

static int sem_id = 0;

static int set_semvalue();
static void del_semvalue();
static int semaphore_p();
static int semaphore_v();

int main(int argc, char *argv[])
{
	char message = 'X';
	int i = 0;

	//创建信号量 通过1234可以访问这个信号量
	sem_id = semget((key_t)1234, 1, 0666 | IPC_CREAT);
    /*int semget(key_t key,int nsems,int flag);
	参数 key 是唯一标识一个信号量的关键字，如果为 IPC_PRIVATE(值为 0，创建一个只有创建
			 者进程才可以访问的信号量集),表示创建一个只由调用进程使用的信号量，非 0 值的 key(可以通过
			 ftok 函数获得)表示创建一个可以被多个进程共享的信号量；
	参数 nsems 指定需要使用的信号量数目
	参数 flag 是一组标志，其作用与 open 函数的各种标志很相似。它低端的九个位是该信号量的
				权限，其作用相当于文件的访问权限。
	*/
	if(argc > 1)
	{
		//程序第一次被调用，初始化信号量
		if(!set_semvalue())
		{
			fprintf(stderr, "Failed to initialize semaphore\n");
			exit(EXIT_FAILURE);
		}
		//设置要输出到屏幕中的信息，即其参数的第一个字符
		message = argv[1][0];
		sleep(2);
	}
	for(i = 0; i < 10; ++i)
	{
		//进入临界区
		if(!semaphore_p())
			exit(EXIT_FAILURE);
		//向屏幕中输出数据
		printf("%c", message);
		//清理缓冲区，然后休眠随机时间
		fflush(stdout);
		sleep(rand() % 3);
		//离开临界区前再一次向屏幕输出数据
		printf("%c", message);
		fflush(stdout);
		//离开临界区，休眠随机时间后继续循环
		if(!semaphore_v())
			exit(EXIT_FAILURE);
		sleep(rand() % 2);
	}

	sleep(10);
	printf("\n%d - finished\n", getpid());

	if(argc > 1)
	{
		//如果程序是第一次被调用，则在退出前删除信号量
		sleep(3);
		del_semvalue();
	}
	exit(EXIT_SUCCESS);
}

static int set_semvalue()
{
	//用于初始化信号量，在使用信号量前必须这样做
	union semun sem_union;

	sem_union.val = 1;
	if(semctl(sem_id, 0, SETVAL, sem_union) == -1)
		return 0;
	return 1;
}

static void del_semvalue()
{
	//删除信号量
	union semun sem_union;

	if(semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
		fprintf(stderr, "Failed to delete semaphore\n");
}
/* SETVAL：用来把信号量初始化为一个已知的值。p 这个值通过union semun中的val成员设置，其作用是在信号量第一次使用前对它进行设置。
IPC_RMID：用于删除一个已经无需继续使用的信号量标识符。
 */
static int semaphore_p()
{
	//对信号量做减1操作，即等待P（sv）
	struct sembuf sem_b;
	sem_b.sem_num = 0;//信号量在信号量集中的编号，第一个信号量的编号是 0
	sem_b.sem_op = -1;//P()
	sem_b.sem_flg = SEM_UNDO;
	if(semop(sem_id, &sem_b, 1) == -1)
	{
		fprintf(stderr, "semaphore_p failed\n");
		return 0;
	}
	return 1;
}

static int semaphore_v()
{
	//这是一个释放操作，它使信号量变为可用，即发送信号V（sv）
	struct sembuf sem_b;
	sem_b.sem_num = 0;//信号量在信号量集中的编号，第一个信号量的编号是 0
	sem_b.sem_op = 1;//V()
	sem_b.sem_flg = SEM_UNDO;
	if(semop(sem_id, &sem_b, 1) == -1)
	{
		fprintf(stderr, "semaphore_v failed\n");
		return 0;
	}
	return 1;
}

/* struct sembuf{  
    short sem_num;//除非使用一组信号量，否则它为0  
    short sem_op;//信号量在一次操作中需要改变的数据，通常是两个数，一个是-1，即P（等待）操作，  
                    //一个是+1，即V（发送信号）操作。  
    short sem_flg;//通常为SEM_UNDO,使操作系统跟踪信号，  
                    //并在进程没有释放该信号量而终止时，操作系统释放信号量  
}; */