//============================================================================
// Name        : TestWS.cpp
// Author      : Ilya Juhnowski
// Version     :
// Copyright   : for nikolay.popok@exness.com>
// Description : test task
//               for debugging uncomment lines
//============================================================================

#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include "/usr/include/fcgi_config.h"
#include "/usr/include/fcgiapp.h"
#define THREAD_COUNT 8
#define SOCKET_PATH "127.0.0.1:9000"
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <sstream>

using namespace std;

static int socketId;
int parse_query(char* cs, sqlite3 *db);


static void *doit(void *a)
{
	int rc = 0;
    FCGX_Request request;
    char *query;
    sqlite3 *db;
    int respq = 0;

	   rc = sqlite3_open("test.db", &db);

	   if( rc ) {
	      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	      return NULL;
	   }else{
	      fprintf(stderr, "Opened database successfully\n");
	   }

    if(FCGX_InitRequest(&request, socketId, 0) != 0) {
        printf("Can not init request\n");
        return NULL;
    }
//    printf("Request is inited\n");

    for(;;)
    {
        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

//        printf("Try to accept new request\n");
        pthread_mutex_lock(&accept_mutex);
        rc = FCGX_Accept_r(&request);
        pthread_mutex_unlock(&accept_mutex);

        if(rc < 0) {
            printf("Can not accept new request\n");
            break;
        }
//        printf("request is accepted\n");

        query = FCGX_GetParam("REQUEST_URI", request.envp);
//        printf("REQUEST_URI=%s\n",query);

        FCGX_PutS("Content-type: text/html\r\n", request.out);
        FCGX_PutS("\r\n", request.out);

        respq = parse_query(query, db);
        if (respq==0) {
        	FCGX_PutS("False\r\n", request.out);
        } else {
        	if (respq>0) {
        		FCGX_PutS("True\r\n", request.out);
        	} else {
        		FCGX_PutS("Unknown\r\n", request.out);
        	}
        }
        FCGX_Finish_r(&request);
    }
    return NULL;
}

unsigned int parse_ip4(string* s) {
	string delimiter = ".";
	int delimeter_length = 1;
	unsigned long trans_table[4]={16777216L, 65536L, 256L, 1L};
	unsigned long res = 0;

	size_t pos = 0;
	size_t i = 0;
	std::string token;
	while ((pos = s->find(delimiter)) != std::string::npos) {
	    token = s->substr(0, pos);

	    int b = atoi(token.c_str());
	    res += b*trans_table[i++];
	    s->erase(0, pos + delimeter_length);
	}
	return res;
}

int parse_ip2(string* s) {
	string delimiter = ".";
	int res = -1;

	size_t pos = 0;
	size_t i = 0;
	std::string token;
	while (((pos = s->find(delimiter)) != std::string::npos) || i>1) {
	    token = s->substr(0, pos);

	    int b = atoi(token.c_str());
	    if (i==0) {
	    	res = b*1000;
	    } else {
	    	res+=b;
	    }
	}
	return res;
}

unsigned int HashLy(const char * str)
{
    unsigned int hash = 0;
    for(; *str; str++)
        hash = (hash * 1664525) + (unsigned char)(*str) + 1013904223;
    return hash;
}

int parse_query(char* cs, sqlite3 *db) {
	string * s = new string(cs);
	cout << cs << endl;
	string delimiter = "/";
	int delimeter_length = 1;
	int result = -1;
	string sql;
	string params[4]={};
	char *zErrMsg = 0;
	unsigned int hashed_name;
	unsigned int hashed_name2;
	int     error = 0;
	const char      *tail;
	sqlite3_stmt *res;
    std::ostringstream ss;

	size_t pos = 0;
	size_t i = -2;
	std::string token;
	while ((pos = s->find(delimiter)) != std::string::npos) {
	    token = s->substr(0, pos);
	    if (i == -2) {
	    	i=-1;
	    } else {
	    	params[i] = token;
//	    	cout << i << ":" << token << endl;
	    }
	    i++;
    	s->erase(0, pos + delimeter_length);
	};
    token = s->substr(0, pos);
	params[++i] = token;
//	cout << i << ":" << token << endl;


	if(params[0]=="add") {
		hashed_name = HashLy((&params[1])->c_str());
		ss.str("");
		ss.clear();
		ss << "INSERT INTO `iptables` (`user_id`, `ip_address`) VALUES (" << hashed_name << "," << parse_ip4(&params[2]) << ");" << endl;
//		cout << "SQL:" << sql << endl;
		sql = ss.str();
		error = sqlite3_exec(db,sql.c_str(), 0, 0, &zErrMsg);

	  	if( error != SQLITE_OK ){
	  		fprintf(stderr, "SQL error: %s\n", zErrMsg);
	  	} else {
//	  		fprintf(stdout, "Row added successfully\n");
	  		ss.str("");
	  		ss.clear();
	  		ss << "SELECT * FROM \"" << hashed_name << "\";" << endl;
	  		sql = ss.str();
//	  		cout << "SQL:" << sql << endl;
	  		error = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);

		  	if( error != SQLITE_OK ){
//		  		fprintf(stderr, "User table doesnt exist: %s\n", zErrMsg);
		  		ss.str("");
		  		ss.clear();
		  		ss << "CREATE TABLE `" << hashed_name << "` (ip2 integer primary key);" << endl;
		  		sql = ss.str();
//		  		cout << "SQL:" << sql << endl;
			  	error = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);

//				if( error != SQLITE_OK ){
//				  	   fprintf(stderr, "User table doesnt created: %s\n", zErrMsg);
//				  	} else {
//				  	   fprintf(stdout, "User table created\n");
//				  	}

		  	}
//		  	else {
//		  	   fprintf(stdout, "User table exist\n");
//		  	}

		  	ss.str("");
		  	ss.clear();
		  	ss << "INSERT INTO `" << hashed_name << "` (ip2) values (" << parse_ip2(&params[2]) << ");" << endl;
		  	sql = ss.str();
//		  	cout << "SQL:" << sql << endl;
		  	error = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);

		  	if( error != SQLITE_OK ){
//		  	   fprintf(stderr, "SQL error: %s\n", zErrMsg);
		  	   result = -1;
		  	} else {
//		  	   fprintf(stdout," row added successfully\n");
		  	   result = 0;
		  	}
	  	}
	} else {
		if (params[0]=="isec") {
			hashed_name = HashLy(params[1].c_str());
			hashed_name2 = HashLy(params[3].c_str());

			ss.str("");
			ss.clear();
			ss << "select count(*) from (select ip2 from `" << hashed_name << "` intersect select ip2 from `" << hashed_name2 << "`);" << endl;
			sql = ss.str();
//			cout << "SQL:" << sql << endl;
			error = sqlite3_prepare_v2(db, sql.c_str(), 1000, &res, &tail);

			if (error != SQLITE_OK) {
//				fprintf(stdout,"We did not get any data!");
			    result=-1;
			}

			if (sqlite3_step(res) == SQLITE_ROW) {

			    if (sqlite3_column_int(res,0)>0) {
			    	result = 0;
			    } else {
			    	result = 1;
			    }
			}

			sqlite3_finalize(res);

		} else {
			result = -1; //Wrong request
		}

	}
	return result;
}

int main() {
    int i;
	int rc;
	sqlite3 *db;
	char *sql;
	char *zErrMsg = 0;

    pthread_t id[THREAD_COUNT];


	   rc = sqlite3_open("test.db", &db);

	   if( rc ) {
	      fprintf(stderr, "Check database [No]\n %s\n", sqlite3_errmsg(db));
	      return 0;
	   }else{
	      fprintf(stderr, "Check database [Ok]\n");
	   }

	   sql = "CREATE TABLE `iptables` (`user_id` INTEGER,`ip_address` INTEGER, `created_date` DATE DEFAULT (datetime('now','localtime')), PRIMARY KEY(user_id,ip_address));";

	   rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);

	  	if( rc != SQLITE_OK ){
	  	   fprintf(stderr, "SQL error: %s\n", zErrMsg);
	     	   sqlite3_free(zErrMsg);
	  	} else {
	  	   fprintf(stdout, "Table IPTABLE created successfully\n");
	  	}

    FCGX_Init();
    printf("________ADD USER IP ADDRESS_________\n");
    printf("Usage: http://127.0.0.1/add/ilya/127.0.0.1\n");
    printf("____________________________________\n");
    printf("______CALCULATE INTERSECTION________\n");
    printf("Usage: http://127.0.0.1/isec/ilya/guest\n");

    socketId = FCGX_OpenSocket(SOCKET_PATH, 20);
    if(socketId < 0) {return 1;}
    printf("Socket is opened\n");

    for(i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&id[i], NULL, doit, NULL);
    }

    for(i = 0; i < THREAD_COUNT; i++) {
        pthread_join(id[i], NULL);
    }

    sqlite3_close(db);
    return 0;
}
