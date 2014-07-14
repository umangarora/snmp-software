#include <iostream>
#include <sstream>
#include <pqxx/pqxx>
#include "ping.h" 

using namespace pqxx;
int main(int argc, char** argv)
{

	try{
		connection C("dbname=mydb user=postgres password=admin hostaddr=127.0.0.1 port=5432");
      		if (!C.is_open()) {
        		std::cout << "Can't open database" << std::endl;
			return false;
      		}
		else{
			
				while(1)
				{
				work W(C);
				std::stringstream query;
				query<<"SELECT c_id,d_id,tstamp,ip FROM get_live_ip()";
				result res( W.exec( query.str() ));
				W.commit();
				for(unsigned int rownum = 0 ;rownum < res.size(); rownum++){
					std::string ip_addr = res[rownum][3].as<std::string>();
					int ping_recv = ping(ip_addr);
					std::cout<<ping_recv<<endl;
					if(ping_recv==0)
					{
						work W2(C);
						std::stringstream query2;
						query2<<"DELETE FROM live_table where client_id = '"<<res[rownum][0]<<"';";
						W2.exec( query2.str() );
						W2.commit();
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
