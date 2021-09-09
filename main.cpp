/* This code has compiled in c++ (GCC) 10.2.1 20201125 (Red Hat 10.2.1-9)
*/

#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>

struct data            // structure of 1 kB block
{
    char block[1024];
};

struct inode            //structure of inode
{
    char name[8];
    int size;
    int blockPointers[8];
    int used;
};

// mix and hash used to find hash of command ls, df, rf etc
uint64_t constexpr mix(char m, uint64_t s)
{
    return ((s << 7) + ~(s >> 3)) + ~m;
}

uint64_t constexpr hash(const char *m)
{
    return (*m) ? mix(*m, hash(m + 1)) : 0;
}

// assigning name to file
void assignName(inode *pointer, int node, char *name)
{

    int i;
    for (i = 0; *(name + i); i++)
    {
        (pointer + node)->name[i] = name[i];
    }

    (pointer + i)->name[i] = '\0';

    return;
}

// assigning all details in fee inode
void assignInodeDetails(inode *pointer, int node, char *name, int size, char *freeBlocksPointer, data *dataBlockPointer)
{
    assignName(pointer, node, name);
    (pointer + node)->size = size;
    (pointer + node)->used = 1;
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < 128; j++)
        {
            if (*(freeBlocksPointer + j) == '\0')
            {
                (pointer + node)->blockPointers[i] = j;
                *(freeBlocksPointer + j) = 'n';
                *((char *)(dataBlockPointer + (j - 1))) = '\0';
                break;
            }
        }
    }
    for (int i = size; i < 8; i++)
    {
        (pointer + node)->blockPointers[i] = 0;
    }
}

void createFile(char *name, int size, char *freeBlockInfo, inode *inodeInfo, data *dataBlockInfo)
{
    if (size > 8)
    {
        std::cout << "Size of file must be less than or equal to 8" << std::endl;
        return;
    }
    for (int i = 0; i < 16; i++) // finding free inode to create file
    {
        if ((inodeInfo + i)->used == 1 && !strcmp((inodeInfo + i)->name, name))
        {
            std::cout << "File already exist choose different name" << std::endl;
            return;
        }
    }
    for (int i = 0; i < 16; i++)
    {
        if ((inodeInfo + i)->used == 0)
        {
            assignInodeDetails(inodeInfo, i, name, size, freeBlockInfo, dataBlockInfo);
            break;
        }
    }
    return;
}


void deleteFile(char *name, char *freeBlockInfo, inode *inodeInfo)
{
    int i;
    int size;
    int inodes = 0;
    for (i = 0; i < 16; i++)            // finding file with same name and deleting file
    {
        if ((inodeInfo + i)->used == 1)
        {
            inodes++;
            int same = strcmp((inodeInfo + i)->name, name);
            if (!same)
            {
                size = (inodeInfo + i)->size;

                int j = 0;
                int index;
                while (size--)
                {
                    index = (inodeInfo + i)->blockPointers[j];
                    *(freeBlockInfo + index) = '\0';
                    (inodeInfo + i)->blockPointers[j++] = 0;
                }
                (inodeInfo + i)->used = 0;
                return;
            }
        }
    }
    if (!inodes)
    {
        std::cout << "Disk is empty" << std::endl;
        return;
    }
    std::cout << "File name don't exist" << std::endl;
    return;
}

void listFiles(inode *inodeInfo) // list all file
{
    int horizontal = 0;
    int file = 0;
    for (int i = 0; i < 16; i++)
    {

        if ((inodeInfo + i)->used == 1)
        {
            std::cout << (inodeInfo + i)->name << "        ";
            horizontal++;
            file++;
        }
        if (horizontal == 6)
        {
            std::cout << std::endl;
            horizontal = 0;
        }
    }
    if (file)
    {
        std::cout << std::endl;
    }
    if (!file)
    {
        std::cout << "Disk is empty" << std::endl;
    }
    return;
}

void writeFile(char *name, int block, char *buffer, inode *inodeInfo, char *freeBufferInfo, data *dataBlockInfo)
{
    int i;
    int copy = 1;
    int index;
    for (i = 0; i < 16; i++) // finding file from file name
    {
        copy = strcmp((inodeInfo + i)->name, name);

        if (copy == 0)
        {
            index = i;
            break;
        }
    }

    if (copy != 0)
    {
        std::cout << "This file does not exist you have to first create that file";
        return;
    }

    if ((inodeInfo + index)->size < block)
    {
        std::cout << "Size of file is " << (inodeInfo + index)->size << "you can't write to the file choose block number less than or equal to" << (inodeInfo + index)->size << std::endl;
    }

    char option = 'y';
    if (strlen((char *)(dataBlockInfo + (((inodeInfo + index)->blockPointers[block - 1]) - 1))))
    {
        std::cout << "some content is already present do you want to override (y or n): ";
        std::cin >> option;
    }

    if (option == 'y') // write to file from buffer
    {
        bcopy(buffer, (char *)(dataBlockInfo + (((inodeInfo + index)->blockPointers[block - 1]) - 1)), strlen(buffer));
    }
}

int readFile(char *name, int block, char *buffer, inode *inodeInfo, data *dataBlockInfo)
{
    int i;
    int copy = 1;
    int index;
    int count = 0;
    for (i = 0; i < 16; i++)  // finding file by file name
    {
        if (!strcmp((inodeInfo + i)->name, name) && (inodeInfo + i)->used)
        {
            index = i;
            break;
        }
        else
        {
            count++;
        }
    }

    if (count == 16)
    {
        return 2;
    }
        // coping file's block content into buffer
    data *blockPointer = (dataBlockInfo + (((inodeInfo + index)->blockPointers[block - 1]) - 1));
    bcopy((char *)blockPointer, buffer, strlen((char *)blockPointer));
    if (!strlen(buffer))
    {
        return 0;
    }
    return 1;
}


int main()
{
    void *pointer = new (std::nothrow) char[128 * 1024]();

    if (!pointer)
    {
        std::cout << "Memory allocation failed" << std::endl;
    }

    char *freeBlockInfo = (char *)pointer; 
    inode *inodeInfo = (inode *)((char *)pointer + 128);
    data *dataBlockInfo = (data *)((char *)pointer + 1024);
    *(freeBlockInfo) = 'n';

    int size;
    int option;
    char name[8];
    char buffer[1024];

    std::cout << "type 'mf' for make file\n" 
                    << "    'df' for delete file\n"
                    << "    'wf' for write file\n"
                    << "    'rf' for read file\n"
                    << "    'ls' for list all file\n"
                    << "    'clr' for clear terminal screen\n"
                    << "    'exit' to exit" << std::endl;

    while (true)   // runs until get exit command
    {
        char command[6];
        std::cin >> command;
        switch (hash(command))
        {
        case hash("clr"):
        {
            system("clear");
            break;
        }
        case hash("mf"):
        {
            std::cout << "Enter file name: ";
            std::cin >> name;
            std::cout << "Enter size of file: ";
            std::cin >> size;
            createFile(name, size, freeBlockInfo, inodeInfo, dataBlockInfo);
            break;
        }
        case hash("df"):
        {
            std::cout << "Enter File name: ";
            std::cin >> name;
            deleteFile(name, freeBlockInfo, inodeInfo);
            break;
        }
        case hash("wf"):
        {
            int block;
            std::cout << "Enter file name: ";
            std::cin >> name;

            std::cout << "Enter block number: ";
            std::cin >> block;

            std::cout << "Type content:";

            int i = 0;
            char c = getchar();
            while ((c = getchar()) != '\n')
            {
                buffer[i++] = c;
            }
            buffer[i] = '\0';
            writeFile(name, block, buffer, inodeInfo, freeBlockInfo, dataBlockInfo);
            break;
        }
        case hash("rf"):
        {
            int block;
            char buffer[1024];

            std::cout << "Enter file name: ";
            std::cin >> name;

            std::cout << "Enter block number that you want to read: ";
            std::cin >> block;

            int len = readFile(name, block, buffer, inodeInfo, dataBlockInfo);

            if (len == 0)
            {
                std::cout << "Block is empty" << std::endl;
                break;
            }
            else if (len == 2)
            {
                std::cout << "This file does not exist Enter correct file name" << std::endl;
                break;
            }

            char c;
            int i = 0;  // printing buffer content 
            while ((c = buffer[i++]) != '\0')
            {
                putchar(c);
            }
            bzero(buffer, strlen(buffer));
            std::cout << std::endl;
            break;
        }
        case hash("ls"):
        {

            listFiles(inodeInfo);
            break;
        }
        case hash("exit"):
        {
            return 0;
        }

        default:
        {
            std::cout << "command not found" << std::endl;
        }
        }
    }
    return 0;
}