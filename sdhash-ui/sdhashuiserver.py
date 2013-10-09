#!python
# -*- coding: utf-8 -*-
# author: candice quates
# python handler for sdhash-server 
# see last lines of file for changing run stat
import cherrypy
import serverui

import os.path
import os
import sys

def we_are_frozen():
    """Returns whether we are frozen via py2exe.
    This will affect how we find out where we are located."""
    return hasattr(sys, "frozen")

def module_path():
    """ This will get us the program's directory,
    even if we are frozen using py2exe"""
    if we_are_frozen():
        return os.path.dirname(unicode(sys.executable, sys.getfilesystemencoding( )))
    return os.path.dirname(unicode(__file__, sys.getfilesystemencoding( )))   

chconf = os.path.join(module_path(),'cherrypy.conf')
if __name__ == '__main__':
    cherrypy.quickstart(serverui.SdhashSrv(),config=chconf)
else:
    cherrypy.tree.mount(serverui.SdhashSrv(),config=chconf)
