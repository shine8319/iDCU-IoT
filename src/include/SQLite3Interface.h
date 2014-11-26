int IoT_sqlite3_open( char *path, sqlite3 **pSQLite3 );
int IoT_sqlite3_customize( sqlite3 **pSQLite3 );
int IoT_sqlite3_close( sqlite3 **pSQLite3 );
int IoT_sqlite3_transaction( sqlite3 **pSQLite3, char status );
int IoT_sqlite3_update( sqlite3 **pSQLite3, char* query );
SQLite3Data IoT_sqlite3_select( sqlite3 *pSQLite3, char *query );
