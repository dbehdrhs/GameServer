#include "stdafx.h"

#include "mysql.h"
#pragma comment(lib, "libmysql.lib")

#include "CDB.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//#include "D:\SVN\git\GameServer\Server\GameServer\mysql\include\cppconn\driver.h"
//#include "D:\SVN\git\GameServer\Server\GameServer\mysql\include\cppconn\exception.h"
//#include "D:\SVN\git\GameServer\Server\GameServer\mysql\include\cppconn\resultset.h"
//#include "D:\SVN\git\GameServer\Server\GameServer\mysql\include\cppconn\statement.h"
//#include "D:\SVN\git\GameServer\Server\GameServer\mysql\include\cppconn\prepared_statement.h"
//
//
//#pragma comment(lib, "./mysql/lib64/vs14/mysqlcppconn.lib")
//////////////////////////////////////////////////////////////////////////////////////////////////////

//const string server = "tcp://localhost:3306";
//const string username = "root";
//const string password = "rlaclWla1!";
//const string schema = "test";


CDB::CDB()
	: byThreadCount(1)
{
}


CDB::~CDB()
{
}

BOOL CDB::Start()
{
	HANDLE hThread = INVALID_HANDLE_VALUE;
	DWORD dwThreadID = 0;

	if (byThreadCount < 1)
	{
		return FALSE;
	}

	for (int i = 0; i < byThreadCount; i++)
	{
		if (hThread = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(WorkerThread), reinterpret_cast<LPVOID>(this), 0, &dwThreadID))
		{
			printf("[%u] GameServer Thread Start\n", dwThreadID);
			CloseHandle(hThread);
		}
		else
		{
			Stop();
			return FALSE;
		}
	}
	

	return TRUE;
}

BOOL CDB::Stop()
{
	return TRUE;
}

void CDB::SendDbResult(WORD wID)
{
}

void CDB::WorkerThread(LPVOID lpParam)
{
	reinterpret_cast<CDB*>(lpParam)->Worker();
}

BOOL CDB::Worker()
{
	TestC();

	while (TRUE)
	{

	}
	return TRUE;
}

void CDB::TestC()
{
	printf("MysqlClientVision:%s\n", mysql_get_client_info());

	MYSQL mysql;
	mysql_init(&mysql);
	if (false == mysql_real_connect(&mysql, "127.0.0.1", "root", "rlaclWla1!", "accountdb", 3306, NULL, 0))
	{
		printf("error:%s\n", mysql_error(&mysql));
	}

	if (mysql_query(&mysql, "SELECT name, password, logintime, createtime FROM useraccount"))
	{
		printf("%s\n", mysql_error(&mysql));
	}

	MYSQL_RES* result = mysql_store_result(&mysql);
	MYSQL_ROW row;
	while (row = mysql_fetch_row(result))
	{
		printf("name:%s, password:%s, logintime:%s, createtime:%s\n", row[0], row[1], row[2], row[3]);
	}

	mysql_free_result(result);
}

//void CDB::TestInsert()
//{
//	sql::Driver *driver = NULL;
//	sql::Connection *con = NULL;
//	sql::Statement *stmt = NULL;
//	sql::PreparedStatement *pstmt = NULL;
//
//	try
//	{
//		driver = get_driver_instance();
//		con = driver->connect(server, username, password);
//	}
//	catch (sql::SQLException e)
//	{
//		cout << "Could not connect to server. Error message: " << e.what() << endl;
//		system("pause");
//		exit(1);
//	}
//
//	//please create database "quickstartdb" ahead of time
//	con->setSchema("quickstartdb");
//
//	stmt = con->createStatement();
//	stmt->execute("DROP TABLE IF EXISTS inventory");
//	cout << "Finished dropping table (if existed)" << endl;
//	stmt->execute("CREATE TABLE inventory (id serial PRIMARY KEY, name VARCHAR(50), quantity INTEGER);");
//	cout << "Finished creating table" << endl;
//	delete stmt;
//
//	pstmt = con->prepareStatement("INSERT INTO inventory(name, quantity) VALUES(?,?)");
//	pstmt->setString(1, "banana");
//	pstmt->setInt(2, 150);
//	pstmt->execute();
//	cout << "One row inserted." << endl;
//
//	pstmt->setString(1, "orange");
//	pstmt->setInt(2, 154);
//	pstmt->execute();
//	cout << "One row inserted." << endl;
//
//	pstmt->setString(1, "apple");
//	pstmt->setInt(2, 100);
//	pstmt->execute();
//	cout << "One row inserted." << endl;
//
//	delete pstmt;
//	delete con;
//	system("pause");
//}
//
//void CDB::TestSelect()
//{
//	sql::Driver *driver;
//	sql::Connection *con;
//	sql::PreparedStatement *pstmt;
//	sql::ResultSet *result;
//
//	try
//	{
//		driver = get_driver_instance();
//		//for demonstration only. never save password in the code!
//		con = driver->connect(server, username, password);
//	}
//	catch (sql::SQLException e)
//	{
//		cout << "Could not connect to server. Error message: " << e.what() << endl;
//		system("pause");
//		exit(1);
//	}
//
//	con->setSchema("quickstartdb");
//
//	//select  
//	pstmt = con->prepareStatement("SELECT * FROM inventory;");
//	result = pstmt->executeQuery();
//
//	while (result->next())
//		printf("Reading from table=(%d, %s, %d)\n", result->getInt(1), result->getString(2).c_str(), result->getInt(3));
//
//	delete result;
//	delete pstmt;
//	delete con;
//	system("pause");
//}
//
//void CDB::TestUpdate()
//{
//	sql::Driver *driver;
//	sql::Connection *con;
//	sql::PreparedStatement *pstmt;
//
//	try
//	{
//		driver = get_driver_instance();
//		//for demonstration only. never save password in the code!
//		con = driver->connect(server, username, password);
//	}
//	catch (sql::SQLException e)
//	{
//		cout << "Could not connect to server. Error message: " << e.what() << endl;
//		system("pause");
//		exit(1);
//	}
//
//	con->setSchema("quickstartdb");
//
//	//update
//	pstmt = con->prepareStatement("UPDATE inventory SET quantity = ? WHERE name = ?");
//	pstmt->setInt(1, 200);
//	pstmt->setString(2, "banana");
//	pstmt->executeQuery();
//	printf("Row updated\n");
//
//	delete con;
//	delete pstmt;
//	system("pause");
//}
