# nachos-csci402
NACHOOOOS
Welcome to the shitty readme

# Set up public-private key ssh

Use `ssh-keygen` to generate keys (if ~/.ssh/id_rsa.pub doesn't already exist).  
Concatenate the contents of ~/.ssh/id_rsa.pub to ~/.ssh/authorized_keys on aludra.  
Hooray! You can ssh without password authentication!  
Just in case:  
`chmod 700 ~/.ssh`  
`chmod 600 ~/.ssh/authorized_keys`  

# To watch files  
Install fswatch: https://github.com/emcrisostomo/fswatch  
(hombrew or the like)  

Run this command (with nachosync.sh in your path, see below). It watches for file changes and runs the script nachosync.sh when a change occurs:  
`fswatch -0 /path/to/nachos-csci402/code | xargs -0 -n1 nachosync.sh &`  

Right now you probably have to run it each time you restart your computer or something. I was working on a way to have it run as a daemon but so far its unsuccessful. I'll keep you posted. 

# nachosync.sh  
Do this:  
`cp setup/nachosync.sh /usr/local/bin/nachosync.sh`  
`chmod a+x /usr/local/bin/nachosync.sh`

This command in nachosync.sh does the sync bit.  
`rsync -rav --delete -e "ssh -l <username>" /path/to/nachos-csci402/code/ <username>@aludra.usc.edu:path/to/nachos-csci402/code 2>&1 >> ~/rsync.log`
