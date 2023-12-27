#include "decomp.h"
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>

#define BIT_BUF_SIZE 0x100

typedef unsigned long long ullong;
typedef unsigned int uint;
typedef unsigned char uchar;

typedef struct Huffman{
    uint count[16]; //같은 길이를 가진 코드 개수
    uint table[16][290]; //코드에 해당하는 심볼이 저장된 배열
    uint start[16]; //길이 n의 첫번째 코드
    uint max_bit; //코드의 최대 길이
}Huffman;
// 예시
// Huffman huf;
// 3bit 코드 개수: huf.count[3]
// 4bit 코드 중 가장 낮은 값: huf.start[4];
// 4bit 코드 중 가장 낮은 값이 1010(10)일떄 코드 1101(13)에 해당하는 값: 
//		huf.table[4][13 - huf.start[4]] = huf.table[4][13-10] = huf.table[4][3]


//미리 정의된 호프만 코드
Huffman static_literal;
Huffman static_dist;



const int length_bit[29] = {0,0,0,0,0,0,0,0, 1,1,1,1,   2,2,2,2,   3,3,3,3,   4,4,4,4, 5,5,5,5, 0};
const int dist_bit[30] = {0,0,0,0, 1,1, 2,2, 3,3, 4,4,  5,5, 6,6,   7,7, 8,8,   9,9, 10,10, 11,11, 12,12, 13, 13};

int get_maxbit(char *list, int len);

int inflate(uchar *buffer, uint idx, uint filesize, Huffman* alp_literal, Huffman* alp_dist);

//BTYPE 01일때 미리 정의된 static huffman code 생성하기
void set_static_table()
{
    static int static_set = 0;

    if(static_set)
    {
        return;
    }

    memset(static_dist.count, 0, sizeof(static_dist.count));
    static_dist.max_bit = 5;
    static_dist.count[5] = 32;
    static_dist.start[5] = 0;
    for(int i=0; i<32;++i)
        static_dist.table[5][i]=i;

    
    memset(static_literal.count, 0, sizeof(static_literal.count));
    static_literal.max_bit = 9;
    static_literal.count[9] = 112;
    static_literal.count[8] = 152;
    static_literal.count[7] = 24;

    static_literal.start[7] = 0;
    static_literal.start[8]= 48;
    static_literal.start[9] = 400;

    for(int i=0;i<144;++i)
        static_literal.table[8][i]=i;
    
    for(int i=0; i<8;++i)
        static_literal.table[8][i+144]=280+i;

    for(int i=0;i<24;++i)
        static_literal.table[7][i] = 256+i;

    for(int i=0;i<112;++i)
        static_literal.table[9][i] = 144+i;
    
    static_set = 1;
}

//비트 단위로 파일 읽기
ullong bit_reader(int bitsize, FILE* f)
{
    // 아래 4개의 변수는 static으로 선언해서 함수 리턴 이후에도 값을 기억하도록 한다.
    static FILE *zip_file; //파일 포인터
    static ullong bit_buffer[BIT_BUF_SIZE]; //8바이트 unsigned long long 배열 
    static uint cur_idx = 0; //지금까지 읽은 long long 데이터
    static uint cur_bit = 0; //지금까지 읽은 비트

    uint nextbit;
    uint nex_idx;
    ullong ret;
    int read_done;

    //static 변수 초기화
    if(bitsize == 0 && f != NULL)
    {
        zip_file = f;
        int result = fread(bit_buffer, sizeof(ullong), BIT_BUF_SIZE, f);

        if(result == 0)
        {
            fputs("error while reading commpressed data\n",stderr);
            exit(1);
        }
        cur_bit = 0;
        cur_idx = 0;
    }

    //현재 바이트의 남은 비트 전부 읽기
    if(bitsize == -1)
    {
        bitsize = (8 - cur_bit)%8; //현재 바이트에서 읽은 비트가 없다면 그대로 리턴한다.
    }

    //0 비트 읽기 요청은 0리턴
    if(bitsize == 0)
        return 0;

    nextbit = (cur_bit + bitsize) % 64; //64 비트가 넘어가면 다시 0부터 시작

    //_bextr_u64 는 bextr 인스트럭션으로 컴파일된다. 레지스터 혹은 메모리에서 원하는 비트만 읽을 수 있는 인스트럭션
    if(nextbit > cur_bit)
        ret =  _bextr_u64(bit_buffer[cur_idx], cur_bit, bitsize); 
    
    // 64비트가 넘어가면 bit_buffer 인덱스 1 증가
    else
    {   
        //현재 인덱스에서 남은 비트를 읽고 
        ret = _bextr_u64(bit_buffer[cur_idx], cur_bit, 64-cur_bit); 
        cur_idx = (cur_idx+1) % BIT_BUF_SIZE;
        //버퍼의 데이터를 전부 읽은경우 버퍼 크기만큼 파일에서 다시 읽는다.
        if(cur_idx == 0)
        {
            read_done = fread(bit_buffer, sizeof(ullong), BIT_BUF_SIZE, zip_file);
            if(read_done == 0)
            {
                fputs("error while reading commpressed data\n",stderr);
                exit(1);
            }
        }
        //다음 인덱스에서 나머지 비트 읽기
        ret |= (_bextr_u64(bit_buffer[cur_idx], 0, nextbit) << (64-cur_bit));
    }
    cur_bit = nextbit;
    
    return ret;
}

//호프만 코드 읽기
uint table_lookup(Huffman* alphabet){
    int cur = 0;
    uint tmp= 0;
    int found = 0;
    uint ret = -1;
    // 코드 찾기 
    while (cur < alphabet->max_bit)
    {
        cur += 1;
        //호프만 코드는 최상위 비트부터 읽는다.
        tmp = (tmp << 1) | bit_reader(1, NULL);
        //길이 cur의 코드 개수가 0보다 크고 tmp와 같은 코드가 존재하는지 확인
        if(alphabet->count[cur] != 0 && tmp >= alphabet->start[cur]
             && tmp < alphabet->start[cur] + alphabet->count[cur] )
        {
            ret =alphabet-> table[cur][tmp - alphabet->start[cur]];
            found = 1;
            break;
        }
    }
    if(!found)
    {
        fputs("error while reading commpressed data\n",stderr);
        exit(1);
    }
    return ret;
}

//호프만 트리 생성하기 진짜 트리구조는 아니다.
void build_tree(uchar* clen, int max, Huffman *alphabet)
{
    //최대 비트 길이 구하기
    int max_bit = get_maxbit(clen, max);
    //printf("maxbit %d\n",max_bit);
    memset(alphabet, 0, sizeof(Huffman));

    //각 비트 길이의 코드 개수 구하기
    //코드에 해당하는 심볼 저장
    for(int i=0;i<max;++i)
    {
        int bit_len = clen[i];
        alphabet->table[bit_len][alphabet->count[bit_len]] = i;
        alphabet->count[bit_len] += 1;
    }
    alphabet->max_bit = max_bit;
    alphabet->count[0] = 0;

    int code = 0;
    //각 비트 길이의 첫번째 코드 저장
    for(int bits=1; bits <= max_bit; ++bits)
    {
        code =  (code + alphabet->count[bits-1]) << 1;
        alphabet->start[bits] = code;
    }
}

// BTYPE 10일때 블록에서 코드 읽기
void code_build(Huffman *alpha_literal, Huffman *alpha_dist)
{
    uint order[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    uchar clen[19];
    uint HLIT =  bit_reader(5, NULL);
    uint HDIST = bit_reader(5, NULL);
    uint HCLEN =  bit_reader(4, NULL);
    uint idx;
    ullong temp;

    //코드 길이를 구하기 위한 코드의 길이 읽기
    temp = bit_reader((HCLEN+4)*3, NULL);
    for(int i=0;i<19;++i)
    {
        if(i < HCLEN+4 )
            clen[order[i]] = (uchar)_bextr_u64(temp, i*3, 3);
        else
            clen[order[i]] = 0;
    }
    //코드 생성
    build_tree(clen, 19, alpha_literal);


    uchar literal_len[300];
    uchar dist[40];
    idx = 0;

    // 리터럴 바이트/길이 호프만 코드 읽기
    while(idx < HLIT+257)
    {
        uchar d = table_lookup(alpha_literal);
        int extra;
        if(d<0 | d>18)
        {
            fputs("error while reading compressed data\n",stderr);
            exit(1);
        }

        if(d < 16)
            literal_len[idx++] = d;
        
        else if(d == 16)
        {
            if(idx <= 0)
            {
                fputs("error while reading compressed data\n",stderr);
                exit(1);
            }
            extra =bit_reader(2, NULL)+3;
            memset(&literal_len[idx], literal_len[idx-1], extra);
            idx += extra;
        }
        else if(d == 17)
        {
            extra =bit_reader(3, NULL)+3;
            memset(&literal_len[idx], 0, extra);
            idx += extra;
        }
        else if(d == 18)
        {
            extra =bit_reader(7, NULL)+11;
            memset(&literal_len[idx], 0, extra);
            idx += extra;
        }
    }
    idx = 0;
    // 거리 호프만 코드 읽기
        while(idx < HDIST+1)
        {
            uchar d = table_lookup(alpha_literal);
            int extra;
            if(d<0 | d>18)
            {
                fputs("error while reading commpressed data\n",stderr);
                exit(1);
            }
            if(d < 16)
            {
                dist[idx++] = d;
            }
            else if(d == 16)
            {
                if(idx <= 0)
                {
                    fputs("error while reading commpressed data\n",stderr);
                    exit(1);
                }
                extra =bit_reader(2, NULL)+3;
                memset(&dist[idx], dist[idx-1], extra);
                idx += extra;
            }
            else if(d == 17)
            {
                extra =bit_reader(3, NULL)+3;
                memset(&dist[idx], 0, extra);
                idx += extra;
            }
            else if(d == 18)
            {
                extra =bit_reader(7, NULL)+11;
                memset(&dist[idx], 0, extra);
                idx += extra;
            }
        
        }
    //호프만 코드 생성
    build_tree(literal_len, HLIT+257, alpha_literal);
    build_tree(dist, HDIST+1, alpha_dist);
        
}

void ZIP_Decompress(FILE* f, char* filename, uint filesize)
{
    uint head;
    char *inflate_buffer;
    int idx;
    //호프만 코드를 저장할 변수
    Huffman alpha_literal;
    Huffman alpha_dist;

    //set bit_reader
    bit_reader(0, f);
	//복원할 파일 크기만큼 데이터 확보
    inflate_buffer = malloc(filesize);
    idx = 0;
    while(idx < filesize)
    {
        head = bit_reader(3, NULL);
        //printf("head %d\n",head);
        if((head&6) == 6)
        {
            fputs("head bit 6\n",stderr);
            exit(1);
        }
        //압축하지 않은 블록
        if((head&6) == 0)
        {
            //다음 바이트까지 스킵
            bit_reader(-1, NULL);
            short len = bit_reader(16, NULL);
            short nlen = bit_reader(16, NULL);
            //printf("len %x nlen %x\n",len, nlen);
            if(len != ~nlen)
            {
                fputs("error while reading uncompressed data\n", stderr);
                exit(1);
            }
                
            for(int i=0;i<len;++i)
                inflate_buffer[idx++] = bit_reader(8, NULL);

        }
        //dynamic huffman
        else if(head & 4)
        {
            //호프만 코드 생성하기
            code_build(&alpha_literal, &alpha_dist);
            idx += inflate(inflate_buffer, idx, filesize, &alpha_literal, &alpha_dist);
        }
        //static huffman
        else
        {
            //puts("static");
            //미리 정의된 코드 사용
            set_static_table();
            idx += inflate(inflate_buffer, idx, filesize, &static_literal, &static_dist);
        }
        //마지막 블록
        if(head%2 == 1)
            break;
    }

	//복원한 데이터 크기가 원본 파일 크기와 같은지 확인
    if(idx == filesize)
    {
        FILE* decomped_file = fopen(filename, "wb");
        if(decomped_file != NULL)
        {
            printf("save %s\n", filename);
            printf("size %d\n",idx);
            fwrite(inflate_buffer, 1, idx,decomped_file);
            free(inflate_buffer);
            fclose(decomped_file);
        }
        else
        {
            fputs("error while saving data\n", stderr);
        }
    }
    else
    {
        printf("%d %d",idx, filesize);
        fputs("error while reading commpressed data\n",stderr);
        exit(1);
    } 
}

// 최대값 구하기
int get_maxbit(char* list, int len)
{
    int max_bit = 0;
    for(int i=0; i<len; ++i)
        max_bit = (max_bit > list[i]) ? max_bit : list[i];
    return max_bit;
}

//압축된 블록 읽기
int inflate(uchar* buffer, uint idx, uint filesize, Huffman* alpha_literal, Huffman* alpha_dist)
{
    //puts("start inflate");
    uint literal, length, dist_code, dist;
    uint pos = idx;
    literal = table_lookup(alpha_literal);

    //256은 블록의 끝을 의미 
    //파일 크기 이상 읽지 않기
    while(literal != 256 && pos < filesize)
    {
        // literal byte
        if(literal < 256)
            buffer[pos++] = literal;

        // 이전에 중복된 데이터 복사 
        else if(literal <= 285)
        {
            //복사할 길이 구하기
            length = 3;
            for(int i=0;i<literal-257;++i)
            {
                length += (1<<length_bit[i]);
            }
            length += bit_reader(length_bit[literal-257], NULL);
            if(literal == 285)
                length = 258;

            //복사할 위치 구하기
            dist_code = table_lookup(alpha_dist);
            dist = 1;

            for(int i=0;i<dist_code;++i)
            {
                dist += (1<<dist_bit[i]);
            }
            int extra = bit_reader(dist_bit[dist_code], NULL);
            dist += extra;
            
            //복사할 위치가 0보다 작거나 파일 크기를 넘어서는 경우 
            if(pos < dist || pos+length > filesize)
            {
                break;
            }
            //이전 데이터 복사
            int src = pos - dist;
            for(int i=0; i<length;++i)
            {
                buffer[pos] = buffer[src];
                pos +=1;
                src +=1;

            }
        }
        else
            break;
        
        literal = table_lookup(alpha_literal);
    }
    //원본 파일 크기를 넘었거나 0보다 작은 위치에서 복사하려고 한 경우, 혹은 호프만 코드가 285보다 큰 경우
    if(literal != 256)
    {
        fputs("inflate error\n",stderr);
        exit(1);
    }
    return pos-idx;
}