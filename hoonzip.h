// 압축 알고리즘 목록 이번 과제에서는 압축하지 않은 STORE과 기본 압축 알고리즘인 DEFALTE만 사용한다.

typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long long ullong;

#define ECOD_SIGNATURE 0x06054b50
#define CDE_SIGNATURE 0x02014b50
#define LFH_SIGNATURE 0x04034b50

#define BIT_BUF_SIZE 0x100

char *compression[] ={"STORE", "", "", "", "", "", "","", "DEFLATE"};

typedef struct EOCD{
    uint signature;
    ushort disk_num;
    ushort disk_start;
    ushort num_of_CDR;
    ushort total_CDR;
    unsigned CD_size;
    unsigned CD_offset;
    ushort comment_len;
} __attribute__((packed)) EOCD;

//Central Directory record
typedef struct CDEntry
{
    uint signature; //0x02014b50
    ushort version; //파일이 생성된 버전
    ushort min_version; //압축 해제에 필요한 최소 버전
    ushort flag;
    ushort method; //압축 알고리즘
    ushort mod_time; //마지막 변경 시간
    ushort mod_date; //마지막 변경 날짜
    uint CRC32; //원본 파일의 crc-32
    uint compressed_size; //압축된 크기
    uint uncompressed_size; //원본 크기
    ushort filename_len; //파일 이름 길이
    ushort extra_len; //엑스트라 필드 길이
    ushort filecomment_len; //코멘트 길이
    ushort disknum; // 디스크 번호
    ushort Internal_att;
    uint external_att;
    uint local_header_offset; //local file header 오프셋
    char *filename; //파일 이름
    char *extra;
    char *comment;
} __attribute__((packed))  CDEntry;


typedef struct local_file_header{
    uint signature; //0x04034b50
    ushort min_version; //압축 해제에 필요한 최소 버전
    ushort flag;
    ushort method; //압축 알고리즘
    ushort mod_time; //마지막 변경 시간
    ushort mod_date; //마지막 변경 날짜
    uint CRC_32; //원본 파일의 CRC-32 
    uint comp_size; // 압축된 크기
    uint uncomp_size; // 원본크기
    ushort file_name_len; //파일 이름 길이
    ushort extra_len; //엑스트라 필드 길이
    char *file_name; // 파일 이름
    char *extra_field;
}__attribute__((packed)) local_file_header;

const char *compression_method[] = {"STORE","","","","","","","","DEFLATE"};

enum zip_option {decomp, list};
void print_usage();
