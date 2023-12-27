#include <stdio.h>
#include <stdlib.h>
#include "decomp.h"
#include "hoonzip.h"
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define MAXPATH 4096

//local file header 읽기
int read_local_file_header(local_file_header* lfh, FILE *f)
{
    int result;

    result = fread(lfh, sizeof(local_file_header)-2*sizeof(char*), 1, f);

    if(result != 1 || lfh->signature != LFH_SIGNATURE)
        return 1;

    lfh->file_name = malloc(lfh->file_name_len+1);
    result = fread(lfh->file_name, lfh->file_name_len, 1, f);
    if(result != 1)
        return 2;

    if(lfh->extra_len)
    {
        lfh->extra_field = malloc(lfh->extra_len+1);
        result = fread(lfh->extra_field, lfh->extra_len, 1, f);
        if(result != 1)
            return 2;
    }
    // puts("===================================================");
    // printf("file name: %s\n",lfh->file_name);
    // printf("compression method: %s\n",compression_method[lfh->method]);
    // printf("original size: %d\n",lfh->uncomp_size);
    // printf("compressed size: %d\n",lfh->comp_size);
    return 0;
 }

 void free_lh(local_file_header* lfh)
 {
    if(lfh->file_name_len > 0)
        free(lfh->file_name);
    if(lfh->extra_len > 0)
        free(lfh->extra_field);
 }


// Central directory entry 읽기
int read_CDEntry(CDEntry* cd_data, FILE *f)
{
    int result;

    //가변 길이 필드(파일 이름, 엑스트라 필드, 코멘트)를 제외한 레코드 읽기
    result= fread(cd_data, sizeof(CDEntry) - 3*sizeof(char*), 1, f);
    // 시그니쳐가 맞지 않는 경우
    if(cd_data->signature != CDE_SIGNATURE)
        return 1;
    // 파일을 읽지 못한 경우
    if(result != 1)
        return 2;

    // 파일 이름 읽기
    cd_data->filename = malloc(cd_data->filename_len+1);
    result = fread(cd_data->filename, cd_data->filename_len, 1, f);
    cd_data->filename[cd_data->filename_len] = '\0';
    if(result != 1)
        return 3;

    //엑스트라 필드가 있다면 읽기
    if(cd_data->extra_len > 0)
    {
        cd_data->extra = malloc( cd_data->extra_len+1);
        result = fread(cd_data->extra, cd_data->extra_len, 1, f);
        if(result != 1)
            return 3;
    }
    //파일 코멘트가 있다면 읽기
    if(cd_data->filecomment_len > 0)
    {
        cd_data->comment = malloc( cd_data->filecomment_len+1);
        result = fread(cd_data->comment, cd_data->filecomment_len, 1, f);
        if(result != 1)
            return 3;
    }
    // puts("======================================");
    // printf("file name: %s\n",cd_data->filename);
    // printf("orignal size: %d\n",cd_data->uncompressed_size);
    // printf("compressed size: %d\n",cd_data->compressed_size);
    // printf("external file attribute: %o\n", cd_data->external_att);
    // printf("interal file attribute: %o\n", cd_data->Internal_att);
    // printf("compressed method: %d\n", cd_data->method);
    // printf("local file header 0x%x\n",cd_data->local_header_offset);

    
    return 0;
}

void free_cd_entry(CDEntry* cd_data)
{
    if(cd_data->filename_len !=0)
        free(cd_data->filename);

    if(cd_data->extra_len >0)
        free(cd_data->extra);

    if(cd_data->filecomment_len > 0)
        free(cd_data->comment);
}
//EOCD 레코드 읽기
int read_EOCD(EOCD* eocd, FILE* f, uint file_size)
{
    fseek(f, 0, SEEK_END);
    uint signature;
    int read_done;
    int i;

    //end of central directory signature 찾기
    for(i=22;i<file_size;++i)
    {
        fseek(f, -i, SEEK_END);
        read_done = fread(&signature, 4, 1, f);
        
        if(signature == ECOD_SIGNATURE)
            break;
    }
    //시그니쳐를 찾지 못함
    if(i == file_size)
    {
        fputs("can't find end_of_central_directory_record\n", stderr);
        exit(1);
    }

    //extra field를 제외한 EOCD 레코드 읽기
    fseek(f, -i, SEEK_END);
    read_done = fread(eocd, sizeof(EOCD), 1, f);
    if(read_done == 0)
    {
        fputs("error while reading end_of_central_directory_record", stderr);
        exit(1);
    }
    // puts("EOCD record");
    // printf("signature 0x%x\n",eocd->signature);
    // printf("central directory starts offset 0x%x\n", eocd->CD_offset);
    // printf("total number of central directory records %d\n",eocd->total_CDR);
    return 1;
}

int main(int argc, char**argv)
{
    int result;
    unsigned file_size;

    EOCD eocd;
    CDEntry *cd_entries;
    local_file_header *local_file_headers;
    FILE* f;
    char opt;
    char *exdir = NULL;
    enum zip_option zop  = decomp;
    puts("let\'s go\n");

    //옵션 읽기 
    // -l 파일 리스팅
    // -d 압축 해제한 파일을 저장할 위치 지정
    while ((opt = getopt(argc, argv, "lhd:")) != -1) 
    {
        switch (opt) {
        case 'h':
            print_usage();
            exit(0);
        // 파일 리스트 확인
        case 'l':
            zop = list;
            break;
        // 파일을 저장할 폴더 지정
        case 'd':
            exdir = optarg;
            break;
        default: /* '?' */
            print_usage();
            exit(1);
       }
    }
    if(optind >= argc)
    {
        print_usage();
        exit(1);
    }

    // 압축 파일 열기
    f = fopen(argv[optind], "rb");

    if(f == NULL)
    {
        fputs("please check your filename\n",stderr);
        exit(1);
    }
    
    //파일 크기 확인
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);

    //end_of_central directory 레코드 읽기
    result = read_EOCD(&eocd, f, file_size);
    if(result == 0)
    {
        fputs("not valid zip file\n",stderr);
        exit(1);
    }
   
    //central directory 레코드와 local file header 메모리 할당
    cd_entries = malloc( sizeof(CDEntry) * eocd.num_of_CDR);
    local_file_headers = malloc( sizeof(local_file_header) * eocd.num_of_CDR);

    
    //central directory 읽기
    fseek(f, eocd.CD_offset, SEEK_SET);
    for(int i=0; i<eocd.total_CDR; ++i)
    {
        result = read_CDEntry(&cd_entries[i], f);
        if(result !=0)
        {
            fputs("error while reading central directory record please check it is vaild zip file\n",stderr);
            exit(1);
        }
    }

    //저장된 파일 리스트 출력
    if(zop==list)
    {
        puts("file list");
        for(int i=0; i<eocd.total_CDR; ++i)
        {
            puts("=====================================================");
            printf("name %s\n",cd_entries[i].filename);
            if(cd_entries[i].external_att & 0x20)
            {
                printf("compression method %s\n",compression_method[cd_entries[i].method]);
                printf("compressed size %d\n",cd_entries[i].compressed_size);
                printf("original size %d\n",cd_entries[i].uncompressed_size);
                fseek(f, cd_entries[i].local_header_offset, SEEK_SET);
                result = read_local_file_header(&local_file_headers[i], f);
                long dataoffset = ftell(f);
                printf("compressed data offset: %ld\n",dataoffset);
            }

        }
    }
    //압축 해제
    else
    {
        //지정한 경로가 있을 경우
        if(exdir)
        {
            struct stat st = {0};
            //폴더가 없다면 생성
            if (stat(exdir, &st) == -1 && errno == ENOENT) {
                result = mkdir(exdir, 0700);
                if(result !=0)
                {
                    fputs("please check directory name\n",stderr);
                    exit(1);
                }
            }
            else if(stat(exdir, &st) == -1 )
            {
                fputs("please check directory name\n",stderr);
                exit(1);
            }
        }
        //local file header 읽고 압축 해제하기
        for(int i=0; i<eocd.total_CDR; ++i)
        {
            fseek(f, cd_entries[i].local_header_offset, SEEK_SET);
            result = read_local_file_header(&local_file_headers[i], f);
            if(result != 0)
            {
                fputs("error %d while reading local file header\n",stderr);
                exit(1);
            }
            //지정한 경로가 있을 경우 파일 이름 앞에 붙이기
            char extract_path[MAXPATH];
            extract_path[0] = '\0';
            if(exdir)
            {
                strncpy(extract_path, exdir, MAXPATH-2);
                extract_path[MAXPATH-1] = '\0';
                strncat(extract_path, "/", MAXPATH -1 - strlen(extract_path) );
            }
            strncat(extract_path, cd_entries[i].filename, MAXPATH -1 - strlen(extract_path) );

            //해당 위치에 파일이 이미 존재하는지 확인
            struct stat st = {0};
            if (stat(extract_path, &st) == -1 && errno == ENOENT) {
                puts("======================================");
                printf("extract %s\n",extract_path);
                if(cd_entries[i].external_att & 0x20)
                {
                    if(cd_entries[i].method != 8)
                    {
                        fputs("not supported compression method\n",stderr);
                        exit(1);
                    }
                    ZIP_Decompress(f, extract_path, cd_entries[i].uncompressed_size);
                }
                else if(cd_entries[i].external_att & 0x10)
                {   
                    struct stat st = {0};
                    if (stat(extract_path, &st) == -1) {
                        mkdir(extract_path, 0700);
                    }
                }
            }
            else{
                fprintf(stderr, "file %s already exist\n",extract_path);
                exit(1);
            }

            free_lh(&local_file_headers[i]);
        }
    }
    for(int i=0; i<eocd.total_CDR; ++i)
        free_cd_entry(&cd_entries[i]);

    free(cd_entries);
    free(local_file_headers);

    return 0;
}

void print_usage(){
    puts("usage: hoonzip [-l] [-d exdir] FILE_NAME");
    puts("");
    puts("-l list files (no extract files)");
    puts("-d extract files into exdir");
}
