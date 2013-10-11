UPDATE mailaccounts SET status = status | ( SELECT (1 << (max(statusbit) + 0)) FROM mailstatusflags WHERE context = 'accountstatus' ) WHERE type <> 3 AND type <> 5;
UPDATE mailaccounts SET status = status | ( SELECT (1 << (max(statusbit) + 1)) FROM mailstatusflags WHERE context = 'accountstatus' ) WHERE (type == 0 OR type == 1);
