#include <stdio.h>
#include "ihex.h"

int passed=0;
int failed=0;

void test(const char *text,int result){
    printf("%-60s%s\r\n",text,result?"PASS":"FAIL");
    if(result){
        passed++;
    }
    else{
        failed++;
    }
}

void test_memory_consilidation(){
    memory_t m;
    segment_t s1;
    segment_t s2;
    segment_t s3;
    memory_init(&m);
    segment_init(&s1,0,3);
    segment_init(&s2,3,3);
    segment_init(&s3,6,3);
    s1.buffer[0]=1;
    s1.buffer[1]=2;
    s1.buffer[2]=3;
    s2.buffer[0]=4;
    s2.buffer[1]=5;
    s2.buffer[2]=6;
    s3.buffer[0]=7;
    s3.buffer[1]=8;
    s3.buffer[2]=9;
    memory_add(&m,&s1);
    memory_add(&m,&s2);
    memory_add(&m,&s3);
    test("Memory containt three segments",memory_count(&m)==3);
    test("Unconsilidated memory has all data",memory_size(&m)==9);
    memory_consilidate(&m);
    test("Memory consilidated to one segment",memory_count(&m)==1);
    test("Consilidated memory has all data",memory_size(&m)==9);
    for(int i=0;i<9;i++){
        test("Consilidated memory has expected data",m.segment->buffer[i]==i+1);
    }
    memory_free(&m);
    segment_free(&s1);
    segment_free(&s2);
    segment_free(&s3);
}

void test_parse_data(){
    memory_t m;
    memory_init(&m);
    int r=parse_string(":0B0010006164647265737320676170A7",33,&m);
    test("Parser returned one for one record",r==1);
    test("Parsed hex string returned one segment",memory_count(&m)==1);
    test("Parsed segment has correct size",memory_size(&m)==11);
    test("Parsed segment has correct address",m.segment->address==16);
    memory_free(&m);
}

void test_parse_eof(){
    memory_t m;
    memory_init(&m);
    int r=parse_string(":00000001FF",11,&m);
    test("Parser returned one for one record",r==1);
    test("Parsed hex string returned zero segments",memory_count(&m)==0);
    test("Parsed segment has correct size",memory_size(&m)==0);
    memory_free(&m);
}

void test_parse_extended_segment_address(){
    memory_t m;
    memory_init(&m);
    int r=parse_string(":020000021200EA\n:0B0010006164647265737320676170A7",49,&m);
    test("Parser returned two for two records",r==2);
    test("Parsed hex string returned one segment",memory_count(&m)==1);
    test("Parsed segment has correct size",memory_size(&m)==11);
    test("Parsed segment has correct address",m.segment->address==73744);
    memory_free(&m);
}

void test_parse_extended_linear_address(){
    memory_t m;
    memory_init(&m);
    int r=parse_string(":020000040800F2\n:0B0010006164647265737320676170A7",49,&m);
    test("Parser returned two for two records",r==2);
    test("Parsed hex string returned one segment",memory_count(&m)==1);
    test("Parsed segment has correct size",memory_size(&m)==11);
    test("Parsed segment has correct address",m.segment->address==134217744);
    memory_free(&m);
}

int main(int argc,char *argv){
    test_memory_consilidation();
    test_parse_data();
    test_parse_eof();
    test_parse_extended_segment_address();
    test_parse_extended_linear_address();
    return 0;
}

