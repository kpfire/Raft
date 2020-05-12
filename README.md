# Raft
Pengcheng Rong \
Aaron Chang

### Build and Run
```make``` \
```./raft.o```

### Command Line Interface
StartRaft [num_servers] [timeout_type]
   - num_servers (int)             | number of initial server threads to create
   - timeout_type (int, default=0) | 0 for deterministic election timeouts, any other int for random

Sleep [num_seconds]

CrashServer [server_id] \
RestartServer [server_id]
   - server_id (int)               | server id to crash

Request [server_id] [key] [value]
   - key (string)                  | key for state machine, i.e. X
   - value (int, optional)         | leave blank for read request, provide int for delta to apply to state machine

ConfigChange [server_id] [ids]
   - ids (string)                  | list of server ids to change configuration to, i.e. 0,6,3,2,100
