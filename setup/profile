# /etc/profile

# System wide environment and startup programs
# Functions and aliases go in /etc/bashrc

HOSTNAME=`/bin/hostname`
HISTSIZE=1000

export PATH HOSTNAME HISTSIZE

for i in /etc/profile.d/*.sh ; do
    if [ -r "$i" ]; then
        . $i
    fi
done
