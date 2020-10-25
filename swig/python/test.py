#!/usr/bin/python
# 
# IMPORTANT: Any edits to sdbf.i will break these tests
#
# test.py - Unittests for the sdhash python swig interface
# @brad_anton 
# 2020-10-10
#
import sdbf_class
import unittest

from os.path import getsize

class TestSdbfClass(unittest.TestCase):
    TEST_FILE="sdbf.i"
    TEST_FILE_SDBF="sdbf:03:6:sdbf.i:2333:sha1:256:5:7ff:160:1:42:EBAAAAAIxgEAAAEAAAgAAAAAAAgEQAiAAAAAAAAAgAAQAQMAwSGAigQACCEIDhEACABAAAAIAAQAAAQAAgEBIAFAAgIIAkAEAAAMAUQAABAQCQAAQAAmAAACAAAAAGAAJAAkQgAEiAAgEAQQEAAAEAQBggAAAABAgCBgIAIAAAmCAAAGCAAECAgDAAABAAABASABUAAAAAAIAAAIEACCAIAAADAAECAAAABAEAABACEQAAAAAACEAAAhAQgAAUAABggAAAqIAAYAmAAIJBAEgAAQJDABgAMAIEAQEAAAACBAgACAgAAQQAVAgAAEAAGAAAAAAgwAEigAAAAIBgYBCA=="
    
    def check_sdbf(self, sdbf, expected_sdbf):
        parts = expected_sdbf.split(':')

        self.assertIsNotNone(sdbf)
        self.assertEqual(sdbf.to_string(), expected_sdbf)
        self.assertEqual(sdbf.name(), parts[3])
        self.assertEqual(len(sdbf.name()), int(parts[2])) # A little redundant
        # .size() should be bf_size * bf_count
        self.assertEqual(sdbf.size(), int(parts[6]) * int(parts[10]))
        self.assertEqual(sdbf.input_size(), int(parts[4]))

    def test_formated_string(self):
        s = sdbf_class.sdbf(self.TEST_FILE_SDBF)
        self.check_sdbf(s, "{}\n".format(self.TEST_FILE_SDBF))

    def test_read_file(self):
        s = sdbf_class.sdbf(self.TEST_FILE, 0)
        self.check_sdbf(s, "{}\n".format(self.TEST_FILE_SDBF))

    """
    This current doesn't work, but i *think* it could with a little swig typemap magic

    def test_read_filestream(self):
        size = getsize(self.TEST_FILE)
        with open(self.TEST_FILE, 'rb') as f:
            s = sdbf_class.sdbf(self.TEST_FILE, f, 0, size, None)
            self.check_sdbf(s, "{}\n".format(self.TEST_FILE_SDBF))
    """

    def test_read_cstring(self):
        with open(self.TEST_FILE, 'rb') as f:
            data = f.read()
            s = sdbf_class.sdbf(self.TEST_FILE, data, 0, len(data), None)
            self.check_sdbf(s, "{}\n".format(self.TEST_FILE_SDBF))

if __name__ == '__main__':
    unittest.main()
