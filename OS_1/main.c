#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROOT_START 0x2600	//根目录区从第19个扇区开始512*19=0x2600
#define DATA_START 0x4200	//数据区起始地址 

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define MAX_LEN 100

#define BYTSPERSEC 512	//每扇区字节数
#define SECPERCLUS 1	//每簇扇区数
#define RSVDSECCNT 1	//Boot记录占用的扇区数

#define DIR_SIZE 32 //条目大小

typedef struct{
	char	DIR_Name[11];													//文件名8字节扩展名3字节
	u8		DIR_Attr;														//文件属性
	char	DIR_Reserved[10];												//保留位
	u16		DIR_WrtTime;													//最后一次写入时间
	u16		DIR_WrtDate;													//最后一次写入日期
	u16		DIR_FstClus;													//此条目对应的开始簇号
	u32		DIR_FileSize;													//文件大小
}  RootEntry;

void read_fat12(FILE* file);
void readFile(FILE* file, char* path, int offset, RootEntry* re_ptr);
void readContent(FILE* file, RootEntry* re_ptr);
int getfatValue(FILE* file, int currentClus);

void find(FILE* file);
BOOL findFile(FILE* file, char* file_to_find, char* path, int offset, RootEntry* re_ptr);
BOOL findDir(FILE* file, char* file_to_find, char* path, int offset, RootEntry* re_ptr);

void toCapital(char* path);
BOOL isDirTotalSame(char* dir_to_find, char* path);
BOOL isDirPath(char* path);
BOOL isNowPathSame(char* path_to_find, char* path, int len);

BOOL isEmpty(RootEntry* re_ptr);
BOOL isDir(RootEntry* re_ptr);
BOOL isNameValid(RootEntry* re_ptr);

void printFile(char* path, RootEntry* re_ptr);
void printDir(char* path);
void addSubDir(char* path, char* next);
void addSubFile(char* paht, char* file);

extern void my_print(char* c, int length);
extern void change_color();
extern void def_color();

//函数入口
int main(){	
	FILE *file = fopen("a.img", "rb");
	read_fat12(file);
	find(file);	
	fclose(file);
	return 0;
}

//读文件主方法
void read_fat12(FILE* file){
	RootEntry* re_ptr = (RootEntry*)malloc(sizeof(RootEntry));
	memset(re_ptr, 0, sizeof(RootEntry));
	readFile(file, "", ROOT_START, re_ptr);
	free(re_ptr);
}

//读文件
void readFile(FILE* file, char* path, int offset, RootEntry* re_ptr){
	//文件位置指针当前位置相对于文件首的偏移字节数,返回当前文件位置
	long point = ftell(file); 		
	//重定位到offset偏移量的位置	
	fseek(file, offset, SEEK_SET);													
	memset(re_ptr, 0, DIR_SIZE);
	//读取当前偏移量的第一个目录条目
	fread(re_ptr, DIR_SIZE, 1, file);
	BOOL isOnlyDir = TRUE;
	while(!isEmpty(re_ptr)){
		if (isDir(re_ptr)){
			//如果是目录，取出簇号（数据区里存放子目录下的目录项
			u32 fst_Clus = re_ptr->DIR_FstClus;			
			//分配新路径 = 原目录路径 + 8个字节	
			char* newPath = (char*)malloc(strlen(path) + 0x08);		
			memset(newPath, 0, strlen(newPath));									
			memcpy(newPath, path, strlen(path));		
			addSubDir(newPath, re_ptr->DIR_Name);
			//把当前目录项保存起来
			RootEntry oldre_ptr;
			memcpy(&oldre_ptr, re_ptr, DIR_SIZE);									
			//读取子目录
			readFile(file, newPath, ((fst_Clus-2) * 0x200 + DATA_START), re_ptr);
			//将内存块清零，避免后续写入出现错误
			memset(newPath,0,strlen(newPath));										
			free(newPath);
			memcpy(re_ptr, &oldre_ptr, DIR_SIZE);
		}else if(isNameValid(re_ptr)){	
			//若不为目录，如果文件名合法则打印路径		
			printFile(path, re_ptr);
			isOnlyDir = FALSE;
		}
		//re_ptr置0，并从当前位置读取一个目录项,指针后移一项
		memset(re_ptr, 0, DIR_SIZE);												
		fread(re_ptr, DIR_SIZE, 1, file);
	}
	//如果是空目录，则打印到目录路径
	if(isOnlyDir)																	
		printDir(path);
	//设置指针到刚进入方法时指针指向的位置
	fseek(file, point, SEEK_SET);													
}

//读取文件的内容
void readContent(FILE* file, RootEntry* re_ptr){
	int size = SECPERCLUS*BYTSPERSEC;
	u16	currentClus = re_ptr->DIR_FstClus;
	int next_Clus = 0;
	if(currentClus==0xff7){
		printf("bad clus!");
		return;
	}
	while(next_Clus<0xff8){
		next_Clus = getfatValue(file,currentClus);
		int offset = (currentClus-2)*0x200+DATA_START;
		fseek(file,offset,SEEK_SET);
		//暂存从簇中读出的数据 
		char* str = (char* )malloc(size);  											
		fread(str,size,1,file); 
		int i;
		for(i=0;i<size;++i){
			if(str[i]!=0x00){
				my_print(&str[i],1);
			}else{
				break;
			}
		}
		currentClus = next_Clus;
		if (next_Clus == 0xFF7) {
			printf("坏簇，读取失败!\n");
			break;
		}
	}
}

//读取FAT表
int getfatValue(FILE* file, int num){
	//FAT1的偏移字节
	int fatBase = RSVDSECCNT * BYTSPERSEC;
	//FAT项的偏移字节
	int fatPos = fatBase + num*3/2;
	//奇偶FAT项处理方式不同，分类进行处理，从0号FAT项开始
	int type = 0;
	if (num % 2 == 0) {
		type = 0;
	} else {
		type = 1;
	}

	//先读出FAT项所在的两个字节
	u16 bytes;
	u16* bytes_ptr = &bytes;
	int check;
	check = fseek(file,fatPos,SEEK_SET);
	if (check == -1) 
		printf("fseek in getFATValue failed!");

	check = fread(bytes_ptr,1,2,file);
	if (check != 2)
		printf("fread in getFATValue failed!");

	//u16为short，结合存储的小尾顺序和FAT项结构可以得到
	//type为0的话，取byte2的低4位和byte1构成的值，type为1的话，取byte2和byte1的高4位构成的值
	if (type == 0) {
		return bytes&0x0FFF;
	} else {
		return bytes>>4;
	}
}

//根据输入路径查找文件
void find(FILE* file){
	while(TRUE){
		//存放输入路径
		char* filePath = (char*)malloc(MAX_LEN*sizeof(char));
		my_print("Please input the file path you want to find(no more than 99 bytes): \n",70);
		scanf("%s",filePath);
		toCapital(filePath);
	
		//先申请空间存放目录条目
		RootEntry* re_ptr = (RootEntry*)malloc(sizeof(RootEntry));
		memset(re_ptr,0,sizeof(RootEntry));
		//判断是否是目录路径
		BOOL isDir = isDirPath(filePath);
		BOOL isFind;
		if(!isDir){
			isFind = findFile(file,filePath,"",ROOT_START,re_ptr);
		}else{
			isFind = findDir(file,filePath,"",ROOT_START,re_ptr);
		}
		if(!isFind){
			my_print("Unknown file!\n",14);
		}
		free(re_ptr);
	}
}

//查找文件并输出文件内容
BOOL findFile(FILE* file, char* file_to_find, char* path, int offset, RootEntry* re_ptr){
	long point = ftell(file); 						
	fseek(file, offset, SEEK_SET);							
	memset(re_ptr, 0, DIR_SIZE);											
	fread(re_ptr, DIR_SIZE, 1, file);
	
	BOOL isfind = FALSE;	
	while(!isEmpty(re_ptr)){
		if (isDir(re_ptr)){
			u32 fst_Clus = re_ptr->DIR_FstClus;								
			char* newPath = (char*)malloc(strlen(path) + 0x08);			
			memset(newPath, 0, strlen(newPath));
			memcpy(newPath, path, strlen(path));		
			addSubDir(newPath, re_ptr->DIR_Name);
			//判断路径到目前为止是否相同
			BOOL nowSame = isNowPathSame(file_to_find,newPath,strlen(newPath));
			if(nowSame){
				RootEntry oldre_ptr;
				memcpy(&oldre_ptr, re_ptr, DIR_SIZE);
				isfind = findFile(file,file_to_find, newPath, ((fst_Clus-2) * 0x200 + DATA_START), re_ptr);
				memcpy(re_ptr, &oldre_ptr, DIR_SIZE);
			}
			memset(newPath,0,strlen(newPath));	
			free(newPath);
			if(isfind)
				return TRUE;

		}else if(isNameValid(re_ptr)){		
			//文件名+扩展名11个字节
			char* newPath = (char*)malloc(strlen(path) + 0x0B);		
			memset(newPath, 0, strlen(newPath));
			memcpy(newPath, path, strlen(path));		
			addSubFile(newPath, re_ptr->DIR_Name);
			BOOL sameFile = isNowPathSame(file_to_find,newPath,strlen(newPath));
			memset(newPath,0,strlen(newPath));						
			free(newPath);	
			if(sameFile){
				readContent(file,re_ptr);
				return TRUE;
			}
		}
		memset(re_ptr, 0, DIR_SIZE);
		fread(re_ptr, DIR_SIZE, 1, file);
	}	
	fseek(file, point, SEEK_SET);
	return FALSE;
}

//查找目录并输出目录下全部子目录及文件
BOOL findDir(FILE* file, char* file_to_find, char* path, int offset, RootEntry* re_ptr){
	long point = ftell(file); 									
	fseek(file, offset, SEEK_SET);		
	memset(re_ptr, 0, DIR_SIZE);		
	fread(re_ptr, DIR_SIZE, 1, file);
		
	BOOL isfind = FALSE;
	while(!isEmpty(re_ptr)){
		if (isDir(re_ptr)){
			u32 fst_Clus = re_ptr->DIR_FstClus;	
			char* newPath = (char*)malloc(strlen(path) + 0x08);		
			memset(newPath, 0, strlen(newPath));
			memcpy(newPath, path, strlen(path));		
			addSubDir(newPath, re_ptr->DIR_Name);
			BOOL totalSame = isDirTotalSame(file_to_find,newPath);
			if(totalSame){
				readFile(file, newPath, ((fst_Clus-2) * 0x200 + DATA_START), re_ptr);
				return TRUE;
			}
			BOOL nowSame = isNowPathSame(file_to_find,newPath,strlen(newPath));
			if(nowSame){
				RootEntry oldre_ptr;
				memcpy(&oldre_ptr, re_ptr, DIR_SIZE);
				isfind = findDir(file,file_to_find, newPath, ((fst_Clus-2) * 0x200 + DATA_START), re_ptr);
				memcpy(re_ptr, &oldre_ptr, DIR_SIZE);
			}
			memset(newPath,0,strlen(newPath));
			free(newPath);
			if(isfind)
				return TRUE;

		}
		memset(re_ptr, 0, DIR_SIZE);		
		fread(re_ptr, DIR_SIZE, 1, file);
	}	
	fseek(file, point, SEEK_SET);
	return FALSE;
}

//判断目录路径是否完全匹配
BOOL isDirTotalSame(char* dir_to_find, char* path){
	int i=0;
	int j=0;
	BOOL isfind = TRUE;
	for(i=0;i<strlen(dir_to_find);){	
		if(dir_to_find[i]=='/'){
			i++;
		}
		while(path[j]==' '){
			j++;
		}
		if((dir_to_find[i]==path[j])){
			if((dir_to_find[i]!='\0')&&(path[j]!='\0')){
				j++;
				i++;
			}
		}else{
			isfind = FALSE;
			break;
		}
	}
	if(isfind){
		if(i!=(strlen(dir_to_find)))
			isfind = FALSE;
	}
	return isfind;
}

//判断输入的路径时目录路径还是文件路径
BOOL isDirPath(char* path){
	BOOL isDir = TRUE;
	int i=0;
	for(i=0;i<strlen(path);){
		if(path[i]=='.'){
			isDir = FALSE;
			break;
		}
		i++;
	}
	if(path[strlen(path)-1]!='/'){
		isDir = FALSE;
	}
	return isDir;
}

//比较走到目前为止路径是否相同
BOOL isNowPathSame(char* path_to_find, char* path, int len){
	int i=0;
	int j=0;
	BOOL isSame = TRUE;
	for(i=0;i<len;){
		if(path_to_find[j]=='/')
			j++;
		if(path_to_find[j]=='.'){
			j++;
		}
		while(path[i]==' '){
			i++;
		}
		//已行至末尾 
		if(i>=len)
			return isSame;
		if(path_to_find[j]==path[i]){
			j++;
			i++;
		}else{
			isSame = FALSE;
			break;
		}
	}
	return isSame;
}

//将输入路径转为大写
void toCapital(char* str){
	int i;
	for(i=0;i<strlen(str);i++){
		if(str[i]>=97&&str[i]<=122){
			str[i]=str[i]-32;
		}
	}
}

//只打印目录路径
void printDir(char* path){
	int i=0;
	change_color();
	char* p="/";
	while(i<strlen(path)){
		if (path[i] != ' '){
			my_print(&path[i], 1);
		}
		if (i % 8 == 7){
			my_print(p, 1);
		}
		i++;
	}
	def_color();
	my_print("\n",1);
}

//打印文件路径
void printFile(char* path, RootEntry* re_ptr){
	int i = 0;
	change_color();
	char* p = "/";
	//目录部分
	while (i < strlen(path)){
		if (path[i] != ' '){
			my_print(&path[i], 1);
		}
		if (i % 8 == 7){
			my_print(p, 1);
		}
		i++;
	}
	def_color();
	if(isEmpty(re_ptr))
		return;
	//文件名部分
	for (i = 0; i < 8; i++){
		if (re_ptr->DIR_Name[i] == ' '){
			break;
		}
		my_print(&re_ptr->DIR_Name[i], 1);
	}
	my_print(".", 1);
	//扩展名部分 
	for (i = 8; i < 11; i++){
		if (re_ptr->DIR_Name[i] == ' '){
			break;
		}
		my_print(&re_ptr->DIR_Name[i], 1);
	}
	my_print("\n",1);
}

//把当前目录项加入路径
void addSubDir(char* path, char* next){
	int p = 0;
	while (path[p] != 0)
		p += 8;
	int i = 0;
	for (i = 0; i < 8; i++){
		path[p + i] = next[i];
	}
}

//把当前文件名加入路径
void addSubFile(char* path, char* file){
	int p = 0;
	while (path[p] != 0)
		p += 8;
	int i = 0;
	for (i = 0; i <11; i++){
		path[p + i] = file[i];
	}
}

//判断条目中的32个字节是否全为空
BOOL isEmpty(RootEntry* re_ptr){
	int i = 0;
	for (i = 0; i < 0x20; i++){
		if(((char*)re_ptr)[i] > 0){
			return FALSE;
		}
	}
	return TRUE;
}

//判断条目是否为目录
BOOL isDir(RootEntry* re_ptr){
	int i;
	if(re_ptr->DIR_Name[0]==0){
		return FALSE;	
	}
	for(i=0;i<11;i++){
		if(!(((re_ptr->DIR_Name[i]>=48)&&(re_ptr->DIR_Name[i]<=57))||
		((re_ptr->DIR_Name[i]>=65)&&(re_ptr->DIR_Name[i]<=90))||
		((re_ptr->DIR_Name[i]>=97)&&(re_ptr->DIR_Name[i]<=122))||
		(re_ptr->DIR_Name[i]==32))){
			return FALSE;
		}
	}
/*
	for(i=8;i<11;i++){
		if(re_ptr->DIR_Name[i]!=32){
			return FALSE;
		}
	}
*/
	if((re_ptr->DIR_Attr&0x10)!=0)
		return TRUE;
	else
		return FALSE;
}

//判断文件名是否合法(当前只允许0-9，a-z, A-Z)
BOOL isNameValid(RootEntry* re_ptr){
	int j = 0;
		for (j = 0; j < 11; j++){
			char c = re_ptr->DIR_Name[j];
			if (! ((c >= '0' && c <= '9') 
				|| (c >= 'a' && c <= 'z')
				|| (c >= 'A' && c <= 'Z')
				|| c == ' ')){
				return FALSE;
			} 
		}
	return TRUE;
}
