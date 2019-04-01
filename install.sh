#!/bin/bash
# 
# Copyright (c) 2019 Gary Kim <gary@garykim.dev>
# Licensed under MIT License
#
# Easily run this script by running 
# `curl -sSL --output chernobotinstaller.sh https://raw.githubusercontent.com/hyperum/chernobot/master/install.sh && grep '#!/bin/bash' chernobotinstaller.sh > /dev/null && bash ./chernobotinstaller.sh dl`

set -e

if [[ "$OSTYPE" == "darwin" ]]; then
    echo -e 'Detected Darwin (macOS) OS\n'
    
    # Check if xcode command line tools is installed
    xcode-select --version > /dev/null || echo -e 'XCode command line tools does not seem to be installed\nIf you would like to install it, run this command\nxcode-select install\nthen rerun this script. Warning: This will take a very long time to install.\n' && exit 1

    # Check if Homebrew is installed
    brew --version > /dev/null || echo -e 'Homebrew is not installed\nIf you would like to install it, run this command\n/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"\nthen rerun this script.\n' && exit 1

    # Check if git is avaliable
    git --version > /dev/null || echo -e 'Git is not installed\nIf you would like to install it, run this command\nbrew install git\nthen rerun this script.\n' && exit 1

    if [[ "$1" == "dl" ]] || [[ "$1" == "download" ]]; then
        git clone --recursive https://github.com/hyperum/chernobot.git chernobot
        cd chernobot
        brew install sdl2 ninja cmake opencv
        mkdir -p tmp
        cd tmp
        cmake -G Ninja ..
        ninja install
        cd ..
    elif [[ "$1" == "update" ]]; then
        git pull
        git submodule update --init --recursive
        mkdir -p tmp
        cd tmp
        cmake -G Ninja ..
        ninja install
        cd ..
    else
        mkdir -p tmp
        cd tmp
        cmake -G Ninja ..
        ninja install
        cd ..
    fi

    echo -e 'Chernobot has been installed. You should find a file named "chernobot" in the chernobot folder.'
    
    exit 0

elif [[ "$OSTYPE" == "linux-gnu" ]]; then
    echo -e "Detected GNU/Linux OS\n"

    echo -e 'Linux support is not yet avaliable. It will be coming in the future\n'
    exit 1
    # TODO: Make GNU/Linux install script

    if [ "$(cat /etc/os-release | grep -icP '(ID\_LIKE\=ubuntu|ID=ubuntu)')" != 1 ]; then
        echo -e 'Unfortunatly, there is not support for your distro. Currently, only Ubuntu and Ubuntu deriviatives are supported\n'
        exit 1
    fi
    
else
    echo -e "OS not recognized\n"
fi
