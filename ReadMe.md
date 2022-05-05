# LINUX FILE SYSTEM IN C/C++

This program is a virtual implementation of file system using c/c++ that allows users to create a 500mb disk and perform mount/unmount it. In mounted state, the following operations can be performed on disk via a menu driven interface and prompts:-

1. Create file: Creates a file in the system and prints out the assigned inode number
2. Open file: Opens an existing file, allows to choose between read, write and append modes for it and prints the assigned file descriptor
3. Read file: By providing the descriptor of a file opened in read mode, all its contents are printed on screen
4. Write file: By providing the descriptor of a file opened in write mode, user is prompted to enter the text that should be written into the file. If file already has data in it, it is truncated and overwritten. User can enter `:q` in a new line to end text entry.
5. Append file:  By providing the descriptor of a file opened in write mode, user is prompted to enter the text that should be appended to the end of the file. User can enter `:q` in a new line to end text entry.
6. Close file: Closes file corresponding to the given descriptor
7. Delete file: Deletes a file given filename. If file is open, it cannot be deleted.
8. List of files: Lists all files that exist in filesystem along with their inode numbers.
9. List of opened files: Lists all files that are currently open along with their mode and file descriptors.
10. Unmount: Unmounts disk

When no disk is mounted, the following menu is presented.

1. Create disk: Creates a disk of given name
2. Mount disk: Mounts an existing disk
3. Exit
