echo "*********"
echo "Start Sync: `date`"
rsync -rav --delete -e "ssh -l llsposto" ~/nachos-csci402/code/ llsposto@aludra.usc.edu:~/nachos-csci402/code 2>&1 >> /tmp/nachos_rsync.out
echo "End Sync: `date`"
echo "*********"