CREATE TABLE mailaccountfolders ( 
    id INTEGER NOT NULL,
    foldertype INTEGER NOT NULL,
    folderid INTEGER NOT NULL,
    PRIMARY KEY (id, foldertype),
    FOREIGN KEY (id) REFERENCES mailaccounts(id),
    FOREIGN KEY (folderid) REFERENCES mailfolders(id));
