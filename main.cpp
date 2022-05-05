#include<iostream>
#include<fcntl.h>
#include<bits/stdc++.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/stat.h>
#include<sys/types.h>
//define sb.blocksize 4000
//define sb.inodecount 308
//define sb.datacount 124659
//define sb.inodestart 33
//define sb.datastart 8224

using namespace std;

//Metadata blocks: 1250000
//Block 0-32: Superblock
//Inode blockcount: 308
//Number of inodes: 8000
//Number of inodes per block: 26
//Data blockcount: 124659


//Superblock class: Contains information about blocksize, number of blocks etc along with bitmaps keeping track of used blocks
class Superblock{
public:
	int blocksize = 4000;
	int inodecount = 1000;
	int inodeperblock = 7;
	int inodeblockcount = 143;
	int datacount = 124820;
	int inodestart = 34;
	int datastart = 178;
	int lastUsedInode = 0;
	int lastUsedDatablock = 0;
	char inodebitmap[1000];
	char databitmap[124820];
	int inodesmade = 0;
	int datablocksmade = 0;
	
	Superblock(){
		for(int i = 0; i < inodecount; i++){
			inodebitmap[i] = '0';
		}
		for(int i = 0; i < datacount; i++){
			databitmap[i] = '0';
		}
	}
};

Superblock sb;

//Class to store information about a file, including name, inode number, blocks under use etc
class Inode{
public:
	int number;
	char filename[30];
	int mode = 0;
	int allocatedblocks[125];
	int blocks_filled = 0;
	
	Inode(){
		number = -1;
		for(int i = 0; i < 125; i++){
			allocatedblocks[i] = -1;
		}
	}
	
	Inode(int num, char* fname){
		number = num;
		strcpy(filename, fname);
		for(int i = 0; i < 125; i++){
			allocatedblocks[i] = -1;
		}
	}
	
	void addBlock(int b, int s){
		allocatedblocks[blocks_filled] = b;
		blocks_filled++;
	}
	
	int getLastBlock(){
		if(blocks_filled == 0) return -1;
		else return allocatedblocks[blocks_filled-1];
	}
	
	void clearBlocks(){
		for(int i = 0; i < 125; i++){
			if(allocatedblocks[i] != -1){
				sb.databitmap[allocatedblocks[i]] = '0';
				allocatedblocks[i] = -1;
				sb.datablocksmade--;
			}
		}
		blocks_filled = 0;
	}
	
	void openFile(int m){
		mode = m;
	}
	
	void closeFile(){
		mode = 0;
	}
};

//Some basic declatarations to store data during runtime
unordered_map<string, Inode*> filelist;
unordered_map<int, Inode*> ffilelist;
unordered_map<string, Inode*> openedFileList;
unordered_map<int, Inode*> fopenedFileList;
int fd_to_inode[100];
int fd_in_use = 0;
int fd_arr[100];
string diskpath;
int diskfd = -1;
bool mounted = false;

//Function to fetch inode number and read it to an object
void readInode(Inode* inode, int number, int diskfd){
	int block = number%sb.inodeblockcount;
	int offset = number%sb.inodeperblock;
	
	int realblock = sb.inodestart + block;
	int pos = realblock*sb.blocksize + offset*sizeof(Inode);
	
	pread(diskfd, (char*)inode, sizeof(Inode), pos);
}

//Function to write inode to correct inode block at correct offset
void writeInode(Inode* inode, int number, int diskfd){
	int block = number%sb.inodeblockcount;
	int offset = number%sb.inodeperblock;
	
	int realblock = sb.inodestart + block;
	int pos = realblock*sb.blocksize + offset*sizeof(Inode);
	
	pwrite(diskfd, (char*)inode, sizeof(Inode), pos);
}

//Disk creation function
void createdisk(string path){
	struct stat buffer;
   	int exist = stat(path.c_str(), &buffer);
   	
   	if(!exist){
   		cout << "Disk by this name already exists" << endl;
   		return;
   	}

	int disk = open(path.c_str(), O_WRONLY | O_CREAT, 0777);
	pwrite(disk, " ", 1, 500*1000*1000);
	sb = Superblock();
	pwrite(disk, (char*)&sb, sizeof(Superblock), 0*sb.blocksize);
	
	close(disk);
}

void mountdisk(string path){
	diskpath = path;
	diskfd = open(path.c_str(), O_RDONLY, 0777);
	if(diskfd <= 0){
		cout << "Could not open disk" << endl;
		return;
	}
	
	memset((char*)&sb, 0, sizeof(Superblock));
	int res = pread(diskfd, (char*)&sb, sizeof(Superblock), 0*sb.blocksize);
	//cout << sizeof(Superblock) << endl;
	
	int readFile;
	//cout << "INODE START " << sb.inodestart << endl;  
	for(int i = 0; i < sb.inodecount; i++){
		if(sb.inodebitmap[i] == '1'){
			Inode* inode = new Inode();
			//cout << sb.inodestart + i << endl;
			readInode(inode, i, diskfd);
			//readFile = pread(diskfd, (char*)inode, sizeof(Inode), (sb.inodestart + i)*sb.blocksize);
			//cout << "Read file metadata: " << readFile << endl;
			//cout << inode->filename << endl;
			filelist[string(inode->filename)] = inode;
			//cout << "1" << endl;
			ffilelist[inode->number] = inode;
			//cout << "2" << endl;
		}
	}
	close(diskfd);
	
	for(int i = 0; i < 100; i++){
		fd_to_inode[i] = -1;
	}
	fd_in_use = 0;
	
	mounted = true;
}

void dismountdisk(){
	diskfd = open(diskpath.c_str(), O_WRONLY, 0777);
	diskpath = "";
	int writecount;
	//cout << "INODE START " << sb.inodestart << endl;
	for(int i = 0; i < sb.inodecount; i++){
		if(sb.inodebitmap[i] == '1'){
			//cout << sb.inodestart + i << endl;
			writeInode(ffilelist[i], i, diskfd);
			//writecount = pwrite(diskfd, (char*)ffilelist[i], sizeof(Inode), (sb.inodestart + i)*sb.blocksize);
			//if(writecount < 0) cout << strerror(errno) << endl;
			delete ffilelist[i];
			//cout << "Wrote file metadata: " << writecount << endl;
		}
	}
	
	pwrite(diskfd, (char*)&sb, sizeof(Superblock), 0*sb.blocksize);
	
	close(diskfd);
	mounted = false;
	filelist.empty();
	ffilelist.empty();
	openedFileList.empty();
	fopenedFileList.empty();
	for(int i = 0; i < 100; i++){
		fd_to_inode[i] = -1;
	}
	fd_in_use = 0;
}

void create_file(){
	if(sb.inodesmade >= sb.inodecount){
		cout << "Cannot make any more files, all inodes used" << endl;
		return;
	}

	char* filename = new char[100];
	cout << "Enter filename: ";
	cin >> filename;
	
	if(filelist.find(string(filename)) != filelist.end()){
		cout << "File by this name already exists" << endl;
		return;
	}

	int i = sb.lastUsedInode;
	while(sb.inodebitmap[i] == '1'){
		i = (i+1)%sb.inodecount;
	}

	sb.inodebitmap[i] = '1';
	sb.lastUsedInode = i;
	sb.inodesmade++;

	Inode *inode = new Inode(i, filename);
	filelist[filename] = inode;
	ffilelist[i] = inode;

	cout << "Filename: " << filename << " with inode number: " << i << " created" << endl;
}

void open_file(){
	if(fd_in_use >= 100){
		cout << "All file descriptors in use, close files to open another one" << endl;
		return;
	}

	cout << "Enter filename to open: ";
	char* filename = new char[100];
	cin >> filename;
	
	if(filelist.find(string(filename)) == filelist.end()){
		cout << "No such file exists" << endl;
		return;
	}
	else{
		cout << "Enter mode to open in:" << endl;
		cout << "0. Read" << endl;
		cout << "1. Write" << endl;
		cout << "2. Append" << endl;
		int mode_i;
		cin >> mode_i;
		char mode;
		switch(mode_i){
			case 0:
				mode = 'r';
				break;
			case 1:
				mode = 'w';
				break;
			case 2:
				mode = 'a';
				break;
			default:
				mode = '*';
		}
		
		if (mode != 'r' && mode != 'w' && mode != 'a'){
			cout << "Invalid mode" << endl;
			return;
		}
		
		Inode* fnode = filelist[string(filename)];
		fnode->mode = mode;
		openedFileList[string(filename)] = fnode;
		fopenedFileList[fnode->number] = fnode;
		int i = 0;
		while(i < 100 && fd_to_inode[i] != -1){
			i++;
		}
		fd_to_inode[i] = fnode->number;
		
		cout << "File " << filename << " opened in '" << mode << "' mode successfully. File descriptor: " << i << " and inode number: " << fd_to_inode[i] << endl;
	}
}

int assignFreeBlock(){
	if(sb.datablocksmade >= sb.datacount){
		cout << "No more space to write any further: File written possibly partially" << endl;
		return -1;
	}
	
	int i = sb.lastUsedDatablock;
	while(sb.databitmap[i] == '1'){
		i = (i+1)%sb.datacount;
	}
	return i;
}

int writeHelper(int blockno, char* content, int diskfd){
	pwrite(diskfd, 0, sb.blocksize, (sb.datastart + blockno)*sb.blocksize);
	int written = pwrite(diskfd, content, sb.blocksize, (sb.datastart + blockno)*sb.blocksize);
	return written;
}

int truncateBlock(int blockno, int diskfd){
	int written = pwrite(diskfd, 0, sb.blocksize, (sb.datastart + blockno)*sb.blocksize);
	return written;
}

void appendFile(int fd, string content){
	diskfd = open(diskpath.c_str(), O_RDWR, 0777);
	if(fopenedFileList.find(fd_to_inode[fd]) == fopenedFileList.end()){
		cout << "No such file opened" << endl;
		return;
	}
	
	Inode* fnode = fopenedFileList[fd_to_inode[fd]];
	if (fnode->mode != 'a'){
		cout << "Not in append mode" << endl;
		return;
	}
	
	int lastBlock = fnode->getLastBlock();
	int currBlock;
	char* buffer = new char[sb.blocksize];
	pread(diskfd, buffer, sb.blocksize, (sb.datastart + lastBlock)*sb.blocksize);
	
	string tempcontent;
	if(lastBlock != -1 && strlen(buffer) < sb.blocksize){
		tempcontent = string(buffer);
		content = tempcontent + content;
		currBlock = lastBlock;
		fnode->blocks_filled--;
	}
	else{
		currBlock = assignFreeBlock();
		if(currBlock == -1){
			cout << "No space to append" << endl;
			delete buffer;
			close(diskfd);
			return;
		}
	}
	//cout << "Content to write: " << endl << content << endl;
	
	memset(buffer, '\0', sb.blocksize);
	int written;
	while(content.size() > 0){
		sb.databitmap[currBlock] = '1';
		sb.lastUsedDatablock = currBlock;
		sb.datablocksmade++;
		strcpy(buffer, content.substr(0, sb.blocksize).c_str());
		written = writeHelper(currBlock, buffer, diskfd);
		if(written > 0) fnode->addBlock(currBlock, written);
		
		if(content.size() >= sb.blocksize){
			content = content.substr(sb.blocksize-1, content.size());
			memset(buffer, '\0', sb.blocksize);
			currBlock = assignFreeBlock();
			if(currBlock == -1){
				cout << "No more space to append, file written possibly partially" << endl;
				delete buffer;
				close(diskfd);
				return;
			}
		}
		else{
			break;
		}
	}
	
	delete buffer;
	close(diskfd);
	cout << "Append successful" << endl;
}

void writeFile(int fd, string content){
	diskfd = open(diskpath.c_str(), O_RDWR, 0777);
	if(fopenedFileList.find(fd_to_inode[fd]) == fopenedFileList.end()){
		cout << "No such file opened" << endl;
		return;
	}
	
	Inode* fnode = fopenedFileList[fd_to_inode[fd]];
	if (fnode->mode != 'w'){
		cout << "Not in write mode" << endl;
		return;
	}
	
	fnode->clearBlocks();
	char* buffer = new char[sb.blocksize];
	memset(buffer, '\0', sb.blocksize);
	int written;
	int currBlock = assignFreeBlock();
	if(currBlock == -1){
		cout << "Disk is full" << endl;
		delete buffer;
		close(diskfd);
		return;
	}
	while(content.size() > 0){
		sb.databitmap[currBlock] = '1';
		sb.lastUsedDatablock = currBlock;
		sb.datablocksmade++;
		strcpy(buffer, content.substr(0, sb.blocksize).c_str());
		written = writeHelper(currBlock, buffer, diskfd);
		if(written > 0) fnode->addBlock(currBlock, written);
		//cout << "Wrote " << written << endl;
		if(content.size() >= sb.blocksize){
			content = content.substr(sb.blocksize-1, content.size());
			memset(buffer, '\0', sb.blocksize);
			currBlock = assignFreeBlock();
			if(currBlock == -1){
				cout << "Disk full while writing, result possibly partial" << endl;
				delete buffer;
				close(diskfd);
				return;
			}
		}
		else{
			//cout << "Finished writing" << endl;
			break;
		}
	}
	
	delete buffer;
	close(diskfd);
	cout << "Write successful" << endl;
}

void readFile(int fd){
	diskfd = open(diskpath.c_str(), O_RDONLY, 0777);
	if(fopenedFileList.find(fd_to_inode[fd]) == fopenedFileList.end()){
		cout << "No such file opened" << endl;
		return;
	}
	
	Inode* fnode = fopenedFileList[fd_to_inode[fd]];
	if (fnode->mode != 'r'){
		cout << "Not in read mode" << endl;
		return;
	}
	
	char* buffer = new char[sb.blocksize];
	memset(buffer, '\0', sb.blocksize);
	string res = "";
	for(int i = 0; i < 10; i++){
		if(fnode->allocatedblocks[i] == -1) break;
		
		if(pread(diskfd, buffer, sb.blocksize, (sb.datastart + fnode->allocatedblocks[i])*sb.blocksize) > 0){
			res += string(buffer);
		}
		//cout << buffer;
	}
	
	cout << res << endl;
	
	delete buffer;
	close(diskfd);
	cout << "<End of File>" << endl;
}

void closeFile(int fd){
	if(fopenedFileList.find(fd_to_inode[fd]) == fopenedFileList.end()){
		cout << "No such file open" << endl;
		return;
	}
	
	
	fopenedFileList[fd_to_inode[fd]]->mode = '*';
	openedFileList.erase(string(fopenedFileList[fd_to_inode[fd]]->filename));
	fopenedFileList.erase(fd_to_inode[fd]);
	fd_to_inode[fd] = -1;
	cout << "File closed" << endl;
}

void listFiles(){
	if(filelist.size() == 0){
		cout << "No files created yet" << endl;
		return;
	}
	for(auto info : filelist){
		cout << info.first << ", inode number: " << info.second->number << endl;
	}
}

void listOpenFiles(){
	for(int i = 0; i < 100; i++){
		if(fd_to_inode[i] != -1){
			cout << fopenedFileList[fd_to_inode[i]]->filename << " opened with file descriptor " << i << " in '" << (char)fopenedFileList[fd_to_inode[i]]->mode << "' mode" << endl;
		}
	}
}

void unmountParse(){
	cout << "Enter choice:" << endl;
	cout << "1. Create disk" << endl;
	cout << "2. Mount disk" << endl;
	cout << "3. Exit" << endl;
	cout << "Enter: ";
	int n;
	cin >> n;
	
	if(n==1){
		string name;
		cout << "Enter diskname: ";
		cin >> name;
		
		createdisk(name);
	}
	else if(n==2){
		string name;
		cout << "Enter diskname: ";
		cin >> name;
		
		mountdisk(name);
	}
	else if(n==3){
		exit(0);
	}
	else{
		cout << "Invalid input" << endl;
	}
}

void deleteFile(){
	char* filename = new char[100];
	cout << "Enter filename: ";
	cin >> filename;
	
	if(filelist.find(string(filename)) == filelist.end()){
		cout << "No such file" << endl;
		return;
	}
	
	Inode* inode = filelist[string(filename)];
	if(openedFileList.find(string(filename)) != filelist.end()){
		cout << "File already open, cannot delete" << endl;
		return;
	}
	
	filelist.erase(string(filename));
	ffilelist.erase(inode->number);
	sb.inodebitmap[inode->number] = '0';
	inode->clearBlocks();
	
	delete inode;
	sb.inodesmade--;
	cout << "Deleted" << endl;
}

string readContent(){
	string res = "";
	string temp;
	while(getline(cin>>ws, temp)){
		res += "\n";
		//getline(cin>>ws, temp);
		if(temp == ":q") break;
		res += temp;
	}
	
	res.pop_back();
	
	cout << "Content to write: " << endl;
	cout << res << endl;
	
	return res;
}

void mountParse(){
	cout << "Working on disk: " << diskfd << endl;
	cout << "Enter choice:" << endl;
	cout << "1. Create file" << endl;
	cout << "2. Open file" << endl;
	cout << "3. Read file" << endl;
	cout << "4. Write file" << endl;
	cout << "5. Append file" << endl;
	cout << "6. Close file" << endl;
	cout << "7. Delete file" << endl;
	cout << "8. List of files" << endl;
	cout << "9. List of opened files" << endl;
	cout << "10. Unmount" << endl;
	cout << "Enter: ";
	
	int n;
	cin >> n;
	
	if(n==1){
		create_file();
	}
	else if(n==2){
		open_file();
	}
	else if(n==3){
		cout << "Enter file descriptor: ";
		int fd;
		cin >> fd;
		readFile(fd);
	}
	else if(n==4){
		cout << "Enter file descriptor: ";
		int fd;
		cin >> fd;
		string content = readContent();
		
		writeFile(fd, content);
	}
	else if(n==5){
		cout << "Enter file descriptor: ";
		int fd;
		cin >> fd;
		string content = readContent();
		
		appendFile(fd, content);
	}
	else if(n==6){
		cout << "Enter file descriptor: ";
		int fd;
		cin >> fd;
		closeFile(fd);
	}
	else if(n==7){
		deleteFile();
	}
	else if(n==8){
		listFiles();
	}
	else if(n==9){
		listOpenFiles();
	}
	else if(n==10){
		dismountdisk();
	}
	else{
		cout << "Invalid input" << endl;
	}
}

int main(){
	while(true){
		if(mounted) mountParse();
		else unmountParse();
		cout << endl;
	}
	
	return 0;
}
