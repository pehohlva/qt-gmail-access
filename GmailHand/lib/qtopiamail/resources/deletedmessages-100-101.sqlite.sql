ALTER TABLE deletedmessages ADD COLUMN parentfolderid INTEGER REFERENCES mailfolders(id);
