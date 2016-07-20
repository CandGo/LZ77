#ifdef __cplusplus   
#define EXPORT extern "C" __declspec (dllexport)   
#else   
#define EXPORT __declspec (dllexport)   
#endif   
#include<stdio.h>
#include<stdlib.h>
//#include<time.h>
#include<windows.h>
#include"string.h"
//#include<math.h>

#define  max_read 4608
#define  max_wind 4096
unsigned char window[max_wind];//wind_num
unsigned char readtmp[max_read];
unsigned int hash[69632]={0};
unsigned int read_num=0;
unsigned int wind_num=0;
unsigned int figure_num=0;
int init_wind(FILE *fp)
{
	memset(window,0,max_wind);
	return fread(window,1,max_wind,fp);
}
int file_size(char * file)
{
	int tmp_num=0;
	char a[102400]={0};
	FILE * s_fp=fopen(file,"rb");
	while(1)
	{
		int fread_num=fread(a,1,102400,s_fp)	;
		tmp_num=tmp_num+fread_num;
		if(fread_num<102400) break;
	}
	fclose(s_fp);
	return tmp_num;
}
unsigned int log_2(unsigned int num)
{
	int p=0;
	while(num=num/2) p++;
	return p;
}
int trans_one (unsigned char tmp)
{	
	figure_num++;
	read_num=figure_num/8;
		readtmp[read_num++]=readtmp[read_num]|tmp>>(figure_num%8);
		if(figure_num%8)
		readtmp[read_num]=readtmp[read_num]|tmp<<(8-(figure_num%8));
		figure_num=figure_num+8;
	return 0;
}
unsigned int fig_one ()
{	
	figure_num++;
	read_num=figure_num/8;
	unsigned char c=readtmp[read_num]<<(figure_num%8)|readtmp[read_num+1]>>(8-figure_num%8);
	figure_num=figure_num+8;
	window[wind_num]=c;
	wind_num++;
	return figure_num;
}
unsigned int fig_more()
{
	unsigned int tmp_len=1;
	unsigned int tmp_off=0;
	unsigned int len_code=0;
	read_num=figure_num/8;
	unsigned int c=0;
	unsigned int i=0;
	for (i=0;i<4&&read_num<sizeof(readtmp);i++)
		c=c|(readtmp[read_num++]<<(24-i*8));
	c=c<<(figure_num%8)|(readtmp[read_num++]>>(8-figure_num%8));
	if (c>>24==255)
	{
	tmp_len=256;
	len_code=len_code+8;
	c=(c<<8)|(readtmp[read_num++]<<figure_num%8)>>figure_num%8;
	}
	for (i=31;i>0;i--)
		{
			len_code=len_code+1;//加0
			if ((c>>i)%2!=1) break;
			tmp_len=tmp_len*2;
		}
		unsigned int n_n=len_code-(len_code/9)*8;
		tmp_len=tmp_len+((c<<n_n)>>(32-len_code+1));	
		unsigned char n=log_2((wind_num<<1)-1);
		tmp_off=(c<<(n_n+len_code-1))>>(32-n);
		len_code=len_code+len_code-1;
		len_code=len_code+n;
		figure_num=figure_num+len_code;
		for(i=0;i<tmp_len;i++)
			window[wind_num++]=window[tmp_off++];
	return figure_num;	
}

unsigned int trans_more (unsigned int len,unsigned int off)
{
	unsigned int num_p=log_2(len);
	unsigned int num_re=len<<(32-num_p);//余数编码 已经在最前端
	unsigned int len_code=0;//一次二元组编码长度
	unsigned int tmp_c =~0;
	unsigned int x=num_p;
		if(num_p>8)
		{
			figure_num--;
			trans_one(255);
			x=num_p-8;
		}
			tmp_c =tmp_c<<(32-x);//剩下的111
			tmp_c=tmp_c|num_re>>(x+1);	//中间的0 和余数编码
			len_code=log_2((wind_num<<1)-1);//减少函数调用
			tmp_c=tmp_c|off<<(32-num_p-x-1-len_code);//off编码
			len_code=len_code+num_p+x+1;
		while(1)
		{
			if (len_code>8)
			{
				figure_num--;
				trans_one(tmp_c>>(32-8));
				tmp_c=tmp_c<<8;
				len_code=len_code-8;
			}
			else
			{
				figure_num--;
				trans_one(tmp_c>>(32-8));
				figure_num=figure_num-8+len_code;
				break;
			}
		}
	return 0;
}
unsigned int macth(unsigned int * n_len,unsigned int * n_off)
{
	unsigned int i;
		for (i=0;i<wind_num;i++)
		{	
			unsigned int n_read1=wind_num;//测试字符
			unsigned int n_i=i;
			unsigned int cnt=0;
			while(window[n_read1]==window[n_i]&&n_i<wind_num&&n_read1<max_wind)
			{
				n_read1++;
				n_i++;
				cnt++;
			}
			if (cnt>(*n_len)&&cnt!=1)
			{
				*n_len=cnt;
				*n_off=i;
			}
		}
		if (*n_len>1) 
			return 1;
		else return 0;		
}

void c_hash_table(unsigned int * hash,int * next,unsigned int tmp_wind)
{
	for (unsigned int i=0;i<tmp_wind-1;i++)
	{
		unsigned int tmp_two=(window[i]<<8)|window[i+1];	//计算坐标	
			while(1)
			{	
				if(hash[tmp_two]>(max_wind<<17))
				{
					hash[tmp_two]=i<<17;
					break;
				}
				if((hash[tmp_two]<<15)==0)
				{
					hash[tmp_two]=hash[tmp_two]|(*next);
					hash[*next]=i<<17;
					(*next)++;
					break;
				}
				else
					tmp_two=(hash[tmp_two]<<15)>>15;
			}	
		}
}

unsigned int hash_table(unsigned int * hash,unsigned int * n_len,unsigned int * n_off,unsigned int tmp_wind)
{
	if(wind_num==tmp_wind-1) return 0;
	unsigned int tmp_two=(window[wind_num]*256);//计算坐标
	tmp_two=tmp_two+window[wind_num+1];
	unsigned int cnt=0;
	while(1)
	{
		cnt=0;	
		if (tmp_two>70000)
		{
			printf("error___!\n");
		}
		if(hash[tmp_two]>(max_wind<<17))
		{
			printf("error======!\n");
			//break;
		}
		//	printf(">>>\n");
		unsigned int tmp_w=wind_num;
		unsigned int tmp_h=hash[tmp_two]>>17;
		if (tmp_h+2>wind_num)
		{
			break;
		}
		while((window[tmp_h++])==window[tmp_w++]&&tmp_w<=tmp_wind&&tmp_h<=wind_num)
			cnt++;
		if (cnt>(*n_len)&&cnt!=1)
		{
			*n_len=cnt;
			*n_off=hash[tmp_two]>>17;
		}
		//tmp_two=;
		if((hash[tmp_two]<<15)==0)
			return 1;
		else
			tmp_two=(hash[tmp_two]<<15)>>15;
	}	
	return *n_len;
}
int compress(char * file)
{
		FILE * fp=fopen(file,"rb");
		FILE * f_write;
		unsigned int file_s=0;
		int file_num=0;
		if(fp==NULL) 
		{
			printf("no file\n");
			return 0;
		}
		else
		{
			file_num=file_size(file);
			char tmp_name[133]={0};
			strcpy(tmp_name,file);
			strcat(tmp_name,".lz77");
		f_write=fopen(tmp_name,"wb+");
		}
		unsigned int n_len,n_off;
		int next=65536;
		figure_num=0;
	while(1)
	{
		memset(window,0,max_wind);
		memset(hash,~0,69632*4);
		wind_num=0;
		next=65536;
		unsigned int tmp=fread(window,1,max_wind,fp);
		c_hash_table(hash,&next,tmp);
		while (wind_num<tmp)
		{
				n_len=n_off=0;	
			//if (macth(&n_len,&n_off))
				if(hash_table(hash,&n_len,&n_off,tmp))
				{
					trans_more(n_len,n_off);
					wind_num=wind_num+n_len;
				}
				else
				{
					trans_one(window[wind_num]);
					wind_num++;
				}
		}
		fwrite(readtmp,figure_num/8,1,f_write);
		unsigned char c=readtmp[figure_num/8];
		memset(readtmp,0,max_read);
		readtmp[0]=c;
		figure_num=figure_num%8;
		file_s=file_s+tmp*100;
		printf("----->%d%c\n",file_s/file_num,37);
		if (tmp<max_wind)
		{
			if(figure_num!=0)
			fwrite(&readtmp[0],1,1,f_write);
			break;
		}
	
	}
	fclose(fp);
	fclose(f_write);
	return 1;
}
///yzw add 20140409
EXPORT int compress1(char * file,char * save_path)
{
		FILE * fp=fopen(file,"rb");
		FILE * f_write;
		unsigned int file_s=0;
		int file_num=0;
		if(fp==NULL) 
		{
			printf("no file\n");
			return -4;
		}
		else
		{
			if(save_path==NULL)
			{
				file_num=file_size(file);
				char tmp_name[256]={0};
				//strcpy(tmp_name,save_path);
				strcpy(tmp_name,file);
				strcat(tmp_name,".lz77");
				f_write=fopen(tmp_name,"wb+");
				if(f_write==NULL)
					return -5;
			}
			else
			{
				f_write=fopen(save_path,"wb+");
				if(f_write==NULL)
					return -6;	
			}
		}
		unsigned int n_len,n_off;
		int next=65536;
		figure_num=0;
	while(1)
	{
		memset(window,0,max_wind);
		memset(hash,~0,69632*4);
		wind_num=0;
		next=65536;
		unsigned int tmp=fread(window,1,max_wind,fp);
		c_hash_table(hash,&next,tmp);
		while (wind_num<tmp)
		{
				n_len=n_off=0;	
			//if (macth(&n_len,&n_off))
				if(hash_table(hash,&n_len,&n_off,tmp))
				{
					trans_more(n_len,n_off);
					wind_num=wind_num+n_len;
				}
				else
				{
					trans_one(window[wind_num]);
					wind_num++;
				}
		}
		fwrite(readtmp,figure_num/8,1,f_write);
		unsigned char c=readtmp[figure_num/8];
		memset(readtmp,0,max_read);
		readtmp[0]=c;
		figure_num=figure_num%8;
		file_s=file_s+tmp*100;
		//printf("----->%d%c\n",file_s/file_num,37);
		if (tmp<max_wind)
		{
			if(figure_num!=0)
			fwrite(&readtmp[0],1,1,f_write);
			break;
		}
	
	}
	fclose(fp);
	fclose(f_write);
	return 0;
}

///
int uncompress(char * file)
{
	FILE * fp=fopen(file,"rb");
	FILE * f_write;
	int i,k=0;
	unsigned int file_s=0;
	unsigned int file_size1=0;
	if(fp==NULL) 
	{
		printf("no file\n");
		return 0;
	}
	else
	{
		file_s=file_size(file);
		char tmp_name[133]={'l','z','7','7','_'};
		strcpy(&tmp_name[5],file);
		k=0;
		for(i=strlen(tmp_name);k<6;k++)
			tmp_name[i--]=0;
		f_write=fopen(tmp_name,"wb+");
	}
	figure_num=0;
	int mark_seek=0;
	while(1)
	{	
	unsigned int tmp;
		wind_num=0;
		memset(window,0,max_wind);
		if(feof(fp)==0)
		tmp=fread(readtmp,1,max_read,fp);
		while (wind_num<max_wind)
		{	
			if ((figure_num+7)/8>=tmp&&((readtmp[figure_num/8]>>(7-figure_num%8))%2==0))
				break;
				unsigned char c_1=readtmp[figure_num/8]>>(7-figure_num%8);
			if (c_1%2==1)
				fig_more();
			else
				fig_one();
		}
		fwrite(window,1,wind_num,f_write);
		file_size1=file_size1+tmp*100;
		printf("----->%d%c\n",file_size1/file_s,37);
		if(wind_num<max_wind) break;
		mark_seek=0-(tmp-figure_num/8);
		if(feof(fp)==0)
		{	
			fseek(fp,mark_seek,SEEK_CUR);
			figure_num=figure_num%8;
		}
	}
	fclose(fp);
	fclose(f_write);
	return 1;
}
////yzw add 20140409
char * findchar(char * str,char c)
{
	char * p2=NULL;
	char * p=NULL;
		if(str==NULL)
			return NULL;
		else
			p=str;
		while(p++)
		{
			if(*p==c)
				p2=p;
		}
		return p2;
}
EXPORT int uncompress1(char * file,char * save_path)
{
	FILE * fp=fopen(file,"rb");
	FILE * f_write;
	int i,k=0;
	unsigned int file_s=0;
	unsigned int file_size1=0;
	if(fp==NULL) 
	{
		printf("no file\n");
		return 0;
	}
	else
	{
		
		file_s=file_size(file);
		char tmp_name[256]={0};
		//strcpy(tmp_name,save_path);
		
		strcpy(tmp_name,file);
		k=0;
		
		/*char * f_p1 = NULL;
		f_p1=findchar(file,'\\');
		
			if(f_p1==NULL)
				return -1;
			else
				f_p1++;*/
		if(save_path==NULL)
		{
			for(i=strlen(tmp_name);k<5;k++)
				tmp_name[i--]=0;
			
			//strcat(tmp_name,".lz77un");
			f_write=fopen(tmp_name,"wb+");
			if(f_write==NULL)
				return -5;
		}
		else
		{
			f_write=fopen(save_path,"wb+");	
			if(f_write==NULL)
				return -6;
		}
	}
	figure_num=0;
	
	int mark_seek=0;
	while(1)
	{	
	unsigned int tmp;
		wind_num=0;
		memset(window,0,max_wind);
		if(feof(fp)==0)
		tmp=fread(readtmp,1,max_read,fp);
		while (wind_num<max_wind)
		{	
			if ((figure_num+7)/8>=tmp&&((readtmp[figure_num/8]>>(7-figure_num%8))%2==0))
				break;
				unsigned char c_1=readtmp[figure_num/8]>>(7-figure_num%8);
			if (c_1%2==1)
				fig_more();
			else
				fig_one();
		}
		fwrite(window,1,wind_num,f_write);
		file_size1=file_size1+tmp*100;
		printf("----->%d%c\n",file_size1/file_s,37);
		if(wind_num<max_wind) break;
		mark_seek=0-(tmp-figure_num/8);
		if(feof(fp)==0)
		{	
			fseek(fp,mark_seek,SEEK_CUR);
			figure_num=figure_num%8;
		}
	}
	fclose(fp);
	fclose(f_write);
	return 0;
}
////
//void main()
//{
//	char com_name[128]={0};
//	int choose;
//	while(1)
//	{
//		printf("1.compress\n2.uncompress\n0.exit\n");
//		scanf("%d",&choose);
//		switch(choose)
//		{
//		case 1:
//			while(1)
//			{
//				memset(com_name,1,128);
//				printf("please input file name\n");
//				scanf("%s",com_name);
//				if(compress(com_name)==1)break;
//			}
//			break;
//		case 2:
//			while(1)
//			{
//				memset(com_name,1,128);
//				printf("please input file name\n");
//				scanf("%s",com_name);
//				if(uncompress(com_name)==1)break;
//			}
//			break;
//		default : exit(0);
//		}
//	}
//	//compress("ceshi01.jpg");
////	compress("Linux.pdf");
//	//uncompress("c_file.txt");
//	//uncompress("compress2.txt");
//	//uncompress("tmp.txt");
//	//unsigned int i=32287;
//	//i=(i<<30)>>30;
////	printf("%u",i);
//		//FILE * tmp=fopen("123.mp3","rb");
//		//unsigned ch
//}
