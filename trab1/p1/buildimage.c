/* Author(s): <Your name(s) here>
 * Creates operating system image suitable for placement on a boot disk
*/
/* TODO: Comment on the status of your submission. Largely unimplemented */
#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512       /* floppy sector size in bytes */
#define BOOTLOADER_SIG_OFFSET 0x1fe /* offset for boot loader signature */
// more defines...

// Function to read ELF header from file
Elf32_Ehdr* read_elf_header(FILE** execfile, char *filename) {

    *execfile = fopen(filename, "rb");
    if (*execfile == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Allocate memory for the ELF header
    Elf32_Ehdr* elf_header = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));

    if (elf_header == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    // Read the ELF header from the file
    if (fread(elf_header, sizeof(Elf32_Ehdr), 1, *execfile) != 1) {
        fprintf(stderr, "Error reading ELF header from %s ELF file\n", filename);
        free(elf_header);
        return NULL;
    }

    printf("%s's ELF heaedr read successfully!\n", filename);

    return elf_header;
}

void * read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr *ehdr) {
    void* header = NULL;

    *execfile = fopen(filename, "rb");
    if (*execfile == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Check if the file has a section header table
    if (ehdr->e_phnum == 0) {
        printf("The %s ELF file does not have a program header table.\n", filename);
        printf("Reading %s's section header table instead.\n", filename);

        // Allocate memory for section header table
        Elf32_Shdr *shdr_table = (Elf32_Shdr *)malloc(ehdr->e_shentsize * ehdr->e_shnum);
        if (shdr_table == NULL) {
            perror("Memory allocation failed");
            fclose(*execfile);
            return NULL;
        }

        // Read section header table
        fseek(*execfile, ehdr->e_shoff, SEEK_SET);
        fread(shdr_table, ehdr->e_shentsize, ehdr->e_shnum, *execfile);

        printf("%s's section header table read successfully!\n", filename);
        header = shdr_table;
    }
    else {
        // Allocate memory for program header table
        Elf32_Phdr *phdr_table = (Elf32_Phdr *)malloc(ehdr->e_phentsize * ehdr->e_phnum);
        if (phdr_table == NULL) {
            perror("Memory allocation failed");
            fclose(*execfile);
            return NULL;
        }

        // Read program header table
        fseek(*execfile, ehdr->e_phoff, SEEK_SET);
        fread(phdr_table, ehdr->e_phentsize, ehdr->e_phnum, *execfile);

        printf("%s's program header table read successfully!\n", filename);
        header = phdr_table;
    }
    return header;
}

// // Reads in an executable file in ELF format and return it's sections headers
// Elf32_Shdr * read_section_header_table(FILE **execfile, char *filename, Elf32_Ehdr *ehdr)
// {
//     *execfile = fopen(filename, "rb");
//     if (*execfile == NULL) {
//         perror("Error opening file");
//         return NULL;
//     }

//     // Check if the file has a section header table
//     if (ehdr->e_shnum == 0) {
//         fprintf(stderr, "Error reading section header table of %s ELF file\n", filename);
//         printf("The ELF file does not have a section header table.\n");
//         fclose(*execfile);
//         return NULL;
//     }

//     // Allocate memory for section header table
//     Elf32_Shdr *shdr_table = (Elf32_Shdr *)malloc(ehdr->e_shentsize * ehdr->e_shnum);
//     if (shdr_table == NULL) {
//         perror("Memory allocation failed");
//         fclose(*execfile);
//         return NULL;
//     }

//     // Read section header table
//     fseek(*execfile, ehdr->e_shoff, SEEK_SET);
//     fread(shdr_table, ehdr->e_shentsize, ehdr->e_shnum, *execfile);

//     printf("%s's section header table read successfully!\n", filename);

//     return shdr_table;
// }

// // Reads in an executable file in ELF format and return it's program header
// Elf32_Phdr * read_program_header_table(FILE **execfile, char *filename, Elf32_Ehdr *ehdr)
// { 
// 	*execfile = fopen(filename, "rb");
//     if (*execfile == NULL) {
//         perror("Error opening file");
//         return NULL;
//     }

//     // Check if the file has a program header
//     if (ehdr->e_phnum == 0) {
//         fprintf(stderr, "Error reading program heaer of %s ELF file\n", filename);
//         printf("The ELF file does not have a program header.\n");
//         fclose(*execfile);
//         return NULL;
//     }

//     // Allocate memory for program header table
//     Elf32_Phdr *phdr_table = (Elf32_Phdr *)malloc(ehdr->e_phentsize * ehdr->e_phnum);
//     if (phdr_table == NULL) {
//         perror("Memory allocation failed");
//         fclose(*execfile);
//         return NULL;
//     }

//     // Read program header table
//     fseek(*execfile, ehdr->e_phoff, SEEK_SET);
//     fread(phdr_table, ehdr->e_phentsize, ehdr->e_phnum, *execfile);

// 	printf("%s's program header table read successfully!\n", filename);

//     return phdr_table;
// }

/* Writes the bootblock to the image file */
void write_bootblock(FILE **imagefile, FILE *bootfile, Elf32_Shdr *boot_shdr_table)
{
    unsigned char *section_buffer;
    int useful_sections[] = {1, 2, 3, 13, 14, 15}; // Indices of useful sections

    // Iterate through useful section indices
    for (int i = 0; i < sizeof(useful_sections) / sizeof(useful_sections[0]); i++) {
        int idx = useful_sections[i];

        // Allocate buffer for section contents
        section_buffer = (unsigned char *)malloc(boot_shdr_table[idx].sh_size);
        if (section_buffer == NULL) {
            perror("Memory allocation failed");
            fclose(*imagefile);
            return;
        }

        // Read section contents from ELF file
        fseek(bootfile, boot_shdr_table[idx].sh_offset, SEEK_SET);
        fread(section_buffer, 1, boot_shdr_table[idx].sh_size, bootfile);

        // Write section contents to image file
        fwrite(section_buffer, 1, boot_shdr_table[idx].sh_size, *imagefile);

        // Free buffer memory
        free(section_buffer);
    }
}

/* Writes the kernel to the image file */
void write_kernel(FILE **imagefile,FILE *kernelfile, Elf32_Phdr *kernel_phdr)
{ 
}

/* Counts the number of sectors in the kernel */
int count_kernel_sectors(Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr)
{   
    return 0;
}

/* Records the number of sectors in the kernel */
void record_kernel_sectors(FILE **imagefile,Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr, int num_sec)
{    
}


/* Prints segment information for --extended option */
void extended_opt(Elf32_Phdr *bph, int k_phnum, Elf32_Phdr *kph, int num_sec)
{

	/* print number of disk sectors used by the image */

  
	/* bootblock segment info */
 

	/* print kernel segment info */
  

	/* print kernel size in sectors */
}
// more helper functions...

/* MAIN */
int main(int argc, char **argv)
{
	FILE *kernelfile, *bootfile, *imagefile; // file pointers for bootblock, kernel, and image
    Elf32_Ehdr *boot_ehdr; // bootblock ELF header
    Elf32_Ehdr *kernel_ehdr; // kernel ELF header
    Elf32_Shdr *boot_shdr_table; // bootblock section header table
    Elf32_Phdr *kernel_program_header; // kernel program header

    // Read bootblock ELF header
    boot_ehdr = read_elf_header(&bootfile, "bootblock.o");

    // Read kernel ELF header
    kernel_ehdr = read_elf_header(&kernelfile, "kernel.s");

	/* build image file */
	if (imagefile == NULL) {
        imagefile = fopen("image.o", "wb");
        if (imagefile == NULL) {
            perror("Error opening image file");
            exit(EXIT_FAILURE);
        }
    }

	/* read executable bootblock file */  
    boot_shdr_table = read_exec_file(&bootfile, "bootblock.o", boot_ehdr);

	/* write bootblock */  
	// write_bootblock(&imagefile, bootfile, boot_program_header);

	/* read executable kernel file */
    kernel_program_header = read_exec_file(&kernelfile, "kernel.s", kernel_ehdr);

	/* write kernel segments to image */

	/* tell the bootloader how many sectors to read to load the kernel */

	/* check for  --extended option */
	// if(!strncmp(argv[1], "--extended", 11)) {
	// 	/* print info */
	// }
	
	// Clean up
    printf("freeing pointers...\n");
    free(boot_ehdr);
    free(kernel_ehdr);
    free(boot_shdr_table);
    free(kernel_program_header);
    fclose(bootfile);
    fclose(kernelfile);
	// fclose(imagefile);
  
	return 0;
} // ends main()



