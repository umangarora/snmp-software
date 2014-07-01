#include <stdio.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <iterator>
#include <stdint.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>


#include <pqxx/pqxx>

#include "executor.hpp"
#include "strutil.hpp"
#include <sha.h>

using namespace ourapi;

using boost::property_tree::ptree;
using std::make_pair;
using std::vector;


Executor::Executor()
{
}

/*
******************************
Getting UID from MAC functions
******************************
*/
bool Executor::uid(const args_container &args, outputType type, string & response){
	std::stringstream ss;
	ss << "SELECT uid from uid where hash=decode('"<< generatehash(args.mac)<<"','hex');";
	return Executor::generic_query(response,ss.str(),VALID_API_MAC);
}

bool write_uid(pqxx::result & res,string & response){
  ptree root_t;
  root_t.put("uid",res[0][0]);
  std::ostringstream oss;
  write_json(oss,root_t);
  response = oss.str();
  return true;
}
/*
*******************************
Getting last N entries function
89-*******************************
*/
bool Executor::last(const args_container &args, outputType type, string & response,const string & url){
  std::stringstream ss;
  ss << " select device_id,client_id,ts,label,type from logs where ";
  if( url == "/client"){
    ss << " client_id = " << args.uid;
  }
  else if( url == "/ap"){
    ss << " device_id = " << args.uid;
  }
  else { // Not yet implemented API or invalid API
    return false;
  }
  //ss << " and ts >= to_timestamp('" << args.from <<"','"<<args.format<<"') ";
  //ss << " and ts <= to_timestamp('" << args.to << "','" << args.format << "')";
  ss << " order by ts desc limit " << args.last <<" ;";
  return Executor::generic_query(response,ss.str(),VALID_API_LAST);
}

bool write_last_std(pqxx::result & res, string & response){
	ptree root_t;
  ptree children;
  root_t.put("size",res.size());
  for(unsigned int rownum = 0 ;rownum < res.size(); rownum++){
    ptree child;
    child.put("device_id",res[rownum][0]);
    child.put("client_id",res[rownum][1]);
    child.put("ts",res[rownum][2]);
    child.put("label",res[rownum][3]);
    child.put("type",res[rownum][4]);
    children.push_back(make_pair("",child));
  }
  root_t.add_child("log entries",children);
  std::stringstream ss;
  write_json(ss,root_t);
  response = ss.str();
  return true;
}
/*
************************************************
Getting entries between two dates+times function
************************************************
*/
bool Executor::std(const args_container &args, outputType type, string & response,const string & url){
  std::stringstream ss;
  ss << " select device_id,client_id,ts,label,type from logs where ";
  if( url == "/client"){
    ss << " client_id = " << args.uid;
  }
  else if( url == "/ap"){
    ss << " device_id = " << args.uid;
  }
  else { // Not yet implemented API or invalid API
    return false;
  }
  ss << " and ts >= to_timestamp('" << args.from <<"','"<<args.format<<"') ";
  ss << " and ts <= to_timestamp('" << args.to << "','" << args.format << "')";
  ss << " order by ts desc;";
  return Executor::generic_query(response,ss.str(),VALID_API_STD);
}
/*
************************************************
For Harkirat - duration function
************************************************
*/
bool format_entries(pqxx::result & res, std::string & response){
	bool ret=false;
	pqxx::result::const_iterator cur_it = res.begin(), prev_it = cur_it;
	int cur_traptype;
	vector<struct duration_container> durationRecords;
	if( cur_it!=res.end() ) {
		createNew(durationRecords, cur_it);
		cur_it++;
	}

	while(cur_it != res.end()){
		cur_traptype = cur_it[4].as<int>();
		if( cur_traptype ==1 ){
			if( prev_it[0].as<int>() == cur_it[0].as<int>() ){
				if(!durationRecords.back().deauthFlag){
					durationRecords.back().to = cur_it[2].as<string>();
				}
				if(durationRecords.back().deauthFlag) {
					createNew(durationRecords, cur_it);
				}
			}
			else{
				if(!durationRecords.back().deauthFlag){
					durationRecords.back().to = cur_it[2].as<string>();
					createNew(durationRecords, cur_it);
				}
				if(durationRecords.back().deauthFlag){
					createNew(durationRecords, cur_it);
				}
			}
		}
		else if( cur_traptype ==2 ){
			if( prev_it[0].as<int>() == cur_it[0].as<int>() ){
				if(!durationRecords.back().deauthFlag){
					durationRecords.back().to =  cur_it[2].as<string>();			
					durationRecords.back().deauthFlag =  true;
				}
			}
			else{
				for(vector<struct duration_container>::reverse_iterator i = durationRecords.rbegin(); 
				i != durationRecords.rend(); ++i)
				{
					struct duration_container ele = *i;
					if( ele.device_id==cur_it[0].as<int>() ){
						if(!durationRecords.back().deauthFlag){
							ele.to = cur_it[2].as<int>();
							ele.deauthFlag = true;
						}
					}
				}
			}
		}
	cur_it++;
	prev_it++;

	} //while ends
	std::stringstream ss;
	std::copy(durationRecords.begin(), durationRecords.end(),std::ostream_iterator<struct duration_container>(ss,"\n"));
	response = ss.str();
	ret=true;
	return ret;
}
/*
*****************************
Function to execute SQL query
*****************************
*/
bool Executor::generic_query(string & response, const string query,unsigned int type){
  try{
    pqxx::connection conn("dbname=mydb user=postgres password=admin hostaddr=127.0.0.1 port=5432");
    if(!conn.is_open()){
      return false;
    }
    pqxx::work w(conn);
    pqxx::result res = w.exec(query);
    w.commit();
    //TBD: Maybe some checking on pqxx::result itself
    if(type == VALID_API_MAC){
      return write_uid(res,response);
    }
    else if(type == VALID_API_LAST){
      return write_last_std(res,response);
    }
    else if(type == VALID_API_STD){
      return write_last_std(res,response);
    }
  }
  catch(const std::exception & e){
    std::cerr<<e.what()<<std::endl;
    return false;
  }
  response = "API not ready yet"; // Remove before issuing a pull request
  return true;
}
