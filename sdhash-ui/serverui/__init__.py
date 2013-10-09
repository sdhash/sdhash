#!python
# -*- coding: utf-8 -*-
# author: candice quates
# python root class for sdhash-server 
# note connection for sdhash-srv in serverconf.py

import view
import cherrypy
from serverconf import *


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

current_dir = module_path()

from jinja2 import Environment, FileSystemLoader
env = Environment(loader=FileSystemLoader(current_dir+'/templates'))

class SdhashSrv:
    @cherrypy.expose
    def _get_listing(self, _ = None):
        return view.listing()

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def _get_results_list(self):
        return view.getresultbyname('web')

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def _get_result(self,id = 0, _ = None):
        resid = int(id)
        return dict(aaData=view.getresultbyid(resid))

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def _hashset(self,id = 0, _ = None):
        setid = int(id)
        return dict(aaData=view.hashsetdisplay(setid))

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def _files(self, _ = None):
        return dict(aaData=view.filelisting())

    @cherrypy.expose
    def _res_status(self, id=-1):
        resid = int(id)
        return view.getresultstatus(resid)

    @cherrypy.expose
    def _hashset_status(self, id=-1):
        setid = int(id)
        return view.gethashsetstatus(setid)

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def _compare(self, set1=-1, set2=-1,samp=0, thr=1):
        s1 = int(set1)
        s2 = int(set2)
        sample =int(samp)
        threshold = int(thr)
        return dict(resid=view.hashsetcompare2(s1,s2,threshold,sample))

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def _compare_all(self, set1=-1, thr=1):
        s1 = int(set1)
        threshold = int(thr)
        return dict(resid=view.hashsetcompare(s1,threshold))

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def _remove(self,id=0):
        resid = int(id)
        return dict(status=view.removeresult(resid));

    @cherrypy.expose
    @cherrypy.tools.json_out()
    def _hash(self,hashsetname='', sourcefiles=''):
        fname = hashsetname
        rawfilelist = sourcefiles
        return dict(resid=view.createhashset(fname,rawfilelist))

    def index(self):
        tmpl = env.get_template('index.html')
        return tmpl.render(listing=view.listing(), results=view.getresultbyname('web'), status=host+":"+str(port))
    index.exposed = True;

