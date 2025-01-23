#include "DB.h"

bool DB_odbc(int c_id, const char* name) {
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;

	// Temporary buffer to hold the SQL command
	char temp[200];
	sprintf_s(temp, sizeof(temp), "EXEC [2020180047_GameServer].[dbo].get_user_data '%s'", name);

	// Convert the SQL command to wide char format
	wchar_t* exec;
	int str_size = MultiByteToWideChar(CP_ACP, 0, temp, -1, NULL, NULL);
	exec = new WCHAR[str_size];
	MultiByteToWideChar(CP_ACP, 0, temp, -1, exec, str_size);

	// Variables to hold fetched data
	SQLWCHAR p_id[20];
	SQLINTEGER p_x, p_y, p_exp, p_hp, p_hpitemcount, p_wepitemcount, p_level, p_atk;
	SQLLEN cbP_ID = 0, cbP_X = 0, cbP_Y = 0, cbP_EXP = 0, cbP_HP = 0, cbP_HPITEMCOUNT = 0,
		cbP_WEPITEMCOUNT = 0, cbP_LEVEL = 0, cbP_ATK = 0;

	// Allocate environment handle
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

		// Allocate connection handle
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2024GraduationWork", SQL_NTS, NULL, 0, NULL, 0);

				// Allocate statement handle
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					//  cout << "ODBC Connection Success" << endl;
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
					retcode = SQLExecDirect(hstmt, (SQLWCHAR*)exec, SQL_NTS);

					// Bind columns
					retcode = SQLBindCol(hstmt, 1, SQL_C_WCHAR, p_id, sizeof(p_id), &cbP_ID);
					retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &p_x, sizeof(p_x), &cbP_X);
					retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &p_y, sizeof(p_y), &cbP_Y);
					retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &p_exp, sizeof(p_exp), &cbP_EXP);
					retcode = SQLBindCol(hstmt, 5, SQL_C_LONG, &p_hp, sizeof(p_hp), &cbP_HP);
					retcode = SQLBindCol(hstmt, 6, SQL_C_LONG, &p_hpitemcount, sizeof(p_hpitemcount), &cbP_HPITEMCOUNT);
					retcode = SQLBindCol(hstmt, 7, SQL_C_LONG, &p_wepitemcount, sizeof(p_wepitemcount), &cbP_WEPITEMCOUNT);
					retcode = SQLBindCol(hstmt, 8, SQL_C_LONG, &p_level, sizeof(p_level), &cbP_LEVEL);
					retcode = SQLBindCol(hstmt, 9, SQL_C_LONG, &p_atk, sizeof(p_atk), &cbP_ATK);

					// Fetch user data
					retcode = SQLFetch(hstmt);

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						// Update client data if user exists
						strcpy_s(clients[c_id]._name, name);
						clients[c_id].x = p_x;
						clients[c_id].y = p_y;
						clients[c_id]._hp = p_hp;
						clients[c_id]._exp = p_exp;
						clients[c_id].hpitemcount = p_hpitemcount;
						clients[c_id].WepitemCount = p_wepitemcount;
						clients[c_id]._level = p_level;
						clients[c_id]._atk = p_atk;

						//    cout << "User data retrieved successfully." << endl;
						return true;
					}
					else {
						//  cout << "Failed to fetch user data." << endl;
						return false;
					}
				}
				else {
					//    cout << "Failed to allocate statement handle." << endl;
				}
				SQLDisconnect(hdbc);
			}
			else {
				//    cout << "Failed to connect to data source." << endl;
			}
			SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		}
		else {
			//   cout << "Failed to allocate connection handle." << endl;
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
	else {
		// cout << "Failed to allocate environment handle." << endl;
	}

	return false;
}

bool DB_save(int c_id) {
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2024GraduationWork", SQL_NTS, NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					std::wcout << L"ODBC Connection Success\n";
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					// Prepare the SQL statement
					const wchar_t* sql = L"EXEC [2020180047_GameServer].[dbo].save_user_data ?, ?, ?, ?, ?, ?, ?, ?, ?";
					retcode = SQLPrepare(hstmt, (SQLWCHAR*)sql, SQL_NTS);

					// Bind parameters
					SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_WCHAR, 20, 0, clients[c_id]._name, 0, NULL);
					SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &clients[c_id].x, 0, NULL);
					SQLBindParameter(hstmt, 3, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &clients[c_id].y, 0, NULL);
					SQLBindParameter(hstmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &clients[c_id]._exp, 0, NULL);
					SQLBindParameter(hstmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &clients[c_id]._max_hp, 0, NULL);
					SQLBindParameter(hstmt, 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &clients[c_id].hpitemcount, 0, NULL);
					SQLBindParameter(hstmt, 7, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &clients[c_id].WepitemCount, 0, NULL);
					SQLBindParameter(hstmt, 8, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &clients[c_id]._level, 0, NULL);
					SQLBindParameter(hstmt, 9, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &clients[c_id]._atk, 0, NULL);

					// Execute the statement
					retcode = SQLExecute(hstmt);

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						std::wcout << L"User data inserted successfully.\n";
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}
					else {
						//std::wcout << L"Failed to insert user data.\n";
					}

					SQLDisconnect(hdbc);
				}
				else {
					//std::wcout << L"ODBC Connection Failed\n";
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}

	return (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO);
}

