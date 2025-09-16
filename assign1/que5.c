#define N 5

int value = 0;

void *thread_func(void *param);

int main(int argc, char *argv[]) {
	pid_t pid;
	pthread_t tid;
	pthread_attr_t attr;

	pid = fork();

	if (pid == 0)
	{
		pthread_attr_init(&attr);
	}
	
	

}
