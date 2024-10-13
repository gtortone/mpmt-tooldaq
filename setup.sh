#!/bin/bash

### change with your ToolFramework ToolDAQFramework base directories
export TOOLFW_BASEDIR=/home/tortone/devel/HyperK/tooldaq/lib/ToolFrameworkCore
export TOOLDAQ_BASEDIR=/home/tortone/devel/HyperK/tooldaq/lib/ToolDAQFramework
export DAQINTERFACE_BASEDIR=/home/tortone/devel/HyperK/tooldaq/lib/libDAQInterface
###

export TOOLFW_INCDIR=$TOOLFW_BASEDIR/include
export TOOLFW_LIBDIR=$TOOLFW_BASEDIR/lib
export TOOLDAQ_INCDIR=$TOOLDAQ_BASEDIR/include
export TOOLDAQ_LIBDIR=$TOOLDAQ_BASEDIR/lib
export TOOLDAQ_TEMPLATEDIR=$TOOLDAQ_BASEDIR/UserTools/template
export TOOLFW_TEMPLATEDIR=$TOOLFW_BASEDIR/UserTools/template
export DAQINTERFACE_INCDIR=$DAQINTERFACE_BASEDIR/include
export DAQINTERFACE_LIBDIR=$DAQINTERFACE_BASEDIR/lib

export LD_LIBRARY_PATH=$TOOLFW_LIBDIR:$TOOLDAQ_LIBDIR:$DAQINTERFACE_LIBDIR:$LD_LIBRARY_PATH

alias NodeDaemon=$TOOLDAQ_BASEDIR/NodeDaemon
alias RemoteControl=$TOOLDAQ_BASEDIR/RemoteControl
