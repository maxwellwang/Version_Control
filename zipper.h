#ifndef ZIPPER_H
#define ZIPPER_H

//actual function used to send compressed files
void zip_send(int fd, int len, char ** paths);


//remove any existing tar/directory
void zip_init();

//add file to tar
void zip_add(char * path);

//create tar and remove dir
void zip_tar();

//untar and remove tar, leaving _wtf_dir
void zip_untar();

//get size of tar (used for when sending)
int zip_size();
#endif
