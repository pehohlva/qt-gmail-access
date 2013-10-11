UPDATE mailmessages SET parentfolderid=1 WHERE parentfolderid IN (2,3,4,5);
UPDATE mailmessages SET previousparentfolderid=1 WHERE previousparentfolderid IN (2,3,4,5);
DELETE FROM mailfolders WHERE id IN (2,3,4,5);
UPDATE mailfolders SET displayname='Local Storage' WHERE id=1;
