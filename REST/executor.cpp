#include <stdio.h>
#include <iostream>
#include <vector>
#include <sstream>
//
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
  ss << " order by ts limit " << args.last <<" ;";
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
---
Comment on 3rd July 2014: @Harkirat - wherever there is a cout,
make a child and add it to the ptree
Email me once you are done.
Issue a pull request after testing.
(Remember only one pull request is allowed per branch so rember, to do it in
a sperate branch.)
Extra note: I would advise you to create an inline function with make a node with the connected to, from and to information and returns a reference to the child ptree.
Comment End
---
************************************************
*/
bool format_entries(pqxx::result & res, std::string & response){
  bool ret = true;
  size_t curr = 0;
  size_t prev = curr;
  while(curr < res.size()){
    if(res[curr][4].as<int>() == 1){
      if(res[curr][0].as<int>() == res[prev][0].as<int>()){
        // do nothing associated at same place
      }
      else{ // associated to a new AP
        std::cout<<"Connected to:"<<res[prev][0]<<" from:"<<res[prev][2];
        std::cout<<" to:"<<res[curr][2]<<std::endl;
        prev = curr;
      }
    }
    else if(res[curr][4].as<int>()==2){
      if(res[curr][0].as<int>() == res[prev][0].as<int>()){
        std::cout<<"Connected to:"<<res[prev][0]<<" from:"<<res[prev][2];
        std::cout<<" to:"<<res[curr][2]<<std::endl;
        prev = curr+1;
      }
      else{
        // do nothing, recieved late deauth trap
      }
    }
    curr++;
  }
  // Take care of last record
  if(prev < res.size() && res[prev][4].as<int>()==1){
    std::cout<<"Connected to:"<<res[prev][0]<<" from:"<<res[prev][2];
    std::cout<<" to:NA"<<std::endl;
  }
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
      format_entries(res,response); 
      std::cout<<response<<std::endl; // Added temporarily for testing
      return write_last_std(res,response); // This will be replaced by format_entries()
    }
    else if(type == VALID_API_STD){
      return write_last_std(res,response); // This will be replaced by format_entries()
    }
  }
  catch(const std::exception & e){
    std::cerr<<e.what()<<std::endl;
    return false;
  }
  response = "API not ready yet"; // Remove before issuing a pull request
  return true;
}
