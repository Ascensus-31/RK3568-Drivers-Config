
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 * ./gpio-test-app /dev/mygpio0 hi
 * ./gpio-test-app /dev/mygpio0 lo
 * ./gpio-test-app /dev/mygpio0 read
 */
int main(int argc, char **argv)
{
	int fd;
	char wStatus;
	char rStatus;
	
	/* 1. 判断参数 */
	if (argc != 3) 
	{
		printf("Usage: %s <dev> <hi | lo>\n", argv[0]);
		return -1;
	}

	/* 2. 打开文件 */
	fd = open(argv[1], O_RDWR);
	if (fd == -1)
	{
		printf("can not open file %s\n", argv[1]);
		return -1;
	}

	/* 3. 写文件 */
	if (0 == strcmp(argv[2], "hi"))
	{
		wStatus = 1;
		write(fd, &wStatus, 1);
		printf("Set %s to %d \n", argv[1], wStatus);

	}
	else if (0 == strcmp(argv[2], "lo"))
	{
		wStatus = 0;
		write(fd, &wStatus, 1);
		printf("Set %s to %d \n", argv[1], wStatus);

	}
	/* 4. 读文件 */
	else if (0 == strcmp(argv[2],"read"))
	{
		read(fd,&rStatus,1);
		printf("%s status:%c\n",argv[1], rStatus);
	}
	else{printf("Invalid Command.\n");};

	close(fd);

	return 0;
}


