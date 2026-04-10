#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
int main(int argc,char**argv){
if(argc<3)return 1;
FILE*in=fopen(argv[1],"rb");if(!in)return 1;
fseek(in,0,SEEK_END);long sz=ftell(in);rewind(in);
int32_t mem[1000]={0};long bit=0;
for(long i=0;i<sz;i++){
int ch=fgetc(in);
if(ch==' ')bit++;
else if(ch=='\t'){mem[(bit/4)%1000]|=(1<<(3-(bit%4)));bit++;}
}
fclose(in);
unsigned char elf[120]={
0x7f,0x45,0x4c,0x46,2,1,1,0,0,0,0,0,0,0,0,0,
2,0,0x3e,0,1,0,0,0,0x78,0,0x40,0,0,0,0,0,
0x40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0x40,0,0x38,0,1,0,0,0,0,0,0,0,
1,0,0,0,7,0,0,0,0,0,0,0,0,0,0,0,
0,0,0x40,0,0,0,0,0,0,0,0x40,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0x20,0,0,0,0,0};
unsigned char x86[]={0xb8,0x3c,0,0,0,0xbf,0x2a,0,0,0,0x0f,0x05};
long fsz=120+sizeof(x86)+sizeof(mem);
memcpy(elf+104,&fsz,4);memcpy(elf+112,&fsz,4);
FILE*out=fopen(argv[2],"wb");
fwrite(elf,1,120,out);fwrite(x86,1,sizeof(x86),out);fwrite(mem,1,sizeof(mem),out);
fclose(out);
printf("Native ELF Compiled: %s\n",argv[2]);
return 0;
}
