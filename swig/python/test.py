#!/usr/bin/python

# Import our module, living in the same directory as _sdbf_class.so
import sdbf_class

# Name a few standalone objects to hash
name = "sdbf_class.py"
name2 = "sdbf_class.pyc"

# Create new objects from these names, in "regular" non-block mode.
test1 = sdbf_class.sdbf(name,0)
test2 = sdbf_class.sdbf(name2,0)

# print out some vital statistics and the hash itself
print "test 1"
print test1.name()
print test1.size()
print test1.input_size()
print test1.to_string()

print "test 2"
print test2.name()
print test2.size()
print test2.input_size()
bar=test2.to_string()

print bar

#test2.print_sdbf(test2)

# Compare the two hashes and get back a score
score = test1.compare(test2,0)

# display our score
print test1.name(), " vs ", test2.name(),": ",score

# Block mode test:
name3 = "sdbf_class.py"
# Note that creating this object you must give the "real size" of the block
test3 = sdbf_class.sdbf(name3,16*1024)

# Shows name
print test3.name()

# Displays block-mode sdbf
#print test3.tostring(); // still working on these two
#print test3;
print test3.to_string()

# This needs to be filled with an existing test file... 
#myfile = sdbf_class.fopen("/home/candice/serverhome/newKitty.sdbf","r")
# add  in a loop here.
#test4= sdbf_class.sdbf(myfile);

#print test4.get_name()
#test4.print_sdbf(test4)

#sdbf_class.fclose(myfile)
