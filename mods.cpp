#include<bits/stdc++.h>
#include<dlfcn.h>
#include<PFishHook.h>
#include "mods.h"
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

std::list<void*> mods;
std::unordered_map<std::string,char*> hooks;
char* ppool;
void catch_signal(int sign);
void MOD_loadall(){
  signal(SIGSEGV, catch_signal);
    struct dirent *entry;
     struct stat statbuf;
     ppool=(char*)mmap(0,65536,7,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    DIR *dp=opendir("mods");
    if(dp==NULL){
      mkdir("mods",S_IRWXU);
      dp=opendir("mods");
    }
    while((entry = readdir(dp)) != NULL) {
      if(strcmp(".so",(entry->d_name+strlen(entry->d_name)-3))==0){
       char buf[8192];
       sprintf(buf,"mods/%s",entry->d_name);
       lstat(buf, &statbuf);
       if(S_IFREG&statbuf.st_mode){
         printf("load %s\n",entry->d_name);
         void* handle=dlopen(buf,RTLD_LAZY);
         mods.push_back(handle);
         void* entry=dlsym(handle,"mod_init");
         printf("%p %p\n",handle,entry);
         if(entry==NULL){
           printf("Failed to load %s,mod_init not found\n",buf);
           continue;
         }
         ((void(*)())entry)();
       }
     }
    }
}
#define BACKTRACE_SIZE   16

void dump(void)
{
	int j, nptrs;
	void *buffer[BACKTRACE_SIZE];
	char **strings;
	
	nptrs = backtrace(buffer, BACKTRACE_SIZE);
	
	printf("backtrace() returned %d addresses\n", nptrs);

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	for (j = 0; j < nptrs; j++)
		printf("  [%02d] %s\n", j, strings[j]);

	free(strings);
}

void catch_signal(int sign)
{
    switch (sign)
    {
    case SIGSEGV:
        dump();
        exit(0);
        break;
    }
}
void* MCHook(std::string&& name,void* func){
  printf("call hook %s %p\n",name.c_str(),func);
  void* oldfunc;
  if(hooks.count(name)==0){
    printf("[DBG] Creating Proxy for %s\n",name.c_str());
    memcpy(ppool,"\xff\x25\x00\x00\x00\x00",6);
    memcpy(ppool+6,&func,8);
    HookIt(dlsym(MCHandle(),name.c_str()),&oldfunc,ppool);
    hooks[name]=ppool;
    ppool+=14;
    return oldfunc;
  }else{
    printf("[DBG] Modifying Proxy for %s\n",name.c_str());
    char* proxy=hooks[name];
    memcpy(&oldfunc,proxy+6,8);
    memcpy(proxy+6,&func,8);
    return oldfunc;
  }
  return oldfunc;
}
