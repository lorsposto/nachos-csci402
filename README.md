# nachos-csci402
NACHOOOOS

ssh without authentication (public-private key)
keygen
create .ssh/authorized_keys
chmod 700 .ssh
chmod 600 authorized_keys

fswatch -0 . | xargs -0 -n1 nachosync.sh
rsync -rav --delete -e "ssh -l <username>" ~/nachos-csci402/code/ <username>@aludra.usc.edu:~/rsynctest 2>&1 >> ~/rsync.log
