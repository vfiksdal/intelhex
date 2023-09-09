#include <stdio.h>
#include "ihex.h"

int main(int argc,char *argv[]){
    for(unsigned int i=1;i<argc;i++){
        memory_t memory;
        memory_init(&memory);
        printf("Parsing %s\r\n",argv[i]);
        int n=parse_file(argv[i],&memory);
        memory_t *head=&memory;
        printf("Parsed %d bytes in %d segments with %d records\r\n",
                memory_size(head),
                memory_count(head),n);
        for(int i=0;i<memory_count(&memory);i++){
            int address=head->segment->address;
            int size=head->segment->size;
            printf("Segment #%d (0x%X - 0x%X):",i+1,address,address+size);
            for(int j=0;j<size;j++){
                if(j%16==0){
                    printf("\r\n\t");
                }
                printf("%02X ",segment_get(head->segment,j));
            }
            head=head->next;
            printf("\r\n");
        }
        memory_free(&memory);
    }
}

