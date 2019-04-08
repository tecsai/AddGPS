#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

char* JpegData = NULL;
char* newJpegData = NULL;
int sentry = 0;
int GpsTagPos = 0;
int Index = 0;
int TagCnt = 0;
int TempTag[12] = {0};

int fileLen;
int newFileLen = 0;
int newFileIndex = 0;

char TagPadding[4] = {0x00, 0x00, 0x00, 0x00};
char GPSSubTagCnt[2] = {0x06, 0x00}; 

char GPSLatitudeRefTag[12] = {0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4E, 0x00, 0x00, 0x00};
char GPSLatitudeTag[12] = {0x02, 0x00, 0x05, 0x00, 0x03, 0x00, 0x00, 0x00, /*offset*/0x00, 0x00, 0x00, 0x00};
char GPSLongtitudeRefTag[12] = {0x03, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00};
char GPSLongtitudeTag[12] = {0x04, 0x00, 0x05, 0x00, 0x03, 0x00, 0x00, 0x00, /*offset*/0x00, 0x00, 0x00, 0x00};
char GPSAltitudeRef[12] = {0x05, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char GPSAltitudeTag[12] = {0x06, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, /*offset*/0x00, 0x00, 0x00, 0x00};

char GPSLatitude[24] = {/*度*/0x2F, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, /*分*/0x27, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, /*秒*/0xA8, 0x27, 0x09, 0x00, 0x10, 0x27, 0x00, 0x00};
char GPSLongtitude[24] = {/*度*/0x78, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, /*分*/0x1F, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, /*秒*/0x8D, 0x94, 0x23, 0x0E, 0x10, 0x27, 0x00, 0x00};
char GPSAltitude[8] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

unsigned int GPSLatitudeOffset = 0;
unsigned int GPSLongtitudeOffset = 0;
unsigned int GPSAltitudeOffset = 0;

/*FF E2*/
int APP2_Index = 0;

//缩略图
int thumbnailIndex = 0;
int thumbnailLength = 0;
int thumbnail[2*1024*1024];

int main()
{
    int fd = open("/share/AddGPS_t/DSC00767.JPG", O_RDWR);       //O_RDWR表示可以进行读写操作，O_APPEND表示光标从文档的文段开始
    char str[100] = {0};
    if (fd == -1)
    {
        perror("open:");                  //perror函数：在打印括号里的内容后输出文件的错误的原因
    }

    //获取文件大小
    lseek(fd, 0, SEEK_SET);                                       //这个函数是为了将光标移到文档开头
    fileLen= lseek(fd, 0L, SEEK_END);  
    lseek(fd,0L,SEEK_SET);  
    newFileLen = fileLen+134;
    JpegData = (char*)malloc(fileLen);
    newJpegData = (char*)malloc(newFileLen);

    //读取文件数据
    int count2 = read(fd, JpegData, fileLen);                                    //读取文档内容写入内存
    if (count2 == -1)
    {
        perror("read fail:");
        close(fd);
    }

    //Seek 0xFF 0xDB
    for(sentry=0; sentry<fileLen; sentry++){
        if(JpegData[sentry]==0xFF && JpegData[sentry+1]==0xDB){
            break;
        }
    }
    sentry-=2; //Skip FF D8 回退两个字节，将缩略图其实字节FF D8跳过

    ////////////////////////////////////////////////////////////////////////////
    /*缩略图
    for(thumbnailIndex = sentry; thumbnailIndex<fileLen; thumbnailIndex++){
        if(JpegData[thumbnailIndex]==0xFF && JpegData[thumbnailIndex+1]==0xD9){
            thumbnailLength = thumbnailIndex+1-sentry;
            break;
        }
    }
    memcpy(thumbnail, JpegData+sentry, thumbnailLength);
    //写入文件
    FILE* tnfpo = fopen("/share/AddGPS_t/DSC00767_GPS_TN.JPG", "wb");
    if(tnfpo){
        int wr = fwrite(thumbnail, 1, thumbnailLength, tnfpo);
        //printf("Save data, size: %d\n", r);
        fflush(tnfpo);
        if(wr < thumbnailLength){
            printf("Error occured when write to file\n");
        }
        fclose(tnfpo);
    }
    *////////////////////////////////////////////////////////////////////////////////
    //Seek 0xFF 0xE1
    for(Index=0; Index<fileLen; Index++){
        if(JpegData[Index]==0xFF && JpegData[Index+1]==0xE1){
            break;
        }
    }
    Index+=18;//Skip Exif mark
    

    TagCnt |= (JpegData[Index]);
    TagCnt |= (JpegData[Index+1]<<8);
    Index+=2;
    printf("Tag count = %d\n", TagCnt);

    for(int tag=0; tag<TagCnt; tag++){
        if(JpegData[Index+(tag*12)+0]==0x25 && JpegData[Index+(tag*12)+1]==0x88){
            printf("GPS tag pos: 0x%08X\n", Index+(tag*12));
            JpegData[Index+(tag*12)+8] = 0xFF & (sentry-12+4);//-0x0C + 4字节padding
            JpegData[Index+(tag*12)+9] = 0xFF & ((sentry-12+4)>>8);
            JpegData[Index+(tag*12)+10] = 0xFF & ((sentry-12+4)>>16);
            JpegData[Index+(tag*12)+11] = 0xFF & ((sentry-12+4)>>24);
        }
    }
    //Compute Offset
    GPSLatitudeOffset = sentry-12+2+4+72;//-0x0C +2字节tag数 +4字节padding +72字节tag
    GPSLongtitudeOffset = GPSLatitudeOffset+24; 
    GPSAltitudeOffset = GPSLongtitudeOffset+24;
    //Fill offset data
    GPSLatitudeTag[8] = 0xFF &   GPSLatitudeOffset;
    GPSLatitudeTag[9] = 0xFF &  (GPSLatitudeOffset>>8);
    GPSLatitudeTag[10] = 0xFF & (GPSLatitudeOffset>>16);
    GPSLatitudeTag[11] = 0xFF & (GPSLatitudeOffset>>24);

    GPSLongtitudeTag[8] = 0xFF &   GPSLongtitudeOffset;
    GPSLongtitudeTag[9] = 0xFF &  (GPSLongtitudeOffset>>8);
    GPSLongtitudeTag[10] = 0xFF & (GPSLongtitudeOffset>>16);
    GPSLongtitudeTag[11] = 0xFF & (GPSLongtitudeOffset>>24);

    GPSAltitudeTag[8] = 0xFF &   GPSAltitudeOffset;
    GPSAltitudeTag[9] = 0xFF &  (GPSAltitudeOffset>>8);
    GPSAltitudeTag[10] = 0xFF & (GPSAltitudeOffset>>16);
    GPSAltitudeTag[11] = 0xFF & (GPSAltitudeOffset>>24);

    //Fill new image
    memcpy(newJpegData, JpegData, sentry);
    newFileIndex += sentry;
    
    memcpy(newJpegData+newFileIndex, TagPadding, 4);
    newFileIndex += 4;

    memcpy(newJpegData+newFileIndex, GPSSubTagCnt, 2);
    newFileIndex += 2;

    memcpy(newJpegData+newFileIndex, GPSLatitudeRefTag, 12);
    newFileIndex += 12;
    memcpy(newJpegData+newFileIndex, GPSLatitudeTag, 12);
    newFileIndex += 12;
    memcpy(newJpegData+newFileIndex, GPSLongtitudeRefTag, 12);
    newFileIndex += 12;
    memcpy(newJpegData+newFileIndex, GPSLongtitudeTag, 12);
    newFileIndex += 12;
    memcpy(newJpegData+newFileIndex, GPSAltitudeRef, 12);
    newFileIndex += 12;
    memcpy(newJpegData+newFileIndex, GPSAltitudeTag, 12);
    newFileIndex += 12;

    memcpy(newJpegData+newFileIndex, GPSLatitude, 24);
    newFileIndex += 24;
    memcpy(newJpegData+newFileIndex, GPSLongtitude, 24);
    newFileIndex += 24;
    memcpy(newJpegData+newFileIndex, GPSAltitude, 8);
    newFileIndex += 8;

    memcpy(newJpegData+newFileIndex, (JpegData+sentry), (fileLen-sentry));
    newFileIndex += (fileLen-sentry);


    //修改APP1中，字节数，用于计算APP2的offset
    //Change FF E2 offset
    for(APP2_Index=0; APP2_Index<newFileIndex; APP2_Index++){
        if(newJpegData[APP2_Index]==0xFF && newJpegData[APP2_Index+1]==0xE2){
            break;
        }
    }
    APP2_Index -= 4;
    printf("APP2 Index = %d\n", APP2_Index);
    //Seek 0xFF 0xE1
    for(Index=0; Index<newFileIndex; Index++){
        if(newJpegData[Index]==0xFF && newJpegData[Index+1]==0xE1){
            break;
        }
    }
    Index+=2;
    newJpegData[Index] = 0xFF & APP2_Index>>8;
    newJpegData[Index+1] = 0xFF & APP2_Index;


    //写入文件
    FILE* fpo = fopen("/share/AddGPS_t/DSC00767_GPS.JPG", "wb");
    if(fpo){
        int r = fwrite(newJpegData, 1, newFileIndex, fpo);
        //printf("Save data, size: %d\n", r);
        fflush(fpo);
        if(r < newFileIndex){
            printf("Error occured when write to file\n");
        }
        fclose(fpo);
    }

    return 0;
} 