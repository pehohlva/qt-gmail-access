UPDATE mailaccounts SET status = status | ( SELECT (1 << max(statusbit)) FROM mailstatusflags WHERE context = 'accountstatus' );
