import serverconf

from thrift.transport import TTransport
from thrift.transport import TSocket
from thrift.transport import THttpClient
from thrift.protocol import TBinaryProtocol
import thrift

import sdhashsrv
from sdhashsrv import *
from sdhashsrv.ttypes import *
from sdhashsrv.constants import *
from sdhashsrv.sdhashsrv import *

from serverconf import *

import json

def listing():
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    try:
        transport.open()
        result=client.setList(True)
        transport.close()
    except Exception:
        result="Connection Failed"
        pass
    return result

def hashsetdisplay(num):
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    result=client.displaySet(num)
    transport.close()
    i = 0;
    output=[]
    middle=result.split("\n")
    reslist=sorted(middle, key=str.lower)    
    for val in reslist:
        if (i>0):
            items=[]
            items=val.rsplit(None,1)    
            output.append((i,items[0],items[1]))
        i=i+1
    return output 

def hashsetcompare(num,thresh):
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    resid=client.createResultID("web")
    client.compareAll(num,thresh,resid)
    transport.close()
    return resid

def hashsetcompare2(num,num2,thresh,sample):
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    resid=client.createResultID("web")
    client.compareTwo(num,num2,thresh,sample,resid)
    transport.close()
    return resid

def getresultbyid(resid):
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    result=client.displayResult(resid)
    queryset='query'
    targetset='target'
    info=client.displayResultInfo(resid)
    stuff=info.split()
    if (info.count('--')==1):
        queryset=stuff[0]
        targetset=stuff[2]         
    else:
        queryset=stuff[0]
        targetset=queryset    
    output=[]
    header=[]
    header.append('queryset')
    header.append('query')
    header.append('targetset')
    header.append('target')
    header.append('score')
    for line in result.split('\n'):
        cols = line.split('|')
        items = []
        cforw = cols[0].count('/')    
        cback = cols[0].count('\\')    
        if (len(cols) == 3): 
            items.append(queryset)
            if (len(cols[0]) >50):
                if (cback > 0):
                    fileparts=cols[0].rsplit('\\',3)
                    items.append('...\\'+fileparts[1]+'\\'+fileparts[2]+'\\'+fileparts[3])
                if (cforw > 0):
                    fileparts=cols[0].rsplit('/',3)
                    items.append('.../'+fileparts[1]+'/'+fileparts[2]+'/'+fileparts[3])
                else:
                    items.append('...'+cols[0][-50:])
            else:
                items.append(cols[0])
            items.append(targetset)
            cforw = cols[1].count('/')    
            cback = cols[1].count('\\')    
            if (len(cols[1]) >50):
                if (cback > 0):
                    fileparts=cols[1].rsplit('\\',3)
                    items.append('...\\'+fileparts[1]+'\\'+fileparts[2]+'\\'+fileparts[3])
                if (cforw > 0):
                    fileparts=cols[1].rsplit('/',3)
                    items.append('.../'+fileparts[1]+'/'+fileparts[2]+'/'+fileparts[3])
            else:
                items.append(cols[1])
            items.append(cols[2])
            output.append(items)
    transport.close()
    return output

def getresultbyname(name):
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    result=client.displayResultsList(name,1) # use json output
    transport.close()
    parsed=json.loads(result)
    parsed.reverse()
    output=[]
    for val in parsed:
        indexnum=val.items()[0][0]
        name=unicode.decode(val.items()[0][1])
        output.append((indexnum,name))
    return output

def filelisting():
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    result=client.displaySourceList()
    transport.close()
    output=[]
    i=0
    reslist=sorted(result, key=str.lower)    
    for val in reslist:
        output.append((i,val))
        i+=1
    return output

def createhashset(name,rawlist):
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    setID=client.createHashsetID()
    fixed=str(rawlist)
    filelist=fixed.split('|')
    print rawlist
    print filelist
    # index searching off until have ui changes
    client.hashString(name,filelist,0,setID,0)
    transport.close()
    return setID 

def getresultstatus(resID):
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    result=client.displayResultStatus(resID)
    time=client.displayResultDuration(resID)
    transport.close()
    return result+' '+time

def gethashsetstatus(hashsetID):
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    result=client.getHashsetName(hashsetID)
    if (result==''):
        output='processing hashset #'+str(hashsetID)
    else:
        output='completed hashset '+result
    transport.close()
    return output

def removeresult(resID):
    socket = TSocket.TSocket(host, port)
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = sdhashsrv.Client(protocol)
    transport.open()
    client.removeResult(resID)
    transport.close()
    return True 
