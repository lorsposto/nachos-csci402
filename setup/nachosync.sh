echo RSYNCING
rsync -rav --delete -e "ssh -l llsposto" ~/nachos-csci402/code/ llsposto@aludra.usc.edu:~/rsynctest 2>&1 >> /tmp/rsync.out