
int _sqlite3_update( sqlite3 **pSQLite3, MGData node );
SQLite3Data _sqlite3_select( sqlite3 *pSQLite3, char *query );

int _sqlite3_open( char *path, sqlite3 **pSQLite3 );
int _sqlite3_close( sqlite3 **pSQLite3 );

int _sqlite3_customize( sqlite3 **pSQLite3 );
int _sqlite3_transaction( sqlite3 **pSQLite3, char status );
int _sqlite3_nolock( sqlite3 **pSQLite3 );
