#include <stdint.h>
#include <stdlib.h>
#include "util.h"


int lcs( const char *s1, int len1, const char *s2, int len2) {
 
     int *curr = (int *)calloc( len2, sizeof(int));
     int *prev = (int *)calloc( len2, sizeof(int));
     int i, j, *swap = NULL;
     int max_substr = 0;
 
     for(i=0; i<len1; ++i) {
          for(j=0; j<len2; ++j) {
               if( s1[i] != s2[j]) {
                    curr[j] = 0;
               } else {
                    if(i == 0 || j == 0) {
                         curr[j] = 1;
                    } else {
                         curr[j] = 1 + prev[j-1];
                    }
                    max_substr = (max_substr < curr[j]) ? curr[j] : max_substr;
               }
          }
          swap=curr;
          curr=prev;
          prev=swap;
fprintf( stderr, "%d/%d\r", i, len1);
     }
     free( curr);
     free( prev);
     return max_substr;
}

int main( int argc, char **argv) {
	mapped_file_t *f1, *f2;
	uint64_t max_substr;
    
	f1 = mmap_file( argv[1], 0, 0);
	f2 = mmap_file( argv[2], 0, 0);
	
	if( f1->size <= f2->size) {
		max_substr = lcs( f1->buffer, f1->size, f2->buffer, f2->size);
		printf( "%s %s %lu %4.2f %s\n", argv[1], argv[2], max_substr, (double)max_substr/f1->size, argv[3]);	
	} else {
		max_substr = lcs( f2->buffer, f2->size, f1->buffer, f1->size);
		printf( "%s %s %lu %4.2f %s\n", argv[2], argv[1], max_substr, (double)max_substr/f2->size, argv[3]);	
	}
}
