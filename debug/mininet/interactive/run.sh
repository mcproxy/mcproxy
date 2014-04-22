#!/bin/bash
sudo su -c "killall ovs-controller; mn -c; python2 network.py"

