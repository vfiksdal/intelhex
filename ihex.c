#include <stdio.h>
#include <string.h>
#include "ihex.h"

logger_t ihexlogger=defaultlogger;

/*!\brief Default logger callback
 * \param level Severity of message
 * \param msg Message to print
 */
void defaultlogger(loglevel_t level,char *msg,...){
    if(level>LL_DEBUG){
        va_list arg;
        va_start(arg,msg);
        vprintf(msg,arg);
        printf("\r\n");
    }
}

/*!\brief Parses and converts n bytes of a hex string to an integer
 * \param data String with hexadecimal codes
 * \param length Number of bytes to convert
 * \return Integer value
 */
int parse_hex(char *data,unsigned int length){
    char d[length+1];
    memcpy(d,data,length);
    d[length]=0;
    return strtol(d,0,16);
}

/*!\brief Parses a hex file
 * \param filename Path to the file to be processed
 * \param memory Memory object to be populated
 * \return Number of records processed
 */
int parse_file(char *filename,memory_t *memory){
    FILE *fd=fopen(filename,"rb");
    if(fd){
        fseek(fd,0,SEEK_END);
        int l=ftell(fd);
        fseek(fd,0,SEEK_SET);
        char buffer[l+1];
        fread(buffer,l,1,fd);
        fclose(fd);
        return parse_string(buffer,l,memory);
    }
    else{
        ihexlogger(LL_ERROR,"Could not open %s",filename);
        return -1;
    }
}

/*!\brief Parses a string of intelhex data
 * \param data String to process, must be in intelhex format
 * \param length Number of bytes in string
 * \param memory Memory object to be processed
 * \return Number of records processed
 */
int parse_string(char *data,unsigned int length,memory_t *memory){
    unsigned long base=0;
    unsigned int line=0;
    unsigned int p=0;
    int eof=0;
    int r=0;
    while(p<length){
        uint8_t c=data[p++];
        if(c==':' && length-p>8){
            uint8_t bytecount=parse_hex(data+p,2);
            uint16_t address=parse_hex(data+p+2,4);
            uint8_t rectype=parse_hex(data+p+6,2);
            line++;
            p+=8;
            if(length-p>=bytecount*2+2){
                if(eof){
                    ihexlogger(LL_WARNING,"Record after EOF in line #%d",line);
                }
                segment_t segment;
                segment_init(&segment,base+address,bytecount);
                uint8_t checksum=bytecount;
                checksum+=address>>8;
                checksum+=address&0xff;
                checksum+=rectype;
                for(uint8_t i=0;i<bytecount;i++){
                    segment_set(&segment,i,parse_hex(data+p,2));
                    checksum+=segment_get(&segment,i);
                    p+=2;
                }
                if((uint8_t)((~checksum)+1)==parse_hex(data+p,2)){
                    if(rectype==0){
                        // Normal data record
                        memory_add(memory,&segment);
                    }
                    else if(rectype==1){
                        // EOF record
                        eof=1;
                    }
                    else if(rectype==2){
                        // Extended segment address record
                        base=segment_get(&segment,0)<<12;
                        base+=segment_get(&segment,1)<<4;
                    }
                    else if(rectype==3){
                        // Start segment address record
                        memory->start=segment_get(&segment,0)<<8;
                        memory->start+=segment_get(&segment,1);
                    }
                    else if(rectype==4){
                        // Extended linear address record
                        base=(segment.buffer[0]<<24)+(segment.buffer[1]<<16);
                    }
                    else if(rectype==5){
                        // Start linear address record
                        memory->start=segment_get(&segment,0)<<24;
                        memory->start+=segment_get(&segment,1)<<16;
                        memory->start+=segment_get(&segment,2)<<8;
                        memory->start+=segment_get(&segment,3);
                    }
                    else{
                        ihexlogger(LL_ERROR,"Unsupported record in line #%d: %d",line,rectype);
                        segment_free(&segment);
                        return -1;
                    }
                    p+=2;
                    r++;
                }
                else{
                    ihexlogger(LL_ERROR,"Corrupted data in line #%d",line);
                    segment_free(&segment);
                    return -1;
                }
                segment_free(&segment);
            }
            else{
                ihexlogger(LL_ERROR,"Incomplete data in line #%d",line);
                return -1;
            }
        }
        else if(c!='\r' && c!='\n' && c!=' ' && c!='\t' && c!=0){
            ihexlogger(LL_WARNING,"Unexpected character %c in line #%d",c,line);
        }
    }
    memory_consilidate(memory);
#ifdef REQUIRE_EOF
    ihexlogger(LL_WARNING,"No EOF record in data");
    return -1;
#endif
    return r;
}

/*!\brief Initializes memory structure
 * \param memory Structure to initialize
 */
void memory_init(memory_t *memory){
    memset(memory,0,sizeof(memory_t));
}

/*!\brief Deallocates memory structure recursively
 * \param memory Memory structure to deallocate
 *
 * Note: Top-level object is not deallocated as this is typically created
 * on the stack. If you have allocated this on the heap you have to call
 * free on the top object after calling this method.
 */
void memory_free(struct memory_t *memory){
    memory_t *head=memory;
    while(head){
        if(head->segment){
            segment_free(head->segment);
            free(head->segment);
            head->segment=0;
        }
        head=head->next;
    }
    if(memory){
        head=memory->next;
        while(head){
            memory_t *tmp=head;
            head=head->next;
            free(tmp);
        }
    }
}

/*!\brief Counts segments in memory structure
 * \param memory Structure to iterate
 */
unsigned int memory_count(memory_t *memory){
    unsigned int r=0;
    memory_t *head=memory;
    while(head){
        if(head->segment){
            r++;
        }
        head=head->next;
    }
    return r;
}

/*!\brief Finds number of bytes in all segments held in memory
 * \param memory Memory structure to iterate
 * \return Number of bytes held in memory structure
 */
unsigned int memory_size(memory_t *memory){
    unsigned int r=0;
    memory_t *head=memory;
    while(head){
        if(head->segment){
            r+=head->segment->size;
        }
        head=head->next;
    }
    return r;
}

/*!\brief Consilidates memory segment which align
 * \param memory Memory structure to iterate
 */
void memory_consilidate(memory_t *memory){
    memory_t *head=memory;
    while(head){
        if(head->segment && head->next && head->next->segment){
            segment_t *s1=head->segment;
            segment_t *s2=head->next->segment;
            if(s1->address+s1->size == s2->address){
                // Consilidate the two segments
                ihexlogger(LL_DEBUG,"Merging segments at 0x%X and 0x%X",s1->address,s2->address);
                segment_t *ns=(segment_t*)malloc(sizeof(segment_t));
                segment_init(ns,s1->address,s1->size+s2->size);
                memcpy(ns->buffer,s1->buffer,s1->size);
                memcpy(ns->buffer+s1->size,s2->buffer,s2->size);
                segment_free(s1);
                segment_free(s2);
                free(s1);
                free(s2);
                head->segment=ns;

                // Drop segment from memory
                memory_t *next=head->next;
                head->next=next->next;
                free(next);
                continue;
            }
        }
        head=head->next;
    }
}

/*!\brief Adds a segment to memory object
 * \param memory Memory object to add the the segment to
 * \param segment Segment to add to memory object
 */
void memory_add(memory_t *memory,segment_t *segment){
    // Create a copy of the segment
    segment_t *ns=(segment_t*)malloc(sizeof(segment_t));
    segment_init(ns,segment->address,segment->size);
    memcpy(ns->buffer,segment->buffer,segment->size);

    // Add segment to the stack
    if(memory->segment==0){
        memory->segment=ns;
    }
    else{
        memory_t *head=memory;
        while(head->next){
            head=head->next;
        }
        head->next=(memory_t*)malloc(sizeof(memory_t));
        head=head->next;
        memory_init(head);
        head->segment=ns;
    }
}

/*!\brief Initialize segment
 * \param segment Segment to initialize
 * \param address Starting address of this segment
 * \param size Size of this segment
 */
void segment_init(segment_t *segment,uint32_t address,uint32_t size){
    segment->address=address;
    segment->size=size;
    if(size){
        segment->buffer=(uint8_t*)malloc(size);
    }
    else{
        segment->buffer=0;
    }
    if(!segment->buffer){
        segment->size=0;
    }
}

/*!\brief Free data from segment
 *
 * Note: The segment object itself is not freed. Only the buffer within.
 */
void segment_free(segment_t *segment){
    free(segment->buffer);
    segment->buffer=0;
    segment->size=0;
}

void segment_set(segment_t *segment,uint32_t index,uint8_t value){
    if(index<segment->size){
        segment->buffer[index]=value;
    }
}

uint8_t segment_get(segment_t *segment,uint32_t index){
    if(index<segment->size){
        return segment->buffer[index];
    }
    else{
        return 0;
    }
}

