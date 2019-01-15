#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


#include "iniparser.h"

void create_example_ini_file(char* ini_name);
int  parse_ini_file(int thread_no, char * ini_name);

#define THREAD_NUM 5
pthread_t threads[THREAD_NUM];
int done[THREAD_NUM];

void *thread_main(void *);


int main(int argc, char * argv[])
{
	int i;
	int rc; 
	int	status; 
	char file_buf[1024];

    for (i = 0; i < THREAD_NUM; i++) {   
        sprintf(file_buf, "%s_%d.ini", "example", i); 
        create_example_ini_file(file_buf);
    }   

    for (i = 0; i < THREAD_NUM; i++) {   
        done[i] = 0;
        pthread_create(&threads[i], NULL, &thread_main, (void *)i);
        //printf("%d, %d\n", i, threads[i]);
    }	

	for (i = THREAD_NUM - 1; i >= 0; i--) {
		done[i] = 1;
		rc = pthread_join(threads[i], (void **)&status);
		if (rc == 0) {
			printf("Completed join with thread %d status= %d\n",i, status);
		}
		else {
			printf("ERROR; return code from pthread_join() is %d, thread %d\n", rc, i);
			return -1;
		}
	}

	return status ;
}

void *thread_main(void *arg)
{
	int i;
	int	status ; 
	char file_buf[1024];

	//printf("therad: %d, %d\n", (int)arg, getpid());
	
	sprintf(file_buf, "%s_%d.ini", "example", arg);

	status = parse_ini_file(arg, file_buf);
	if( status != 0 ){
		printf("ERROR; parse_ini_file return error!\n");
	}

	pthread_exit((void *) 0);
}

void create_example_ini_file(char* ini_name)
{
	FILE	*	ini ;

	if (ini = fopen(ini_name, "r"))
    {   
        fclose(ini);
        return;
    }

	ini = fopen(ini_name, "w");
	fprintf(ini, "\n\
#\n\
# This is an example of ini file\n\
#\n\
\n\
[Pizza]\n\
\n\
Ham       = yes ;\n\
Mushrooms = TRUE ;\n\
Capres    = 0 ;\n\
Cheese    = NO ;\n\
\n\
\n\
[Wine]\n\
\n\
Grape     = Cabernet Sauvignon ;\n\
Year      = 1989 ;\n\
Country   = Spain ;\n\
Alcohol   = 12.5  ;\n\
\n\
#\n\
# end of file\n\
#\n");

	fclose(ini);
}


int parse_ini_file(int thread_no, char * ini_name)
{
	dictionary	*	ini ;

	/* Some temporary variables to hold query results */
	int				b ;
	int				i ;
	double			d ;
	char		*	s ;

	ini = iniparser_load(ini_name);
	if (ini==NULL) {
		fprintf(stderr, "cannot parse file [%s]", ini_name);
		return -1 ;
	}
	//iniparser_dump(ini, stderr);

	/* Get pizza attributes */
	//printf("Pizza:\n");

	b = iniparser_getboolean(ini, "pizza:ham", -1);
	printf("Thread %d: Ham:       [%d]\n",thread_no, b);
	b = iniparser_getboolean(ini, "pizza:mushrooms", -1);
	printf("Thread %d: Mushrooms: [%d]\n",thread_no, b);
	b = iniparser_getboolean(ini, "pizza:capres", -1);
	printf("Thread %d: Capres:    [%d]\n",thread_no, b);
	b = iniparser_getboolean(ini, "pizza:cheese", -1);
	printf("Thread %d: Cheese:    [%d]\n",thread_no, b);

	/* Get wine attributes */
	//printf("Wine:\n");
    s = iniparser_getstring(ini, "wine:grape", NULL);
	if (s) {
		printf("Thread %d: grape:     [%s]\n",thread_no, s);
	} else {
		printf("Thread %d: grape:     not found\n",thread_no);
	}
    i = iniparser_getint(ini, "wine:year", -1);
	if (i>0) {
		printf("Thread %d: year:      [%d]\n",thread_no, i);
	} else {
		printf("Thread %d: year:      not found\n",thread_no);
	}
	s = iniparser_getstring(ini, "wine:country", NULL);
	if (s) {
		printf("Thread %d: country:   [%s]\n",thread_no, s);
	} else {
		printf("Thread %d: country:   not found\n",thread_no);
	}
	d = iniparser_getdouble(ini, "wine:alcohol", -1.0);
	if (d>0.0) {
		printf("Thread %d: alcohol:   [%g]\n", thread_no, d);
	} else {
		printf("Thread %d: alcohol:   not found\n", thread_no);
	}

	iniparser_freedict(ini);
	return 0 ;
}


