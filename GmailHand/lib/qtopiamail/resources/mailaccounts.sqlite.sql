CREATE TABLE mailaccounts( 
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    type INTEGER NOT NULL,
    name VARCHAR,
    emailaddress VARCHAR,
    status INTEGER,
    signature VARCHAR);
