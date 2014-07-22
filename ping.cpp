#include <iostream>
#include <sstream>
#include <pthread.h>
#include <unistd.h>
#include<stdint.h>
#include <stdlib.h>
#include <pqxx/pqxx>
#include "ping.h"

using namespace pqxx;

#define NUM_THREADS     2

struct data_record {
	std:: string *ip;
	int client_id;
};

void *runner(void *param)
{
	int ping_recv;
	struct data_record *record =  (struct data_record*)param;
	std::string *ip_addr = record->ip;
	int client_id = record->client_id;
	try{
		connection C1("dbname=mydb user=postgres password=admin hostaddr=127.0.0.1 port=5432");
      		if (!C1.is_open()) {
			pthread_exit(0);
      		}
		else{
			ping_recv = ping(*ip_addr);
			if(ping_recv==0)
			{
				work W2(C1);
				std::stringstream query2;
				query2<<"DELETE FROM live_table where client_id = '"<<record->client_id<<"';";
				W2.exec( query2.str() );
				W2.commit();
			}	
		}
		delete record->ip;
		pthread_exit(0);
	}
	catch (const std::exception &e){
      	std::cerr << e.what() << std::endl;
   	}
}

void create_thread(pthread_t threads[],int thread_num, struct data_record *record)
{
	int rc;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	rc = pthread_create(&threads[thread_num], &attr, runner, (void *) record);
	if (rc){	
	         cout << "Error:unable to create thread," << rc << endl;
        	 exit(-1);
      		}
}	

int main(int argc, char** argv)
{

	try{
		connection C("dbname=mydb user=postgres password=admin hostaddr=127.0.0.1 port=5432");
      		if (!C.is_open()) {
			return false;
      		}
		else{
			pthread_t threads[NUM_THREADS];
			while(1)
			{
				work W(C);
				std::stringstream query;
				query<<"SELECT c_id,d_id,tstamp,ip FROM get_live_ip()";
				result res( W.exec( query.str() ));
				W.commit();
				unsigned rownum = 0;
				while( rownum<res.size() )
				{
					int k;
					for(k=0;k<NUM_THREADS;k++)
					{
						struct data_record *record ;

						record = (struct data_record *)malloc(sizeof(struct data_record));
						record->client_id = res[rownum][0].as<int>();
						record->ip = new std::string (res[rownum][3].as<std::string>() );
						create_thread(threads,k, record);
						rownum++;
						if(rownum>=res.size()){k++;break;}
		
					}
					for(int l=0;l<k;l++)
					{
						pthread_join(threads[l],NULL);
					}								
				
				}
			sleep(5);
			}
		}
	}
	catch (const std::exception &e){
      	std::cerr << e.what() << std::endl;
   	}
}
