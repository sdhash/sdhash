/* sdbf_test.cc shortest possible test program for sdbf
   author: candice quates

 g++ -fopenmp sdbf_test.cc -o sdbf_test -I../external ../libsdbf.a -L../external/stage/lib -lcrypto -lc -lm -lboost_system -lboost_filesystem -lboost_thread -lpthread 

*/


#include <iostream>
#include <stdint.h>

#include "../sdbf/sdbf_class.h"
#include "../sdbf/sdbf_defines.h"

using namespace std;

int
main() {
    uint32_t res1;
	/// create new sdbf from binary of built sdhash file, no parallelism
    sdbf *test1 = new sdbf("../sdhash", 0);
	/// create new sdbf from binary of Doxygen file, 16KB to a bloom filter
    sdbf *test2 = new sdbf("../Doxyfile", 16*1024);

	/// display our hashes
	cout << test1 ;
	cout << test2 ;
	/// Compare test 1 with test2, and print the resulting score.
    res1=test1->compare(test2,0);

	uint8_t* stuff;
    cout << "test1 vs test2 " << res1 << "\n";
    string foo=test2->to_string();
    sdbf *test3=new sdbf(foo);
    sdbf *test4=new sdbf(test2->to_string());
    res1=test3->compare(test4,0);
    cout << "test3 vs test4 " << res1 << "\n";
    cout << test3->to_string();
    return 0;
}
