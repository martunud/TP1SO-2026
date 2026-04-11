#!/bin/bash
apt-get update && apt-get install -y libncurses-dev ncurses-dev > /dev/null 2>&1
exec bash
