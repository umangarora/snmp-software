#include <iostream>
#include <pqxx/pqxx>

using namespace std;
using namespace pqxx;

int main(int argc, char* argv[])
{
	char *sql;
   
	try{
		connection C("dbname=mydb user=postgres password=admin \
		hostaddr=127.0.0.1 port=5432");
		if (C.is_open()) {
			cout << "Opened database successfully: " << C.dbname() << endl;
      		} else {
			cout << "Can't open database" << endl;
         		return 1;
      		}

	sql = (char*)"CREATE OR REPLACE FUNCTION get_live_table() RETURNS SETOF RECORD AS $$ \
		DECLARE \
			r RECORD;\
		BEGIN \
			FOR r IN SELECT device_id, client_id, ts FROM live_table LOOP \
				RETURN NEXT r; \
			END LOOP; \
			RETURN ; \
		END; \
		$$ LANGUAGE plpgsql;" ;

/*"CREATE OR REPLACE FUNCTION get_live_table() RETURNS SETOF live_table AS $$ \
		BEGIN \
			RETURN QUERY SELECT device_id, client_id, ts FROM live_table; \
		END; \
		$$ LANGUAGE plpgsql;" ;*/

		/*DECLARE \
			r RECORD;\
		BEGIN \
			FOR r IN SELECT device_id, client_id, ts FROM live_table LOOP \
				RETURN NEXT r; \
			END LOOP; \
			RETURN ; \
		END; \*/
	work w(C);
	w.exec(sql);
	w.commit();
	
	C.disconnect ();
	}catch (const std::exception &e){
		cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
