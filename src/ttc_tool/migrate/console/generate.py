#!/usr/bin/env python
#write by foryzhou to test distribute ttc's migrating
#2010-12-02

import sys
import os

__version__ = '$Revision: 0.1 $'
migrate_agent_port = "9495"

def main():
	"""Process command line parameters and run the conversion."""
	if len(sys.argv) < 5:
		print "usage:",sys.argv[0],"serverlist","operate","addlist", "name","migrate_config"
		sys.exit()
	serverlist_filename,operate,addlist_filename,name = sys.argv[1:5]
	serverlistold = readlist(serverlist_filename)
	serverlist = serverlistold.copy()
	addlist = readlist(addlist_filename)
	migrateconfig = {}
	if len(sys.argv) == 6:
		migrateconfig = readlist(sys.argv[5])

	mkdir(name)
	if operate == "add":
		serverlist.update(addlist)
	elif operate == "del":
		for k in addlist:
			del serverlist[k]

	#genrate new serverlist
	writelist(serverlist, name+"/"+serverlist_filename+"_new")
	generate_serverlist_xml(serverlist, name+"/"+serverlist_filename+"_new.xml")

	migrate_script_name = name+"/migrate.sh"
	migrate_script = open(migrate_script_name, "w")
	migrate_script.write("#!/usr/bin/env sh\n")
	if  operate == "add":
		for servername in serverlist:
			migrate_script.write("../../ttc_reload_cluster_nodes "+
				serverlist[servername]+" "+serverlist_filename+"_new.xml\n")
		for servername in addlist:
			migrate_script.write("../../ttc_set_node_state "+servername+" "+
				serverlist[servername]+" migrating\n")
	else:
		for servername in serverlistold:
                        migrate_script.write("../../ttc_reload_cluster_nodes "+
                                serverlistold[servername]+" "+serverlist_filename+"_new.xml\n")

	for sourcename in serverlistold:
		for targetname in serverlist:
			if targetname != sourcename: #don't migrate self to self..
				generate_command_xml(name,name, serverlist,
						sourcename,targetname,migrateconfig,serverlistold)
				cmd = "../../client "+serverlistold[sourcename].split(':')[0]
				cmd += " "+migrate_agent_port+" "+name+"_"+sourcename
				cmd += "_to_"+targetname+".xml\n"
				migrate_script.write(cmd)

	migrate_script.close()
	os.system("chmod +x "+migrate_script_name)

def readlist(filename):
	"""
		read nodename and address pair from file
	"""
	list = {}
	file = open(filename,"r")
	for line in file.readlines():
		linearr = line.split()
		if len(linearr) < 2:
			list[linearr[0]] = ""
		else:
			list[linearr[0]] = linearr[1]
	file.close()
	return list

def writelist(list,filename):
	"""
		write nodename and address pair to file
	"""
	file = open(filename,"w")
	for k,v in list.iteritems():
		line = k + " " + v + '\n'
		file.write(line)
	file.close()
	return list

def mkdir(name):
	cmd = "mkdir " + name
	os.system(cmd)

def generate_serverlist_xml(list,filename):
	"""
	write server list to xml file...and this xml can be send to ttc directly
	"""
	file = open(filename,"w")
	file.write('<?xml version="1.0" encoding="UTF-8"?>\n')
	file.write('<server_list>\n')
	for k,v in list.iteritems():
		line = '\t<server name="%s" address="%s"/>\n'%(k,v)
		file.write(line)
	file.write('</server_list>\n')

def generate_command_xml(path,name,serverlist,source,target,migrateconfig,oldserverlist):
	"""
	write migrate command to xml file...and this xml can be send to 
	migrate agent directly.
	path:the directory of dist file
	name:the operate name
	serverlist:servers in resource circle
	target:the target ttc
	source:the source ttc
	migrateconfig:attribute pair append to the request section
	oldserverlist: if command is delete ttc,you may need old serverlist to get the target ttc's ip
	"""
	operatename = path+"_"+source+"_to_"+target
	file = open(path+'/'+operatename+'.xml',"w")
	file.write('<?xml version="1.0" encoding="UTF-8"?>\n')
	#target= the ip address migrate command should send to
	if serverlist.has_key(source):
		requestinfo = '<request command="migrate" name="%s" target="%s"'% (
				operatename,serverlist[source])
	else:
		requestinfo = '<request command="migrate" name="%s" target="%s"'% (
				operatename,oldserverlist[source])
	for k,v in migrateconfig.iteritems():
		str = ' %s="%s"'%(k,v)
		requestinfo+=str
	requestinfo += '>\n'
	file.write(requestinfo)
	file.write('<serverlist>\n')
	for k,v in serverlist.iteritems():
		if k == target:
			line = '\t<server name="%s" need_migrate="yes"/>\n' % k
		else:
			line = '\t<server name="%s" need_migrate="no"/>\n' % k
		file.write(line)
	file.write('</serverlist>\n</request>')
	file.close()

if __name__ == "__main__":
	main()

