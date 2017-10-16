#include <stdio.h>
#include <iostream>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <map>
using namespace std;

#define BUF_LEN 1000

int fd = inotify_init();
map<int, string> fs_inotify;

int dirScan(char *d_path){
	
	int wd;
	DIR *dir;
	struct dirent *ptr;
	char file_path[512] = {0};
	
	wd = inotify_add_watch(fd, d_path, IN_ALL_EVENTS);
	fs_inotify.insert(pair<int, string>(wd, d_path));

	dir = opendir(d_path);
	while((ptr = readdir(dir)) != NULL){
		if((ptr->d_type) == 4 && ptr->d_name[0] != '.'){			
			strcpy(file_path, d_path);
			strcat(file_path, "/");
			strcat(file_path, ptr->d_name);
			dirScan(file_path);
		}
	}
	return 0;
}

int eventDelete(char *path){
	
	map<int, string>::iterator fs;
	
	for(fs = fs_inotify.begin(); fs != fs_inotify.end(); ++fs){
		if(fs->second == path){
			inotify_rm_watch(fd, fs->first);
		 	fs_inotify.erase(fs);
			return 0;
		}
	}
	return -1;
}

void displayEvent(struct inotify_event *i) {
	
	struct stat info;
	char tmp_path[512] = {0};
	
	if(fs_inotify.find(i->wd) != fs_inotify.end()){
		
		strcpy(tmp_path, (fs_inotify.find(i->wd)->second).data());
		strcat(tmp_path, "/");
		strcat(tmp_path, i->name);
		lstat(tmp_path, &info);
	
		if (i->mask & IN_MODIFY){
			if(S_ISDIR(info.st_mode))
				printf("folder modified : %s\n", tmp_path);
			else
				printf("file modified : %s\n", tmp_path);
		}	
		if (i->mask & IN_DELETE){
			if(S_ISDIR(info.st_mode)){
				printf("folder deleted : %s\n", tmp_path);
				eventDelete(tmp_path);
			}
			else
				printf("file deleted : %s\n", tmp_path);
		}
		if (i->mask & IN_CREATE){
			if(S_ISDIR(info.st_mode)){
				printf("folder created : %s\n", tmp_path);
				dirScan(tmp_path);
			}	
			else
				printf("file created : %s\n", tmp_path);
		}
		if (i->mask & IN_MOVED_FROM){
			if(S_ISDIR(info.st_mode)){
				printf("folder moved out : %s\n", tmp_path);
				eventDelete(tmp_path);
			}	
			else
				printf("file moved out : %s\n", tmp_path);
		}
		if (i->mask & IN_MOVED_TO){
			if(S_ISDIR(info.st_mode)){
				printf("folder moved in : %s\n", tmp_path);
				dirScan(tmp_path);
			}	
			else
				printf("file moved in : %s\n", tmp_path);
		}
	}
}

int main() {
		
	char buf[BUF_LEN];
	char root_path[] = "/test";
	char *p;
	ssize_t Event_len;
	struct inotify_event *event;
	
	dirScan(root_path);
	
	printf("\n-----[+] file system monitor starting [+]-----\n\n");
	while (1) {
		Event_len = read(fd, buf, BUF_LEN);
		for (p = buf; p < buf + Event_len; ) {
			event = (struct inotify_event *) p;
			displayEvent(event);
			p += sizeof(struct inotify_event) + event->len;
		}
	}
}
